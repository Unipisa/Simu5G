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

#include "stack/phy/NRPhyUe.h"

#include "stack/ip2nic/IP2Nic.h"
#include "stack/phy/feedback/LteDlFeedbackGenerator.h"
#include "stack/d2dModeSelection/D2DModeSelectionBase.h"

namespace simu5g {

Define_Module(NRPhyUe);



void NRPhyUe::initialize(int stage)
{
    LtePhyUeD2D::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        isNr_ = (strcmp(getFullName(), "nrPhy") == 0);
        otherPhy_.reference(this, "otherPhyModule", true);
    }
}

// TODO: ***reorganize*** method
void NRPhyUe::handleAirFrame(cMessage *msg)
{
    UserControlInfo *lteInfo = check_and_cast<UserControlInfo *>(msg->removeControlInfo());

    connectedNodeId_ = masterId_;
    LteAirFrame *frame = check_and_cast<LteAirFrame *>(msg);
    EV << "NRPhyUe: received new LteAirFrame with ID " << frame->getId() << " from channel" << endl;

    MacNodeId sourceId = lteInfo->getSourceId();
    if (binder_->getOmnetId(sourceId) == 0) {
        // The source has left the simulation
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

    //Update coordinates of this user
    if (lteInfo->getFrameType() == HANDOVERPKT) {
        // Check if the message is on another carrier frequency or handover is already in process
        if (carrierFreq != primaryChannelModel_->getCarrierFrequency() || (handoverTrigger_ != nullptr && handoverTrigger_->isScheduled())) {
            EV << "Received handover packet on a different carrier frequency. Delete it." << endl;
            delete lteInfo;
            delete frame;
            return;
        }

        // Check if the message is from a different cellular technology
        if (lteInfo->isNr() != isNr_) {
            EV << "Received handover packet [from NR=" << lteInfo->isNr() << "] from a different radio technology [to NR=" << isNr_ << "]. Delete it." << endl;
            delete lteInfo;
            delete frame;
            return;
        }

        // Check if the eNodeB is a secondary node
        MacNodeId masterNodeId = binder_->getMasterNode(sourceId);
        if (masterNodeId != sourceId) {
            // The node has a master node, check if the other PHY of this UE is attached to that master.
            // If not, the UE cannot attach to this secondary node and the packet must be deleted.
            if (otherPhy_->getMasterId() != masterNodeId) {
                EV << "Received handover packet from " << sourceId << ", which is a secondary node to a master [" << masterNodeId << "] different from the one this UE is attached to. Delete packet." << endl;
                delete lteInfo;
                delete frame;
                return;
            }
        }

        handoverHandler(frame, lteInfo);
        return;
    }

    // Check if the frame is for us ( MacNodeId matches or - if this is a multicast communication - enrolled in multicast group)
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
     * This could happen if the UE associates with a new master while a packet from the
     * old master is in-flight: the packet is in the air
     * while the UE changes master.
     * Event timing:      TTI x: packet scheduled and sent by the UE (tx time = 1ms)
     *                     TTI x+0.1: UE changes master
     *                     TTI x+1: packet from UE arrives at the old master
     */
    if (lteInfo->getDirection() != D2D && lteInfo->getDirection() != D2D_MULTI && lteInfo->getSourceId() != masterId_) {
        EV << "WARNING: Frame from a UE that is leaving this cell (handover): deleted " << endl;
        EV << "Source MacNodeId: " << lteInfo->getSourceId() << endl;
        EV << "UE MacNodeId: " << nodeId_ << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    if (binder_->isInMulticastGroup(nodeId_, lteInfo->getMulticastGroupId())) {
        // HACK: If this is a multicast connection, change the destId of the airframe so that upper layers can handle it
        lteInfo->setDestId(nodeId_);
    }

    // Send H-ARQ feedback up
    if (lteInfo->getFrameType() == HARQPKT || lteInfo->getFrameType() == GRANTPKT || lteInfo->getFrameType() == RACPKT || lteInfo->getFrameType() == D2DMODESWITCHPKT) {
        handleControlMsg(frame, lteInfo);
        return;
    }

    // This is a DATA packet

    // If the packet is a D2D multicast one, store it and decode it at the end of the TTI
    if (d2dMulticastEnableCaptureEffect_ && binder_->isInMulticastGroup(nodeId_, lteInfo->getMulticastGroupId())) {
        // If not already started, auto-send a message to signal the presence of data to be decoded
        if (d2dDecodingTimer_ == nullptr) {
            d2dDecodingTimer_ = new cMessage("d2dDecodingTimer");
            d2dDecodingTimer_->setSchedulingPriority(10);          // Last thing to be performed in this TTI
            scheduleAt(NOW, d2dDecodingTimer_);
        }

        // Store frame, together with related control info
        frame->setControlInfo(lteInfo);
        storeAirFrame(frame);            // Implements the capture effect

        return;                          // Exit the function, decoding will be done later
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
    // Apply decider to received packet
    bool result = true;
    RemoteSet r = lteInfo->getUserTxParams()->readAntennaSet();
    if (r.size() > 1) {
        // DAS
        for (auto it : r) {
            EV << "NRPhyUe: Receiving Packet from antenna " << it << "\n";

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

    // Here frame has to be destroyed since it is no more useful
    delete frame;

    // Attach the decider result to the packet as control info
    lteInfo->setDeciderResult(result);
    *(pkt->addTagIfAbsent<UserControlInfo>()) = *lteInfo;
    delete lteInfo;

    // Send decapsulated message along with result control info to upperGateOut_
    send(pkt, upperGateOut_);

    if (getEnvir()->isGUI())
        updateDisplayString();
}

void NRPhyUe::triggerHandover()
{
    // TODO: Remove asserts after testing
    assert(masterId_ != candidateMasterId_);

    EV << "####Handover starting:####" << endl;
    EV << "Current master: " << masterId_ << endl;
    EV << "Current RSSI: " << currentMasterRssi_ << endl;
    EV << "Candidate master: " << candidateMasterId_ << endl;
    EV << "Candidate RSSI: " << candidateMasterRssi_ << endl;
    EV << "############" << endl;

    MacNodeId masterNode = binder_->getMasterNode(candidateMasterId_);
    if (masterNode != candidateMasterId_) { // The candidate is a secondary node
        if (otherPhy_->getMasterId() == masterNode) {
            MacNodeId otherNodeId = otherPhy_->getMacNodeId();
            const std::pair<MacNodeId, MacNodeId> *handoverPair = binder_->getHandoverTriggered(otherNodeId);
            if (handoverPair != nullptr) {
                if (handoverPair->second == candidateMasterId_) {
                    // Delay this handover
                    double delta = handoverDelta_;
                    if (handoverPair->first != NODEID_NONE) // The other "stack" is performing a complete handover
                        delta += handoverDetachment_ + handoverAttachment_;
                    else                                                   // The other "stack" is attaching to an eNodeB
                        delta += handoverAttachment_;

                    EV << NOW << " NRPhyUe::triggerHandover - Wait for the handover completion for the other stack. Delay this handover." << endl;

                    // Need to wait for the other stack to complete handover
                    scheduleAt(simTime() + delta, handoverStarter_);
                    return;
                }
                else {
                    // Cancel this handover
                    binder_->removeHandoverTriggered(nodeId_);
                    EV << NOW << " NRPhyUe::triggerHandover - UE " << nodeId_ << " is canceling its handover to eNB " << candidateMasterId_ << " since the master is performing handover" << endl;
                    return;
                }
            }
        }
    }
    // Else it is a master itself

    if (candidateMasterRssi_ == 0)
        EV << NOW << " NRPhyUe::triggerHandover - UE " << nodeId_ << " lost its connection to eNB " << masterId_ << ". Now detaching... " << endl;
    else if (masterId_ == NODEID_NONE)
        EV << NOW << " NRPhyUe::triggerHandover - UE " << nodeId_ << " is starting attachment procedure to eNB " << candidateMasterId_ << "... " << endl;
    else
        EV << NOW << " NRPhyUe::triggerHandover - UE " << nodeId_ << " is starting handover to eNB " << candidateMasterId_ << "... " << endl;

    if (otherPhy_->getMasterId() != NODEID_NONE) {
        // Check if there are secondary nodes connected
        MacNodeId otherMasterId = binder_->getMasterNode(otherPhy_->getMasterId());
        if (otherMasterId == masterId_) {
            EV << NOW << " NRPhyUe::triggerHandover - Forcing detachment from " << otherPhy_->getMasterId() << " which was a secondary node to " << masterId_ << ". Delay this handover." << endl;

            // Need to wait for the other stack to complete detachment
            scheduleAt(simTime() + handoverDetachment_ + handoverDelta_, handoverStarter_);

            // The other stack is connected to a node which is a secondary node of the master from which this stack is leaving
            // Trigger detachment (handover to node 0)
            otherPhy_->forceHandover();

            return;
        }
    }

    binder_->addUeHandoverTriggered(nodeId_);

    // Inform the UE's IP2Nic module to start holding downstream packets
    ip2nic_->triggerHandoverUe(candidateMasterId_, isNr_);

    // Inform the eNB's IP2Nic module to forward data to the target eNB
    if (masterId_ != NODEID_NONE && candidateMasterId_ != NODEID_NONE) {
        IP2Nic *enbIp2nic = check_and_cast<IP2Nic *>(getSimulation()->getModule(binder_->getOmnetId(masterId_))->getSubmodule("cellularNic")->getSubmodule("ip2nic"));
        enbIp2nic->triggerHandoverSource(nodeId_, candidateMasterId_);
    }

    if (masterId_ != NODEID_NONE) {
        // Stop active D2D flows (go back to Infrastructure mode)
        // Currently, DM is possible only for UEs served by the same cell

        // Trigger D2D mode switch
        cModule *enb = getSimulation()->getModule(binder_->getOmnetId(masterId_));
        D2DModeSelectionBase *d2dModeSelection = check_and_cast<D2DModeSelectionBase *>(enb->getSubmodule("cellularNic")->getSubmodule("d2dModeSelection"));
        d2dModeSelection->doModeSwitchAtHandover(nodeId_, false);
    }

    double handoverLatency;
    if (masterId_ == NODEID_NONE)                                                // Attachment only
        handoverLatency = handoverAttachment_;
    else if (candidateMasterId_ == NODEID_NONE)                                                // Detachment only
        handoverLatency = handoverDetachment_;
    else                                                // Complete handover time
        handoverLatency = handoverDetachment_ + handoverAttachment_;

    handoverTrigger_ = new cMessage("handoverTrigger");
    scheduleAt(simTime() + handoverLatency, handoverTrigger_);
}

void NRPhyUe::doHandover()
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
        oldAmc->detachUser(nodeId_, D2D);
    }

    if (candidateMasterId_ != NODEID_NONE) {
        LteAmc *newAmc = getAmcModule(candidateMasterId_);
        assert(newAmc != nullptr);
        newAmc->attachUser(nodeId_, UL);
        newAmc->attachUser(nodeId_, DL);
        newAmc->attachUser(nodeId_, D2D);
    }

    // binder calls
    if (masterId_ != NODEID_NONE)
        binder_->unregisterNextHop(masterId_, nodeId_);

    if (candidateMasterId_ != NODEID_NONE) {
        binder_->registerNextHop(candidateMasterId_, nodeId_);
        das_->setMasterRuSet(candidateMasterId_);
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

    // update NED parameter
    if (isNr_)
        hostModule->par("nrMasterId").setIntValue(num(masterId_));
    else
        hostModule->par("masterId").setIntValue(num(masterId_));

    if (masterId_ == NODEID_NONE)
        masterMobility_ = nullptr;
    else {
        cModule *masterModule = binder_->getModuleByMacNodeId(masterId_);
        masterMobility_ = check_and_cast<IMobility *>(masterModule->getSubmodule("mobility"));
    }
    // update cellInfo
    if (masterId_ != NODEID_NONE)
        cellInfo_->detachUser(nodeId_);

    if (candidateMasterId_ != NODEID_NONE) {
        CellInfo *oldCellInfo = cellInfo_;
        LteMacEnb *newMacEnb = check_and_cast<LteMacEnb *>(getSimulation()->getModule(binder_->getOmnetId(candidateMasterId_))->getSubmodule("cellularNic")->getSubmodule("mac"));
        CellInfo *newCellInfo = newMacEnb->getCellInfo();
        newCellInfo->attachUser(nodeId_);
        cellInfo_ = newCellInfo;
        if (oldCellInfo == nullptr) {
            // first time the UE is attached to someone
            int index = intuniform(0, binder_->phyPisaData.maxChannel() - 1);
            cellInfo_->lambdaInit(nodeId_, index);
            cellInfo_->channelUpdate(nodeId_, intuniform(1, binder_->phyPisaData.maxChannel2()));
        }

        // send a self-message to schedule the possible mode switch at the end of the TTI (after all UEs have performed the handover)
        cMessage *msg = new cMessage("doModeSwitchAtHandover");
        msg->setSchedulingPriority(10);
        scheduleAt(NOW, msg);
    }

    // update DL feedback generator
    fbGen_->handleHandover(masterId_);

    // collect stat
    emit(servingCellSignal_, (long)masterId_);

    if (masterId_ == NODEID_NONE)
        EV << NOW << " NRPhyUe::doHandover - UE " << nodeId_ << " detached from the network" << endl;
    else
        EV << NOW << " NRPhyUe::doHandover - UE " << nodeId_ << " has completed handover to eNB " << masterId_ << "... " << endl;

    binder_->removeUeHandoverTriggered(nodeId_);
    binder_->removeHandoverTriggered(nodeId_);

    // inform the UE's IP2Nic module to forward held packets
    ip2nic_->signalHandoverCompleteUe(isNr_);

    // inform the eNB's IP2Nic module to forward data to the target eNB
    if (oldMaster != NODEID_NONE && candidateMasterId_ != NODEID_NONE) {
        IP2Nic *enbIp2nic = check_and_cast<IP2Nic *>(getSimulation()->getModule(binder_->getOmnetId(masterId_))->getSubmodule("cellularNic")->getSubmodule("ip2nic"));
        enbIp2nic->signalHandoverCompleteTarget(nodeId_, oldMaster);
    }
}

void NRPhyUe::forceHandover(MacNodeId targetMasterNode, double targetMasterRssi)
{
    Enter_Method_Silent();
    candidateMasterId_ = targetMasterNode;
    candidateMasterRssi_ = targetMasterRssi;
    hysteresisTh_ = updateHysteresisTh(currentMasterRssi_);

    cancelEvent(handoverStarter_);  // if any
    scheduleAt(NOW, handoverStarter_);
}

void NRPhyUe::deleteOldBuffers(MacNodeId masterId)
{
    // Delete MAC Buffers

    // delete macBuffer[nodeId_] at old master
    LteMacEnb *masterMac = check_and_cast<LteMacEnb *>(getMacByMacNodeId(binder_, masterId));
    masterMac->deleteQueues(nodeId_);

    // delete queues for master at this UE
    mac_->deleteQueues(masterId_);

    // Delete RLC UM Buffers

    // delete UmTxQueue[nodeId_] at old master
    LteRlcUm *masterRlcUm = check_and_cast<LteRlcUm *>(getRlcByMacNodeId(binder_, masterId, UM));
    masterRlcUm->deleteQueues(nodeId_);

    // delete queues for master at this UE
    rlcUm_->deleteQueues(nodeId_);

    // Delete PDCP Entities
    // delete pdcpEntities[nodeId_] at old master
    // in case of NR dual connectivity, the master can be a secondary node, hence we have to delete PDCP entities residing in the node's master
    MacNodeId masterNodeId = binder_->getMasterNode(masterId);
    LtePdcpRrcEnb *masterPdcp = check_and_cast<LtePdcpRrcEnb *>(getPdcpByMacNodeId(binder_, masterNodeId));
    masterPdcp->deleteEntities(nodeId_);

    // delete queues for master at this UE
    pdcp_->deleteEntities(masterId_);
}

} //namespace

