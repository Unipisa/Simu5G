//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include <assert.h>
#include "simu5g/stack/phy/LtePhyUe.h"

#include "simu5g/stack/rrc/HandoverController.h"
#include "simu5g/stack/phy/NrPhyUe.h"
#include "simu5g/stack/ip2nic/Ip2Nic.h"
#include "simu5g/stack/mac/LteMacEnb.h"
#include "simu5g/stack/phy/packet/LteFeedbackPkt.h"
#include "simu5g/stack/phy/feedback/LteDlFeedbackGenerator.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

Define_Module(LtePhyUe);

using namespace inet;

simsignal_t LtePhyUe::distanceSignal_ = registerSignal("distance");

LtePhyUe::~LtePhyUe()
{
}

void LtePhyUe::initialize(int stage)
{
    LtePhyBase::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        isNr_ = false;        // this might be true only if this module is a UeNrPhy
        nodeType_ = UE;

        if (!hasListeners(averageCqiDlSignal_))
            throw cRuntimeError("no phy listeners");

        WATCH(nodeType_);
        WATCH(servingNodeId_);

        txPower_ = ueTxPower_;

        // get the reference to the MAC module
        mac_ = check_and_cast<LteMacUe *>(gate(upperGateOut_)->getPathEndGate()->getOwnerModule());

        handoverController_.reference(this, "handoverControllerModule", true);

        handoverController_->setPhy(this);

        // setting isNr_ was originally done in the NrPhyUe subclass, but it is needed here
        isNr_ = dynamic_cast<NrPhyUe*>(this) && strcmp(getFullName(), "nrPhy") == 0;

        // get local id
        nodeId_ = MacNodeId(hostModule->par(isNr_ ? "nrMacNodeId" : "macNodeId").intValue());
        EV << "Local MacNodeId: " << nodeId_ << endl;
    }
    else if (stage == INITSTAGE_SIMU5G_CELLINFO_CHANNELUPDATE) { //TODO being fwd, eliminate stage
        // get cellInfo at this stage because the next hop of the node is registered in the Ip2Nic module at the INITSTAGE_SIMU5G_NETWORK_LAYER
        if (servingNodeId_ != NODEID_NONE)
            cellInfo_ = binder_->getCellInfoByNodeId(nodeId_);
        else
            cellInfo_ = nullptr;
    }
}

void LtePhyUe::findCandidateEnb(MacNodeId& outCandidateMasterId, double& outCandidateMasterRssi)
{
    // this is a fictitious frame that needs to compute the SINR
    LteAirFrame *frame = new LteAirFrame("cellSelectionFrame");
    UserControlInfo *cInfo = new UserControlInfo();
    outCandidateMasterId = NODEID_NONE;

    // get the list of all eNodeBs in the network
    for (const auto &enbInfo : binder_->getEnbList()) {
        // the NR phy layer only checks signal from gNBs, and
        // the LTE phy layer only checks signal from eNBs
        if (isNr_ != enbInfo->isNr)
            continue;

        MacNodeId cellId = enbInfo->id;
        LtePhyBase *cellPhy = check_and_cast<LtePhyBase*>(
                enbInfo->eNodeB->getSubmodule("cellularNic")->getSubmodule("phy"));
        double cellTxPower = cellPhy->getTxPwr();
        Coord cellPos = cellPhy->getCoord();
        // check whether the BS supports the carrier frequency used by the UE
        GHz ueCarrierFrequency = primaryChannelModel_->getCarrierFrequency();
        LteChannelModel *cellChannelModel = cellPhy->getChannelModel(ueCarrierFrequency);
        if (cellChannelModel == nullptr)
            continue;

        // build a control info
        cInfo->setSourceId(cellId);
        cInfo->setTxPower(cellTxPower);
        cInfo->setCoord(cellPos);
        cInfo->setFrameType(BROADCASTPKT);
        cInfo->setDirection(DL);
        // get RSSI from the BS
        double rssi = 0;
        std::vector<double> rssiV = primaryChannelModel_->getRSRP(frame, cInfo);
        for (auto value : rssiV)
            rssi += value;
        rssi /= rssiV.size(); // compute the mean over all RBs
        EV << "LtePhyUe::findCandicateEnb - RSSI from cell " << cellId << ": " << rssi << " dB (current candidate cell " << outCandidateMasterId << ": " << outCandidateMasterRssi << " dB)" << endl;
        if (outCandidateMasterId == NODEID_NONE || rssi > outCandidateMasterRssi) {
            outCandidateMasterId = cellId;
            outCandidateMasterRssi = rssi;
        }
    }
    delete cInfo;
    delete frame;
}

