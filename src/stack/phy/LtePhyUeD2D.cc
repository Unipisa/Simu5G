//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include <assert.h>
#include "stack/phy/LtePhyUeD2D.h"
#include "stack/phy/packet/LteFeedbackPkt.h"
#include "stack/d2dModeSelection/D2DModeSelectionBase.h"

namespace simu5g {

Define_Module(LtePhyUeD2D);
using namespace inet;



void LtePhyUeD2D::initialize(int stage)
{
    LtePhyUe::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        d2dTxPower_ = par("d2dTxPower");
        d2dMulticastEnableCaptureEffect_ = par("d2dMulticastCaptureEffect");
        d2dDecodingTimer_ = nullptr;
    }
}

void LtePhyUeD2D::handleSelfMessage(cMessage *msg)
{
    if (msg->isName("d2dDecodingTimer")) {
        // Select one frame from the buffer. Implements the capture effect.
        LteAirFrame *frame = extractAirFrame();
        UserControlInfo *lteInfo = check_and_cast<UserControlInfo *>(frame->removeControlInfo());

        // Decode the selected frame.
        decodeAirFrame(frame, lteInfo);

        // Clear buffer.
        while (!d2dReceivedFrames_.empty()) {
            LteAirFrame *frame = d2dReceivedFrames_.back();
            d2dReceivedFrames_.pop_back();
            delete frame;
        }

        delete msg;
        d2dDecodingTimer_ = nullptr;
    }
    else if (msg->isName("doModeSwitchAtHandover")) {
        // Call mode selection module to check if DM connections are possible.
        cModule *enb = getSimulation()->getModule(binder_->getOmnetId(masterId_));
        D2DModeSelectionBase *d2dModeSelection = check_and_cast<D2DModeSelectionBase *>(enb->getSubmodule("cellularNic")->getSubmodule("d2dModeSelection"));
        d2dModeSelection->doModeSwitchAtHandover(nodeId_, true);

        delete msg;
    }
    else
        LtePhyUe::handleSelfMessage(msg);
}

