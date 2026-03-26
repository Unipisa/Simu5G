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
simsignal_t LtePhyUe::servingCellSignal_ = registerSignal("servingCell");

LtePhyUe::~LtePhyUe()
{
    cancelAndDelete(handoverStarter_);
    cancelAndDelete(handoverTrigger_);
}

void LtePhyUe::initialize(int stage)
{
    LtePhyBase::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        isNr_ = false;        // this might be true only if this module is a UeNrPhy
        nodeType_ = UE;
        enableHandover_ = par("enableHandover");
        handoverLatency_ = par("handoverLatency").doubleValue();
        handoverDetachment_ = handoverLatency_ / 2.0;                      // TODO: make this configurable from NED
        handoverAttachment_ = handoverLatency_ - handoverDetachment_;
        dynamicCellAssociation_ = par("dynamicCellAssociation");
        if (par("minRssiDefault").boolValue())
            minRssi_ = binder_->phyPisaData.minSnr();
        else
            minRssi_ = par("minRssi").doubleValue();

        currentMasterRssi_ = -999.0;
        candidateMasterRssi_ = -999.0;

        hasCollector = par("hasCollector");

        if (!hasListeners(averageCqiDlSignal_))
            throw cRuntimeError("no phy listeners");

        WATCH(nodeType_);
        WATCH(masterId_);
        WATCH(candidateMasterId_);
        WATCH(currentMasterRssi_);
        WATCH(candidateMasterRssi_);
        WATCH(hysteresisTh_);
        WATCH(hysteresisFactor_);
        WATCH(handoverDelta_);

        txPower_ = ueTxPower_;

        handoverStarter_ = new cMessage("handoverStarter");

        // get the reference to the MAC module
        mac_ = check_and_cast<LteMacUe *>(gate(upperGateOut_)->getPathEndGate()->getOwnerModule());

        rlcUm_.reference(this, "rlcUmModule", true);
        pdcp_.reference(this, "pdcpModule", true);
        ip2nic_.reference(this, "ip2nicModule", true);
        fbGen_.reference(this, "feedbackGeneratorModule", true);

        // setting isNr_ was originally done in the NrPhyUe subclass, but it is needed here
        isNr_ = dynamic_cast<NrPhyUe*>(this) && strcmp(getFullName(), "nrPhy") == 0;

        // get local id
        nodeId_ = MacNodeId(hostModule->par(isNr_ ? "nrMacNodeId" : "macNodeId").intValue());
        EV << "Local MacNodeId: " << nodeId_ << endl;
    }
    else if (stage == INITSTAGE_SIMU5G_PHYSICAL_LAYER) {
        // get serving cell from configuration
        masterId_ = binder_->getServingNode(nodeId_);
        candidateMasterId_ = masterId_;

        // find the best candidate master cell
        if (dynamicCellAssociation_) {
            findCandidateEnb(candidateMasterId_, candidateMasterRssi_);

            // binder calls
            // if dynamicCellAssociation selected a different master
            if (candidateMasterId_ != NODEID_NONE && candidateMasterId_ != masterId_) {
                binder_->unregisterServingNode(masterId_, nodeId_);
                binder_->registerServingNode(candidateMasterId_, nodeId_);
            }
            masterId_ = candidateMasterId_;
            currentMasterRssi_ = candidateMasterRssi_;
            updateHysteresisTh(candidateMasterRssi_);
        }

        EV << "LtePhyUe::initialize - Attaching to eNodeB " << masterId_ << endl;

        emit(servingCellSignal_, (long)masterId_);

        if (masterId_ == NODEID_NONE)
            masterMobility_ = nullptr;
        else {
            cModule *masterModule = binder_->getModuleByMacNodeId(masterId_);
            masterMobility_ = check_and_cast<IMobility *>(masterModule->getSubmodule("mobility"));
        }
    }
    else if (stage == INITSTAGE_SIMU5G_CELLINFO_CHANNELUPDATE) { //TODO being fwd, eliminate stage
        // get cellInfo at this stage because the next hop of the node is registered in the Ip2Nic module at the INITSTAGE_SIMU5G_NETWORK_LAYER
        if (masterId_ != NODEID_NONE)
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
    if (msg->isName("handoverStarter"))
        triggerHandover();
    else if (msg->isName("handoverTrigger")) {
        doHandover();
        delete msg;
        handoverTrigger_ = nullptr;
    }
}

