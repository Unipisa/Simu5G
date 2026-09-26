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
#include "simu5g/stack/phy/PhyUe.h"

#include "../ip2nic/HandoverPacketHolderUe.h"
#include "simu5g/stack/rrc/ConnectionControlUe.h"
#include "simu5g/stack/mac/LteMacEnb.h"
#include "simu5g/stack/phy/packet/LteFeedbackPkt.h"
#include "simu5g/stack/phy/feedback/LteDlFeedbackGenerator.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

Define_Module(PhyUe);

using namespace inet;

simsignal_t PhyUe::distanceSignal_ = registerSignal("distance");

PhyUe::~PhyUe()
{
}

void PhyUe::initialize(int stage)
{
    PhyBase::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        nodeType_ = UE;

        if (!hasListeners(averageCqiDlSignal_))
            throw cRuntimeError("no phy listeners");

        WATCH(servingNodeId_);

        txPower_ = ueTxPower_;

        connectionControl_.reference(this, "connectionControlModule", true);
        connectionControl_->setPhy(this);

        // get local id
        nodeId_ = MacNodeId(par("macNodeId").intValue());
        EV << "Local MacNodeId: " << nodeId_ << endl;
    }
    else if (stage == INITSTAGE_SIMU5G_CELLINFO_CHANNELUPDATE) { //TODO being fwd, eliminate stage
        // get cellInfo at this stage because the next hop of the node is registered in the HandoverPacketHolder module at the INITSTAGE_SIMU5G_NETWORK_LAYER
        if (servingNodeId_ != NODEID_NONE)
            cellInfo_ = binder_->getCellInfoByNodeId(nodeId_);
        else
            cellInfo_ = nullptr;
    }
}

void PhyUe::findCandidateEnb(MacNodeId& outCandidateMasterId, double& outCandidateMasterRssi)
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
        PhyBase *cellPhy = check_and_cast<PhyBase*>(
                enbInfo->eNodeB->getSubmodule("cellularNic")->getSubmodule("phy"));
        double cellTxPower = cellPhy->getTxPwr();
        Coord cellPos = cellPhy->getCoord();
        // check whether the BS supports the carrier frequency used by the UE
        GHz ueCarrierFrequency = primaryChannelModel_->getCarrierFrequency();
        ChannelModelBase *cellChannelModel = cellPhy->getChannelModel(ueCarrierFrequency);
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
        EV << "PhyUe::findCandicateEnb - RSSI from cell " << cellId << ": " << rssi << " dB (current candidate cell " << outCandidateMasterId << ": " << outCandidateMasterRssi << " dB)" << endl;
        if (outCandidateMasterId == NODEID_NONE || rssi > outCandidateMasterRssi) {
            outCandidateMasterId = cellId;
            outCandidateMasterRssi = rssi;
        }
    }
    delete cInfo;
    delete frame;
}

void PhyUe::handleSelfMessage(cMessage *msg)
{
    // no local timers
}

void PhyUe::changeServingNode(MacNodeId servingNodeId)
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

double PhyUe::computeReceivedBeaconPacketRssi(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    std::vector<double> rssiV = primaryChannelModel_->getSINR(frame, lteInfo);
    double rssi = 0;
    for (auto value : rssiV)
        rssi += value;
    rssi /= rssiV.size();
    return rssi;
}