// TODO: ***reorganize*** method
void LtePhyUeD2D::handleAirFrame(cMessage *msg)
{
    UserControlInfo *lteInfo = check_and_cast<UserControlInfo *>(msg->removeControlInfo());

    connectedNodeId_ = masterId_;
    LteAirFrame *frame = check_and_cast<LteAirFrame *>(msg);
    EV << "LtePhyUeD2D: received new LteAirFrame with ID " << frame->getId() << " from channel" << endl;

    MacNodeId sourceId = lteInfo->getSourceId();
    if (binder_->getOmnetId(sourceId) == 0) {
        // Source has left the simulation.
        delete msg;
        return;
    }

    double carrierFreq = lteInfo->getCarrierFrequency();
    LteChannelModel *channelModel = getChannelModel(carrierFreq);
    if (channelModel == nullptr) {
        EV << "Received packet on carrier frequency not supported by this node. Delete it." << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    // Check if the message is from a different cellular technology.
    if (lteInfo->isNr() != isNr_) {
        EV << "Received packet [from NR=" << lteInfo->isNr() << "] from a different radio technology [to NR=" << isNr_ << "]. Delete it." << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    // Update coordinates of this user.
    if (lteInfo->getFrameType() == HANDOVERPKT) {
        // Check if the message is on another carrier frequency or handover is already in process.
        if (carrierFreq != primaryChannelModel_->getCarrierFrequency() || (handoverTrigger_ != nullptr && handoverTrigger_->isScheduled())) {
            EV << "Received handover packet on a different carrier frequency. Delete it." << endl;
            delete lteInfo;
            delete frame;
            return;
        }

        handoverHandler(frame, lteInfo);
        return;
    }

    // Check if the frame is for us (MacNodeId matches or - if this is a multicast communication - enrolled in multicast group).
    if (lteInfo->getDestId() != nodeId_ && !(binder_->isInMulticastGroup(nodeId_, lteInfo->getMulticastGroupId()))) {
        EV << "ERROR: Frame is not for us. Delete it." << endl;
        EV << "Packet Type: " << phyFrameTypeToA((LtePhyFrameType)lteInfo->getFrameType()) << endl;
        EV << "Frame MacNodeId: " << lteInfo->getDestId() << endl;
        EV << "Local MacNodeId: " << nodeId_ << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    /*
     * This could happen if the ue associates with a new master while a packet from the
     * old master is in-flight: the packet is in the air
     * while the ue changes master.
     * Event timing:      TTI x: packet scheduled and sent by the UE (tx time = 1ms)
     *                     TTI x+0.1: ue changes master
     *                     TTI x+1: packet from UE arrives at the old master
     */
    if (lteInfo->getDirection() != D2D && lteInfo->getDirection() != D2D_MULTI && lteInfo->getSourceId() != masterId_) {
        EV << "WARNING: frame from a UE that is leaving this cell (handover): deleted " << endl;
        EV << "Source MacNodeId: " << lteInfo->getSourceId() << endl;
        EV << "UE MacNodeId: " << nodeId_ << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    if (binder_->isInMulticastGroup(nodeId_, lteInfo->getMulticastGroupId())) {
        // HACK: if this is a multicast connection, change the destId of the airframe so that upper layers can handle it.
        lteInfo->setDestId(nodeId_);
    }

    // Send H-ARQ feedback up.
    if (lteInfo->getFrameType() == HARQPKT || lteInfo->getFrameType() == GRANTPKT || lteInfo->getFrameType() == RACPKT || lteInfo->getFrameType() == D2DMODESWITCHPKT) {
        handleControlMsg(frame, lteInfo);
        return;
    }

    // This is a DATA packet.

    if (masterId_ == NODEID_NONE) {
        // UE is not (anymore) associated with any eNB/gNB and all harqBuffers are already deleted.
        // Handing this data packet to the MAC layer will lead to null pointers.
        EV << "LtePhyUeD2D: UE " << nodeId_ << " received data packet while not associated with any base station. (masterId " << masterId_ << "). Drop it." << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    // If the packet is a D2D multicast one, store it and decode it at the end of the TTI.
    if (d2dMulticastEnableCaptureEffect_ && binder_->isInMulticastGroup(nodeId_, lteInfo->getMulticastGroupId())) {
        // If not already started, auto-send a message to signal the presence of data to be decoded.
        if (d2dDecodingTimer_ == nullptr) {
            d2dDecodingTimer_ = new cMessage("d2dDecodingTimer");
            d2dDecodingTimer_->setSchedulingPriority(10);          // Last thing to be performed in this TTI.
            scheduleAt(NOW, d2dDecodingTimer_);
        }

        // Store frame, together with related control info.
        frame->setControlInfo(lteInfo);
        storeAirFrame(frame);            // Implements the capture effect.

        return;                          // Exit the function, decoding will be done later.
    }

    if ((lteInfo->getUserTxParams()) != nullptr) {
        int cw = lteInfo->getCw();
        if (lteInfo->getUserTxParams()->readCqiVector().size() == 1)
            cw = 0;
        double cqi = lteInfo->getUserTxParams()->readCqiVector()[cw];
        if (lteInfo->getDirection() == DL) {
            emit(averageCqiDlSignal_, cqi);
            recordCqi(cqi, DL);
        }
    }
    // Apply decider to received packet.
    bool result = true;
    RemoteSet r = lteInfo->getUserTxParams()->readAntennaSet();
    if (r.size() > 1) {
        // DAS
        for (auto it : r) {
            EV << "LtePhyUeD2D: Receiving Packet from antenna " << it << "\n";

            /*
             * On UE set the sender position
             * and tx power to the sender das antenna
             */

            RemoteUnitPhyData data;
            data.txPower = lteInfo->getTxPower();
            data.m = getRadioPosition();
            frame->addRemoteUnitPhyDataVector(data);
        }
        // Apply analog models For DAS.
        result = channelModel->isErrorDas(frame, lteInfo);
    }
    else {
        result = channelModel->isError(frame, lteInfo);
    }

    // Update statistics.
    if (result)
        numAirFrameReceived_++;
    else
        numAirFrameNotReceived_++;

    EV << "Handled LteAirframe with ID " << frame->getId() << " with result "
       << (result ? "RECEIVED" : "NOT RECEIVED") << endl;

    auto pkt = check_and_cast<inet::Packet *>(frame->decapsulate());

    // Here frame has to be destroyed since it is no more useful.
    delete frame;

    // Attach the decider result to the packet as control info.
    lteInfo->setDeciderResult(result);
    *(pkt->addTagIfAbsent<UserControlInfo>()) = *lteInfo;
    delete lteInfo;

    // Send decapsulated message along with result control info to upperGateOut_.
    send(pkt, upperGateOut_);

    if (getEnvir()->isGUI())
        updateDisplayString();
}

void LtePhyUeD2D::triggerHandover()
{
    if (masterId_ != NODEID_NONE) {
        // Stop active D2D flows (go back to Infrastructure mode).
        // Currently, DM is possible only for UEs served by the same cell.

        // Trigger D2D mode switch.
        cModule *enb = getSimulation()->getModule(binder_->getOmnetId(masterId_));
        D2DModeSelectionBase *d2dModeSelection = check_and_cast<D2DModeSelectionBase *>(enb->getSubmodule("cellularNic")->getSubmodule("d2dModeSelection"));
        d2dModeSelection->doModeSwitchAtHandover(nodeId_, false);
    }
    LtePhyUe::triggerHandover();
}

void LtePhyUeD2D::doHandover()
{
    // AMC calls.
    if (masterId_ != NODEID_NONE) {
        LteAmc *oldAmc = getAmcModule(masterId_);
        oldAmc->detachUser(nodeId_, D2D);
    }

    if (candidateMasterId_ != NODEID_NONE) {
        LteAmc *newAmc = getAmcModule(candidateMasterId_);
        assert(newAmc != nullptr);
        newAmc->attachUser(nodeId_, D2D);
    }

    LtePhyUe::doHandover();

    if (candidateMasterId_ != NODEID_NONE) {
        // Send a self-message to schedule the possible mode switch at the end of the TTI (after all UEs have performed the handover).
        cMessage *msg = new cMessage("doModeSwitchAtHandover");
        msg->setSchedulingPriority(10);
        scheduleAt(NOW, msg);
    }
}

void LtePhyUeD2D::handleUpperMessage(cMessage *msg)
{
    auto pkt = check_and_cast<inet::Packet *>(msg);
    auto lteInfo = pkt->removeTag<UserControlInfo>();

    double carrierFreq = lteInfo->getCarrierFrequency();
    LteChannelModel *channelModel = getChannelModel(carrierFreq);
    if (channelModel == nullptr)
        throw cRuntimeError("LtePhyUeD2D::handleUpperMessage - Carrier frequency [%f] not supported by any channel model", carrierFreq);

    if (lteInfo->getFrameType() == DATAPKT && (channelModel->isUplinkInterferenceEnabled() || channelModel->isD2DInterferenceEnabled())) {
        // Store the RBs used for data transmission to the binder (for UL interference computation).
        RbMap rbMap = lteInfo->getGrantedBlocks();
        Remote antenna = MACRO;  // TODO fix for multi-antenna.
        Direction dir = (Direction)lteInfo->getDirection();
        binder_->storeUlTransmissionMap(channelModel->getCarrierFrequency(), antenna, rbMap, nodeId_, mac_->getMacCellId(), this, dir);
    }

    if (lteInfo->getFrameType() == DATAPKT && lteInfo->getUserTxParams() != nullptr) {
        double cqi = lteInfo->getUserTxParams()->readCqiVector()[lteInfo->getCw()];
        if (lteInfo->getDirection() == UL) {
            emit(averageCqiUlSignal_, cqi);
            recordCqi(cqi, UL);
        }
        else if (lteInfo->getDirection() == D2D || lteInfo->getDirection() == D2D_MULTI)
            emit(averageCqiD2DSignal_, cqi);
    }

    EV << NOW << " LtePhyUeD2D::handleUpperMessage - message from stack" << endl;
    LteAirFrame *frame = nullptr;

    if (lteInfo->getFrameType() == HARQPKT || lteInfo->getFrameType() == GRANTPKT || lteInfo->getFrameType() == RACPKT) {
        frame = new LteAirFrame("harqFeedback-grant");
    }
    else {
        // Create LteAirFrame and encapsulate the received packet.
        frame = new LteAirFrame("airframe");
    }

    frame->encapsulate(check_and_cast<cPacket *>(msg));

    // Initialize frame fields.

    frame->setSchedulingPriority(airFramePriority_);

    // Set transmission duration according to the numerology.
    NumerologyIndex numerologyIndex = binder_->getNumerologyIndexFromCarrierFreq(lteInfo->getCarrierFrequency());
    double slotDuration = binder_->getSlotDurationFromNumerologyIndex(numerologyIndex);
    frame->setDuration(slotDuration);

    // Set current position.
    lteInfo->setCoord(getRadioPosition());

    lteInfo->setTxPower(txPower_);
    lteInfo->setD2dTxPower(d2dTxPower_);
    frame->setControlInfo(lteInfo.get()->dup());

    EV << "LtePhyUeD2D::handleUpperMessage - " << nodeTypeToA(nodeType_) << " with id " << nodeId_
       << " sending message to the air channel. Dest=" << lteInfo->getDestId() << endl;

    // If this is a multicast/broadcast connection, send the frame to all neighbors in the hearing range.
    // Otherwise, send unicast to the destination.
    if (lteInfo->getDirection() == D2D_MULTI)
        sendMulticast(frame);
    else
        sendUnicast(frame);
}

void LtePhyUeD2D::storeAirFrame(LteAirFrame *newFrame)
{
    // Implements the capture effect
    // Store the frame received from the nearest transmitter
    UserControlInfo *newInfo = check_and_cast<UserControlInfo *>(newFrame->getControlInfo());
    double carrierFreq = newInfo->getCarrierFrequency();
    LteChannelModel *channelModel = getChannelModel(carrierFreq);
    if (channelModel == nullptr)
        throw cRuntimeError("LtePhyUeD2D::storeAirFrame - Carrier frequency [%f] not supported by any channel model", carrierFreq);

    Coord myCoord = getCoord();
    double distance = 0.0;
    double rsrpMean = 0.0;
    std::vector<double> rsrpVector;
    bool useRsrp = false;

    if (strcmp(par("d2dMulticastCaptureEffectFactor"), "RSRP") == 0) {
        useRsrp = true;

        double sum = 0.0;
        unsigned int allocatedRbs = 0;
        rsrpVector = channelModel->getRSRP_D2D(newFrame, newInfo, nodeId_, myCoord);

        // Get the average RSRP on the RBs allocated for the transmission
        RbMap rbmap = newInfo->getGrantedBlocks();
        // For each Remote unit used to transmit the packet
        for (const auto &[remoteUnit, rbList] : rbmap) {
            // For each logical band used to transmit the packet
            for (const auto &[band, allocation] : rbList) {
                if (allocation == 0) // This Rb is not allocated
                    continue;

                sum += rsrpVector.at(band);
                allocatedRbs++;
            }
        }
        if (allocatedRbs > 0)
            rsrpMean = sum / allocatedRbs;
        EV << NOW << " LtePhyUeD2D::storeAirFrame - Average RSRP from node " << newInfo->getSourceId() << ": " << rsrpMean << endl;
    }
    else { // Distance
        Coord newSenderCoord = newInfo->getCoord();
        distance = myCoord.distance(newSenderCoord);
        EV << NOW << " LtePhyUeD2D::storeAirFrame - Distance from node " << newInfo->getSourceId() << ": " << distance << endl;
    }

    if (!d2dReceivedFrames_.empty()) {
        LteAirFrame *prevFrame = d2dReceivedFrames_.front();
        if (!useRsrp && distance < nearestDistance_) {
            EV << "[ < nearestDistance: " << nearestDistance_ << "]" << endl;

            // Remove the previous frame
            d2dReceivedFrames_.pop_back();
            delete prevFrame;

            nearestDistance_ = distance;
            d2dReceivedFrames_.push_back(newFrame);
        }
        else if (rsrpMean > bestRsrpMean_) {
            EV << "[ > bestRsrp: " << bestRsrpMean_ << "]" << endl;

            // Remove the previous frame
            d2dReceivedFrames_.pop_back();
            delete prevFrame;

            bestRsrpMean_ = rsrpMean;
            bestRsrpVector_ = rsrpVector;
            d2dReceivedFrames_.push_back(newFrame);
        }
        else {
            // This frame will not be decoded
            delete newFrame;
        }
    }
    else {
        if (!useRsrp) {
            nearestDistance_ = distance;
            d2dReceivedFrames_.push_back(newFrame);
        }
        else {
            bestRsrpMean_ = rsrpMean;
            bestRsrpVector_ = rsrpVector;
            d2dReceivedFrames_.push_back(newFrame);
        }
    }
}

LteAirFrame *LtePhyUeD2D::extractAirFrame()
{
    // Implements the capture effect
    // The vector is storing the frame received from the strongest/nearest transmitter

    return d2dReceivedFrames_.front();
}

void LtePhyUeD2D::decodeAirFrame(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    EV << NOW << " LtePhyUeD2D::decodeAirFrame - Start decoding..." << endl;

    double carrierFreq = lteInfo->getCarrierFrequency();
    LteChannelModel *channelModel = getChannelModel(carrierFreq);
    if (channelModel == nullptr)
        throw cRuntimeError("LtePhyUeD2D::decodeAirFrame - Carrier frequency [%f] not supported by any channel model", carrierFreq);

    // Apply decider to received packet
    bool result = true;

    RemoteSet r = lteInfo->getUserTxParams()->readAntennaSet();
    if (r.size() > 1) {
        // DAS
        for (auto it : r) {
            EV << "LtePhyUeD2D::decodeAirFrame: Receiving Packet from antenna " << it << "\n";

            /*
             * On UE set the sender position
             * and tx power to the sender DAS antenna
             */

            RemoteUnitPhyData data;
            data.txPower = lteInfo->getTxPower();
            data.m = getRadioPosition();
            frame->addRemoteUnitPhyDataVector(data);
        }
        // Apply analog models for DAS
        result = channelModel->isErrorDas(frame, lteInfo);
    }
    else {
        if (lteInfo->getDirection() == D2D_MULTI)
            result = channelModel->isError_D2D(frame, lteInfo, bestRsrpVector_);
        else
            result = channelModel->isError(frame, lteInfo);
    }

    // Update statistics
    if (result)
        numAirFrameReceived_++;
    else
        numAirFrameNotReceived_++;

    EV << "Handled LteAirframe with ID " << frame->getId() << " with result "
       << (result ? "RECEIVED" : "NOT RECEIVED") << endl;

    auto pkt = check_and_cast<inet::Packet *>(frame->decapsulate());

    // Note: no need to delete the frame itself - will be deleted later when the buffer of
    // received frames is cleared

    // Attach the decider result to the packet as control info
    lteInfo->setDeciderResult(result);
    *(pkt->addTagIfAbsent<UserControlInfo>()) = *lteInfo;
    delete lteInfo;

    // Send decapsulated message along with result control info to upperGateOut_
    send(pkt, upperGateOut_);

    if (getEnvir()->isGUI())
        updateDisplayString();
}

void LtePhyUeD2D::sendFeedback(LteFeedbackDoubleVector fbDl, LteFeedbackDoubleVector fbUl, FeedbackRequest req)
{
    Enter_Method("SendFeedback");
    EV << "LtePhyUeD2D: feedback from Feedback Generator" << endl;

    // Create a feedback packet
    auto fbPkt = makeShared<LteFeedbackPkt>();
    // Set the feedback
    fbPkt->setLteFeedbackDoubleVectorDl(fbDl);
    fbPkt->setLteFeedbackDoubleVectorUl(fbUl);
    fbPkt->setSourceNodeId(nodeId_);

    auto pkt = new Packet("feedback_pkt");
    pkt->insertAtFront(fbPkt);

    UserControlInfo *uinfo = new UserControlInfo();
    uinfo->setSourceId(nodeId_);
    uinfo->setDestId(masterId_);
    uinfo->setFrameType(FEEDBACKPKT);
    uinfo->setIsCorruptible(false);
    // Create LteAirFrame and encapsulate a feedback packet
    LteAirFrame *frame = new LteAirFrame("feedback_pkt");
    frame->encapsulate(check_and_cast<cPacket *>(pkt));
    uinfo->feedbackReq = req;
    uinfo->setDirection(UL);
    simtime_t signalLength = TTI;
    uinfo->setTxPower(txPower_);
    uinfo->setD2dTxPower(d2dTxPower_);
    // Initialize frame fields

    frame->setSchedulingPriority(airFramePriority_);
    frame->setDuration(signalLength);

    uinfo->setCoord(getRadioPosition());

    lastFeedback_ = NOW;

    // Send one feedback packet for each carrier
    for (auto& cm : channelModel_) {
        double carrierFrequency = cm.first;
        LteAirFrame *carrierFrame = frame->dup();
        UserControlInfo *carrierInfo = uinfo->dup();
        carrierInfo->setCarrierFrequency(carrierFrequency);
        carrierFrame->setControlInfo(carrierInfo);

        EV << "LtePhy: " << nodeTypeToA(nodeType_) << " with id "
           << nodeId_ << " sending feedback to the air channel for carrier " << carrierFrequency << endl;
        sendUnicast(carrierFrame);
    }

    delete frame;
    delete uinfo;
}

void LtePhyUeD2D::finish()
{
    if (getSimulation()->getSimulationStage() != CTX_FINISH) {
        // Do this only at deletion of the module during the simulation

        // AMC calls
        LteAmc *amc = getAmcModule(masterId_);
        if (amc != nullptr)
            amc->detachUser(nodeId_, D2D);

        LtePhyUe::finish();
    }
}

} //namespace