void LtePhyUe::handoverHandler(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    lteInfo->setDestId(nodeId_);
    if (!enableHandover_) {
        delete frame;
        delete lteInfo;
        return;
    }

    frame->setControlInfo(lteInfo);
    double rssi = 0;

    // Compute RSSI from broadcast message (DAS removed - single antenna)
    std::vector<double> rssiV = primaryChannelModel_->getSINR(frame, lteInfo);
    for (auto value : rssiV)
        rssi += value;
    rssi /= rssiV.size();

    EV << "UE " << nodeId_ << " broadcast frame from " << lteInfo->getSourceId() << " with RSSI: " << rssi << " at " << simTime() << endl;

    if (lteInfo->getSourceId() != masterId_ && rssi < minRssi_) {
        EV << "Signal too weak from a candidate master - minRssi[" << minRssi_ << "]" << endl;
        delete frame;
        return;
    }

    if (rssi > candidateMasterRssi_ + hysteresisTh_) {
        if (lteInfo->getSourceId() == masterId_) {
            // receiving even stronger broadcast from current master
            currentMasterRssi_ = rssi;
            candidateMasterId_ = masterId_;
            candidateMasterRssi_ = rssi;
            hysteresisTh_ = updateHysteresisTh(currentMasterRssi_);
            cancelEvent(handoverStarter_);
        }
        else {
            // broadcast from another master with higher RSSI
            candidateMasterId_ = lteInfo->getSourceId();
            candidateMasterRssi_ = rssi;
            hysteresisTh_ = updateHysteresisTh(rssi);
            binder_->addHandoverTriggered(nodeId_, masterId_, candidateMasterId_);

            // schedule self message to evaluate handover parameters after
            // all broadcast messages have arrived
            if (!handoverStarter_->isScheduled()) {
                // all broadcast messages are scheduled at the very same time, a small delta
                // guarantees the ones belonging to the same turn have been received
                scheduleAt(simTime() + handoverDelta_, handoverStarter_);
            }
        }
    }
    else {
        if (lteInfo->getSourceId() == masterId_) {
            if (rssi >= minRssi_) {
                currentMasterRssi_ = rssi;
                candidateMasterRssi_ = rssi;
                hysteresisTh_ = updateHysteresisTh(rssi);
            }
            else { // lost connection from current master
                if (candidateMasterId_ == masterId_) { // trigger detachment
                    candidateMasterId_ = NODEID_NONE;
                    currentMasterRssi_ = -999.0;
                    candidateMasterRssi_ = -999.0; // set candidate RSSI very bad as we currently do not have any.
                                                   // this ensures that each candidate with is at least as 'bad'
                                                   // as the minRssi_ has a chance.

                    hysteresisTh_ = updateHysteresisTh(0);
                    binder_->addHandoverTriggered(nodeId_, masterId_, candidateMasterId_);

                    if (!handoverStarter_->isScheduled()) {
                        // all broadcast messages are scheduled at the very same time, a small delta
                        // guarantees the ones belonging to the same turn have been received
                        scheduleAt(simTime() + handoverDelta_, handoverStarter_);
                    }
                }
                // else do nothing, a stronger RSSI from another nodeB has been found already
            }
        }
    }

    delete frame;
}