void LtePhyUe::handleSelfMessage(cMessage *msg)
{
    // no local timers
}

void LtePhyUe::changeServingNode(MacNodeId servingNodeId)
{
    MacNodeId oldServingNode = servingNodeId_;
    servingNodeId_ = servingNodeId;

    // update reference to master node's mobility module
    if (servingNodeId_ == NODEID_NONE)
        servingNodeMobility_ = nullptr;
    else {
        cModule *servingNodeModule = binder_->getModuleByMacNodeId(servingNodeId_);
        servingNodeMobility_ = check_and_cast<IMobility *>(servingNodeModule->getSubmodule("mobility"));
    }

    // update cellInfo
    if (oldServingNode != NODEID_NONE)
        cellInfo_->detachUser(nodeId_);

    if (servingNodeId_ != NODEID_NONE) {
        LteMacEnb *newMacEnb = check_and_cast<LteMacEnb *>(binder_->getMacByNodeId(servingNodeId_));
        cellInfo_ = newMacEnb->getCellInfo();
        cellInfo_->attachUser(nodeId_);
    }

}

double LtePhyUe::computeReceivedBeaconPacketRssi(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    std::vector<double> rssiV = primaryChannelModel_->getSINR(frame, lteInfo);
    double rssi = 0;
    for (auto value : rssiV)
        rssi += value;
    rssi /= rssiV.size();
    return rssi;
}