// TODO: ***reorganize*** method
void PhyUe::handleAirFrame(cMessage *msg)
{
    LteAirFrame *frame = static_cast<LteAirFrame *>(msg);
    UserControlInfo *lteInfo = new UserControlInfo(frame->getAdditionalInfo());

    EV << "PhyUe: received new LteAirFrame with ID " << frame->getId() << " from channel" << endl;

    MacNodeId sourceId = lteInfo->getSourceId();
    if (!binder_->nodeExists(sourceId)) {
        // The source has left the simulation
        delete msg;
        return;
    }

    GHz carrierFreq = lteInfo->getCarrierFrequency();
    ChannelModelBase *channelModel = getChannelModel(carrierFreq);
    if (channelModel == nullptr) {
        EV << "Received packet on carrier frequency not supported by this node. Delete it." << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    // Update coordinates of this user
    if (lteInfo->getFrameType() == BEACONPKT) {
        // Check if the message is on another carrier frequency
        if (carrierFreq != primaryChannelModel_->getCarrierFrequency()) {
            EV << "Received beacon packet on a different carrier frequency. Delete it." << endl;
            delete lteInfo;
            delete frame;
            return;
        }

        // Check if the message is from a different cellular technology.
        // Note: beacons are the only frames that carry a meaningful isNr flag (it is stamped
        // solely in PhyEnb::createBeaconMessage()), and the only true channel broadcasts
        // reaching both radios of a dual-PHY UE; non-beacon frames are technology-routed at
        // the sender. Hence the filter is scoped to beacons.
        if (lteInfo->isNr() != isNr_) {
            EV << "Received beacon packet [from NR=" << lteInfo->isNr() << "] from a different radio technology [to NR=" << isNr_ << "]. Delete it." << endl;
            delete lteInfo;
            delete frame;
            return;
        }

        connectionControl_->beaconReceived(frame, lteInfo);
        return;
    }

    // Check if the frame is for us ( MacNodeId matches or - if this is a multicast communication - enrolled in multicast group)
    if (lteInfo->getDestId() != nodeId_ && !(binder_->isInMulticastGroup(nodeId_, lteInfo->getPacketMulticastGroupId()))) {
        EV << "ERROR: Frame is not for us. Delete it." << endl;
        EV << "Packet Type: " << phyFrameTypeToA((LtePhyFrameType)lteInfo->getFrameType()) << endl;
        EV << "Frame MacNodeId: " << lteInfo->getDestId() << endl;
        EV << "Local MacNodeId: " << nodeId_ << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    /*
     * This could happen if the UE associates with a new master while a packet from the
     * old master is in-flight: the packet is in the air
     * while the UE changes master.
     * Event timing:      TTI x: packet scheduled and sent for the UE (tx time = 1ms)
     *                     TTI x+0.1: UE changes master
     *                     TTI x+1: packet from the old master arrives at the UE
     */
    if (isStaleFrame(lteInfo)) {
        EV << "WARNING: Frame from an old master during handover: deleted " << endl;
        EV << "Source MacNodeId: " << lteInfo->getSourceId() << endl;
        EV << "Master MacNodeId: " << servingNodeId_ << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    // D2D-aware subclasses rewrite the multicast destination here
    frameAccepted(lteInfo);

    // Send H-ARQ feedback and other control messages up
    if (isControlFrameType((LtePhyFrameType)lteInfo->getFrameType())) {
        handleControlMsg(frame, lteInfo);
        return;
    }

    // This is a DATA packet

    if (servingNodeId_ == NODEID_NONE) {
        // UE is not (anymore) associated with any eNB/gNB and all harqBuffers are already deleted.
        // Handing this data packet to the MAC layer will lead to null pointers.
        // (Matters for D2D/D2D_MULTI DATA in-flight during the mid-handover detachment window.)
        EV << "PhyUe: UE " << nodeId_ << " received data packet while not associated with any base station. Drop it." << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    // D2D-aware subclasses store multicast frames for end-of-TTI decoding (capture effect)
    if (interceptIncomingFrame(frame, lteInfo))
        return;

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

    bool result = channelModel->isReceptionSuccessful(frame, lteInfo);

    // Update statistics
    if (result)
        numAirFrameReceived_++;
    else
        numAirFrameNotReceived_++;

    EV << "Handled LteAirframe with ID " << frame->getId() << " with result "
       << (result ? "RECEIVED" : "NOT RECEIVED") << endl;

    auto pkt = check_and_cast<inet::Packet *>(frame->decapsulate());

    // Here frame has to be destroyed since it is no more useful
    delete frame;

    // Attach the decider result to the packet as control info
    *(pkt->addTagIfAbsent<UserControlInfo>()) = *lteInfo;
    delete lteInfo;

    pkt->addTagIfAbsent<PhyReceptionInd>()->setDeciderResult(result);

    // Send decapsulated message along with result control info to upperGateOut_
    send(pkt, upperGateOut_);

    if (getEnvir()->isGUI())
        updateDisplayString();
}

void PhyUe::validateOutgoingFrame(const UserControlInfo *info)
{
    MacNodeId dest = info->getDestId();
    if (dest != servingNodeId_) {
        // UE is not sending to its master!!
        throw cRuntimeError("PhyUe::validateOutgoingFrame  Ue preparing to send message to %hu instead of its master (%hu)", num(dest), num(servingNodeId_));
    }
}

void PhyUe::handleUpperMessage(cMessage *msg)
{
    auto pkt = check_and_cast<inet::Packet *>(msg);
    auto lteInfo = pkt->getTag<UserControlInfo>();

    validateOutgoingFrame(lteInfo.get());

    GHz carrierFreq = lteInfo->getCarrierFrequency();
    ChannelModelBase *channelModel = getChannelModel(carrierFreq);
    if (channelModel == nullptr)
        throw cRuntimeError("PhyUe::handleUpperMessage - Carrier frequency [%f] not supported by any channel model", carrierFreq.get());

    if (lteInfo->getFrameType() == DATAPKT && channelModel->recordsUlTransmissionMap()) {
        // Store the RBs used for data transmission to the binder (for UL interference computation)
        RbMap rbMap = lteInfo->getGrantedBlocks();
        Remote antenna = MACRO;  // TODO fix for multi-antenna
        // note: the direction is always UL here for a plain UE (enforced by validateOutgoingFrame() above)
        binder_->storeUlTransmissionMap(channelModel->getCarrierFrequency(), antenna, rbMap, nodeId_, servingNodeId_, this, (Direction)lteInfo->getDirection());
    }

    if (lteInfo->getFrameType() == DATAPKT && lteInfo->getUserTxParams() != nullptr) {
        double cqi = lteInfo->getUserTxParams()->readCqiVector()[lteInfo->getCw()];
        if (lteInfo->getDirection() == UL) {
            emit(averageCqiUlSignal_, cqi);
            recordCqi(cqi, UL);
        }
        else
            recordExtraTxCqi(cqi, lteInfo.get());
    }

    PhyBase::handleUpperMessage(msg);
}

void PhyUe::emitMobilityStats()
{
    if (servingNodeMobility_) {
        // emit distance from current serving cell
        inet::Coord masterPos = servingNodeMobility_->getCurrentPosition();
        double distance = getRadioPosition().distance(masterPos);
        emit(distanceSignal_, distance);
    }
}

void PhyUe::sendFeedback(LteFeedbackDoubleVector fbDl, LteFeedbackDoubleVector fbUl, FeedbackRequest req)
{
    Enter_Method("SendFeedback");
    EV << "PhyUe: feedback from Feedback Generator" << endl;

    //Create a feedback packet
    auto fbPkt = makeShared<LteFeedbackPkt>();
    //Set the feedback
    fbPkt->setLteFeedbackDoubleVectorDl(fbDl);
    fbPkt->setLteFeedbackDoubleVectorUl(fbUl);
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
    stampExtraTxControlInfo(uinfo);
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

        EV << "Phy: " << nodeTypeToA(nodeType_) << " with id "
           << nodeId_ << " sending feedback to the air channel for carrier " << carrierFrequency << endl;
        sendUnicast(carrierFrame);
    }

    delete frame;
    delete uinfo;
}

void PhyUe::recordCqi(unsigned int sample, Direction dir)
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

double PhyUe::getAverageCqi(Direction dir)
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

double PhyUe::getVarianceCqi(Direction dir)
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

void PhyUe::finish()
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