void LtePhyUe::triggerHandover()
{
    ASSERT(masterId_ != candidateMasterId_);

    EV << "####Handover starting:####" << endl;
    EV << "current master: " << masterId_ << endl;
    EV << "current RSSI: " << currentMasterRssi_ << endl;
    EV << "candidate master: " << candidateMasterId_ << endl;
    EV << "candidate RSSI: " << candidateMasterRssi_ << endl;
    EV << "############" << endl;

    if (candidateMasterRssi_ == 0)
        EV << NOW << " LtePhyUe::triggerHandover - UE " << nodeId_ << " lost its connection to eNB " << masterId_ << ". Now detaching... " << endl;
    else if (masterId_ == NODEID_NONE)
        EV << NOW << " LtePhyUe::triggerHandover - UE " << nodeId_ << " is starting attachment procedure to eNB " << candidateMasterId_ << "... " << endl;
    else
        EV << NOW << " LtePhyUe::triggerHandover - UE " << nodeId_ << " is starting handover to eNB " << candidateMasterId_ << "... " << endl;

    binder_->addUeHandoverTriggered(nodeId_);

    // inform the UE's Ip2Nic module to start holding downstream packets
    ip2nic_->triggerHandoverUe(candidateMasterId_);
    binder_->removeHandoverTriggered(nodeId_);

    // inform the eNB's Ip2Nic module to forward data to the target eNB
    if (masterId_ != NODEID_NONE && candidateMasterId_ != NODEID_NONE) {
        Ip2Nic *enbIp2Nic = check_and_cast<Ip2Nic *>(binder_->getNodeModule(masterId_)->getSubmodule("cellularNic")->getSubmodule("ip2nic"));
        enbIp2Nic->triggerHandoverSource(nodeId_, candidateMasterId_);
    }

    double handoverLatency;
    if (masterId_ == NODEID_NONE)                                                // attachment only
        handoverLatency = handoverAttachment_;
    else if (candidateMasterId_ == NODEID_NONE)                                                // detachment only
        handoverLatency = handoverDetachment_;
    else
        handoverLatency = handoverDetachment_ + handoverAttachment_;

    handoverTrigger_ = new cMessage("handoverTrigger");
    scheduleAt(simTime() + handoverLatency, handoverTrigger_);
}

void LtePhyUe::doHandover()
{
    // if masterId_ == 0, it means the UE was not attached to any eNodeB, so it only has to perform attachment procedures
    // if candidateMasterId_ == 0, it means the UE is detaching from its eNodeB, so it only has to perform detachment procedures

    if (masterId_ != NODEID_NONE) {
        // Delete Old Buffers
        deleteOldBuffers(masterId_);

        // amc calls
        LteAmc *oldAmc = getAmcModule(masterId_);
        oldAmc->detachUser(nodeId_, UL);
        oldAmc->detachUser(nodeId_, DL);
    }

    if (candidateMasterId_ != NODEID_NONE) {
        LteAmc *newAmc = getAmcModule(candidateMasterId_);
        assert(newAmc != nullptr);
        newAmc->attachUser(nodeId_, UL);
        newAmc->attachUser(nodeId_, DL);
    }

    // binder calls
    if (masterId_ != NODEID_NONE)
        binder_->unregisterServingNode(masterId_, nodeId_);

    if (candidateMasterId_ != NODEID_NONE) {
        binder_->registerServingNode(candidateMasterId_, nodeId_);
    }
    binder_->updateUeInfoCellId(nodeId_, candidateMasterId_);

    // @author Alessandro Noferi
    if (hasCollector) {
        binder_->moveUeCollector(nodeId_, masterId_, candidateMasterId_);
    }

    // change masterId and notify handover to the MAC layer
    MacNodeId oldMaster = masterId_;
    masterId_ = candidateMasterId_;
    mac_->doHandover(candidateMasterId_);  // do MAC operations for handover
    currentMasterRssi_ = candidateMasterRssi_;
    hysteresisTh_ = updateHysteresisTh(currentMasterRssi_);

    // update reference to master node's mobility module
    if (masterId_ == NODEID_NONE)
        masterMobility_ = nullptr;
    else {
        cModule *masterModule = binder_->getModuleByMacNodeId(masterId_);
        masterMobility_ = check_and_cast<IMobility *>(masterModule->getSubmodule("mobility"));
    }

    // update cellInfo
    if (oldMaster != NODEID_NONE)
        cellInfo_->detachUser(nodeId_);

    if (masterId_ != NODEID_NONE) {
        LteMacEnb *newMacEnb = check_and_cast<LteMacEnb *>(binder_->getMacByNodeId(masterId_));
        cellInfo_ = newMacEnb->getCellInfo();
        cellInfo_->attachUser(nodeId_);
    }

    // update DL feedback generator
    fbGen_->handleHandover(masterId_);

    // collect stat
    emit(servingCellSignal_, (long)masterId_);

    if (masterId_ == NODEID_NONE)
        EV << NOW << " LtePhyUe::doHandover - UE " << nodeId_ << " detached from the network" << endl;
    else
        EV << NOW << " LtePhyUe::doHandover - UE " << nodeId_ << " has completed handover to eNB " << masterId_ << "... " << endl;
    binder_->removeUeHandoverTriggered(nodeId_);

    // inform the UE's Ip2Nic module to forward held packets
    ip2nic_->signalHandoverCompleteUe();

    // inform the eNB's Ip2Nic module to forward data to the target eNB
    if (oldMaster != NODEID_NONE && candidateMasterId_ != NODEID_NONE) {
        Ip2Nic *enbIp2Nic = check_and_cast<Ip2Nic *>(binder_->getNodeModule(masterId_)->getSubmodule("cellularNic")->getSubmodule("ip2nic"));
        enbIp2Nic->signalHandoverCompleteTarget(nodeId_, oldMaster);
    }
}