// TODO: ***reorganize*** method
void LtePhyUe::handleAirFrame(cMessage *msg)
{
    LteAirFrame *frame = static_cast<LteAirFrame *>(msg);
    UserControlInfo *lteInfo = new UserControlInfo(frame->getAdditionalInfo());

    connectedNodeId_ = servingNodeId_;
    EV << "LtePhy: received new LteAirFrame with ID " << frame->getId() << " from channel" << endl;

    if (!binder_->nodeExists(lteInfo->getSourceId())) {
        // source has left the simulation
        delete msg;
        return;
    }

    // check if the air frame was sent on a correct carrier frequency
    GHz carrierFreq = lteInfo->getCarrierFrequency();
    LteChannelModel *channelModel = getChannelModel(carrierFreq);
    if (channelModel == nullptr) {
        EV << "Received packet on carrier frequency not supported by this node. Delete it." << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    //Update coordinates of this user
    if (lteInfo->getFrameType() == BEACONPKT) {
        // Check if the message is on another carrier frequency
        if (carrierFreq != primaryChannelModel_->getCarrierFrequency()) {
            EV << "Received beacon packet on a different carrier frequency than the primary cell. Delete it." << endl;
            delete lteInfo;
            delete frame;
            return;
        }

        handoverController_->beaconReceived(frame, lteInfo);
        return;
    }

    // Check if the frame is for us ( MacNodeId matches )
    if (lteInfo->getDestId() != nodeId_) {
        EV << "ERROR: Frame is not for us. Delete it." << endl;
        EV << "Packet Type: " << phyFrameTypeToA((LtePhyFrameType)lteInfo->getFrameType()) << endl;
        EV << "Frame MacNodeId: " << lteInfo->getDestId() << endl;
        EV << "Local MacNodeId: " << nodeId_ << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    /*
     * This could happen if the ue associates with a new master while the old one
     * already scheduled a packet for him: the packet is in the air while the ue changes master.
     * Event timing:      TTI x: packet scheduled and sent for UE (tx time = 1ms)
     *                     TTI x+0.1: ue changes master
     *                     TTI x+1: packet from old master arrives at ue
     */
    if (lteInfo->getSourceId() != servingNodeId_) {
        EV << "WARNING: frame from an old master during handover: deleted " << endl;
        EV << "Source MacNodeId: " << lteInfo->getSourceId() << endl;
        EV << "Master MacNodeId: " << servingNodeId_ << endl;
        delete frame;
        return;
    }

    // send H-ARQ feedback up
    if (lteInfo->getFrameType() == HARQPKT || lteInfo->getFrameType() == GRANTPKT || lteInfo->getFrameType() == RACPKT) {
        handleControlMsg(frame, lteInfo);
        return;
    }
    if ((lteInfo->getUserTxParams()) != nullptr) {
        int cw = lteInfo->getCw();
        if (lteInfo->getUserTxParams()->readCqiVector().size() == 1)
            cw = 0;
        double cqi = lteInfo->getUserTxParams()->readCqiVector()[cw];
        emit(averageCqiDlSignal_, cqi);
        recordCqi(cqi, DL);
    }
    // apply decider to received packet (DAS removed - single antenna only)
    bool result = channelModel->isReceptionSuccessful(frame, lteInfo);

    // update statistics
    if (result)
        numAirFrameReceived_++;
    else
        numAirFrameNotReceived_++;

    EV << "Handled LteAirframe with ID " << frame->getId() << " with result "
       << (result ? "RECEIVED" : "NOT RECEIVED") << endl;

    auto pkt = check_and_cast<inet::Packet *>(frame->decapsulate());

    // here frame has to be destroyed since it is no longer useful
    delete frame;

    // attach the decider result to the packet as control info
    *(pkt->addTagIfAbsent<UserControlInfo>()) = *lteInfo;
    delete lteInfo;

    pkt->addTagIfAbsent<PhyReceptionInd>()->setDeciderResult(result);

    // send decapsulated message along with result control info to upperGateOut_
    send(pkt, upperGateOut_);

    if (getEnvir()->isGUI())
        updateDisplayString();
}

void LtePhyUe::handleUpperMessage(cMessage *msg)
{
    auto pkt = check_and_cast<inet::Packet *>(msg);
    auto lteInfo = pkt->getTag<UserControlInfo>();

    MacNodeId dest = lteInfo->getDestId();
    if (dest != servingNodeId_) {
        // UE is not sending to its master!!
        throw cRuntimeError("LtePhyUe::handleUpperMessage  Ue preparing to send message to %hu instead of its master (%hu)", num(dest), num(servingNodeId_));
    }

    GHz carrierFreq = lteInfo->getCarrierFrequency();
    LteChannelModel *channelModel = getChannelModel(carrierFreq);
    if (channelModel == nullptr)
        throw cRuntimeError("LtePhyUe::handleUpperMessage - Carrier frequency [%f] not supported by any channel model", carrierFreq.get());

    if (lteInfo->getFrameType() == DATAPKT && (channelModel->isUplinkInterferenceEnabled() || channelModel->isD2DInterferenceEnabled())) {
        // Store the RBs used for data transmission to the binder (for UL interference computation)
        RbMap rbMap = lteInfo->getGrantedBlocks();
        Remote antenna = MACRO;  // TODO fix for multi-antenna
        binder_->storeUlTransmissionMap(channelModel->getCarrierFrequency(), antenna, rbMap, nodeId_, mac_->getMacCellId(), this, UL);
    }

    if (lteInfo->getFrameType() == DATAPKT && lteInfo->getUserTxParams() != nullptr) {
        double cqi = lteInfo->getUserTxParams()->readCqiVector()[lteInfo->getCw()];
        if (lteInfo->getDirection() == UL) {
            emit(averageCqiUlSignal_, cqi);
            recordCqi(cqi, UL);
        }
        else if (lteInfo->getDirection() == D2D)
            emit(averageCqiD2DSignal_, cqi);
    }

    LtePhyBase::handleUpperMessage(msg);
}

void LtePhyUe::emitMobilityStats()
{
    if (servingNodeMobility_) {
        // emit distance from current serving cell
        inet::Coord masterPos = servingNodeMobility_->getCurrentPosition();
        double distance = getRadioPosition().distance(masterPos);
        emit(distanceSignal_, distance);
    }
}

void LtePhyUe::sendFeedback(LteFeedbackDoubleVector fbDl, LteFeedbackDoubleVector fbUl, FeedbackRequest req)
{
    Enter_Method("SendFeedback");
    EV << "LtePhyUe: feedback from Feedback Generator" << endl;

    //Create a feedback packet
    auto fbPkt = makeShared<LteFeedbackPkt>();
    //Set the feedback
    fbPkt->setLteFeedbackDoubleVectorDl(fbDl);
    fbPkt->setLteFeedbackDoubleVectorDl(fbUl);
    fbPkt->setSourceNodeId(nodeId_);

    auto pkt = new Packet("feedback_pkt");
    pkt->insertAtFront(fbPkt);

    UserControlInfo *uinfo = new UserControlInfo();
    uinfo->setSourceId(nodeId_);
    uinfo->setDestId(servingNodeId_);
    uinfo->setFrameType(FEEDBACKPKT);
    // create LteAirFrame and encapsulate a feedback packet
    LteAirFrame *frame = new LteAirFrame("feedback_pkt");
    frame->encapsulate(check_and_cast<cPacket *>(pkt));
    uinfo->setFeedbackReq(req);
    uinfo->setDirection(UL);
    simtime_t signalLength = TTI;
    uinfo->setTxPower(txPower_);
    // initialize frame fields

    frame->setSchedulingPriority(airFramePriority_);
    frame->setDuration(signalLength);

    uinfo->setCoord(getRadioPosition());

    //TODO access speed data Update channel index
    lastFeedback_ = NOW;

    // send one feedback packet for each carrier
    for (auto& cm : channelModel_) {
        GHz carrierFrequency = cm.first;
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

void LtePhyUe::recordCqi(unsigned int sample, Direction dir)
{
    if (dir == DL) {
        cqiDlSamples_.push_back(sample);
        cqiDlSum_ += sample;
        cqiDlCount_++;
    }
    if (dir == UL) {
        cqiUlSamples_.push_back(sample);
        cqiUlSum_ += sample;
        cqiUlCount_++;
    }
}

double LtePhyUe::getAverageCqi(Direction dir)
{
    if (dir == DL) {
        if (cqiDlCount_ == 0)
            return 0;
        return (double)cqiDlSum_ / cqiDlCount_;
    }
    if (dir == UL) {
        if (cqiUlCount_ == 0)
            return 0;
        return (double)cqiUlSum_ / cqiUlCount_;
    }
    throw cRuntimeError("Direction %d is not handled.", dir);
}

double LtePhyUe::getVarianceCqi(Direction dir)
{
    double avgCqi = getAverageCqi(dir);
    double err, sum = 0;

    if (dir == DL) {
        for (short & cqiDlSample : cqiDlSamples_) {
            err = avgCqi - cqiDlSample;
            sum += (err * err);
        }
        return sum / cqiDlSamples_.size();
    }
    if (dir == UL) {
        for (short & cqiUlSample : cqiUlSamples_) {
            err = avgCqi - cqiUlSample;
            sum += (err * err);
        }
        return sum / cqiUlSamples_.size();
    }
    throw cRuntimeError("Direction %d is not handled.", dir);
}

void LtePhyUe::finish()
{
    if (getSimulation()->getSimulationStage() != CTX_FINISH) {
        // do this only during the deletion of the module during the simulation, and
        // this PHY layer is connected to a serving base station
        if (servingNodeId_ != NODEID_NONE) {
            cellInfo_->detachUser(nodeId_);
        }
    }
}

} //namespace