// TODO: ***reorganize*** method
void LtePhyUe::handleAirFrame(cMessage *msg)
{
    LteAirFrame *frame = static_cast<LteAirFrame *>(msg);
    UserControlInfo *lteInfo = new UserControlInfo(frame->getAdditionalInfo());

    connectedNodeId_ = masterId_;
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
    if (lteInfo->getFrameType() == HANDOVERPKT) {
        // check if the message is on another carrier frequency or handover is already in process
        if (carrierFreq != primaryChannelModel_->getCarrierFrequency() || (handoverTrigger_ != nullptr && handoverTrigger_->isScheduled())) {
            EV << "Received handover packet on a different carrier frequency than the primary cell. Delete it." << endl;
            delete lteInfo;
            delete frame;
            return;
        }

        handoverHandler(frame, lteInfo);
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
    if (lteInfo->getSourceId() != masterId_) {
        EV << "WARNING: frame from an old master during handover: deleted " << endl;
        EV << "Source MacNodeId: " << lteInfo->getSourceId() << endl;
        EV << "Master MacNodeId: " << masterId_ << endl;
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
    if (dest != masterId_) {
        // UE is not sending to its master!!
        throw cRuntimeError("LtePhyUe::handleUpperMessage  Ue preparing to send message to %hu instead of its master (%hu)", num(dest), num(masterId_));
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
    // emit serving cell id
    emit(servingCellSignal_, (long)masterId_);

    if (masterMobility_) {
        // emit distance from current serving cell
        inet::Coord masterPos = masterMobility_->getCurrentPosition();
        double distance = getRadioPosition().distance(masterPos);
        emit(distanceSignal_, distance);
    }
}

double LtePhyUe::updateHysteresisTh(double v)
{
    if (hysteresisFactor_ == 0)
        return 0;
    else
        return v / hysteresisFactor_;
}

void LtePhyUe::deleteOldBuffers(MacNodeId masterId)
{
    // Delete Mac Buffers

    // delete macBuffer[nodeId_] at old master
    LteMacEnb *masterMac = check_and_cast<LteMacEnb *>(binder_->getMacByNodeId(masterId));
    masterMac->deleteQueues(nodeId_);

    // delete queues for master at this ue
    mac_->deleteQueues(masterId_);

    // Delete Rlc UM Buffers

    // delete UmTxQueue[nodeId_] at old master
    LteRlcUm *masterRlcUm = check_and_cast<LteRlcUm *>(binder_->getRlcByNodeId(masterId, UM));
    masterRlcUm->deleteQueues(nodeId_);

    // delete queues for master at this ue
    rlcUm_->deleteQueues(nodeId_);

    // Delete PDCP Entities
    // delete pdcpEntities[nodeId_] at old master
    LtePdcpEnb *masterPdcp = check_and_cast<LtePdcpEnb *>(binder_->getPdcpByNodeId(masterId));
    masterPdcp->deleteEntities(nodeId_);

    // delete queues for master at this ue
    pdcp_->deleteEntities(masterId_);
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
    uinfo->setDestId(masterId_);
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
        // do this only during the deletion of the module during the simulation

        // do this only if this PHY layer is connected to a serving base station
        if (masterId_ != NODEID_NONE) {
            // clear buffers
            deleteOldBuffers(masterId_);

            // amc calls
            LteAmc *amc = getAmcModule(masterId_);
            if (amc != nullptr) {
                amc->detachUser(nodeId_, UL);
                amc->detachUser(nodeId_, DL);
            }

            // binder call
            binder_->unregisterServingNode(masterId_, nodeId_);

            // cellInfo call
            cellInfo_->detachUser(nodeId_);
        }
    }
}

} //namespace
