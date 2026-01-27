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

#include "HandoverController.h"
#include "simu5g/stack/phy/LtePhyUe.h"
#include "simu5g/stack/phy/LtePhyUeD2D.h"
#include "simu5g/stack/phy/NrPhyUe.h"
#include "simu5g/stack/d2dModeSelection/D2dModeSelectionBase.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(HandoverController);

HandoverController::~HandoverController()
{
}

void HandoverController::initialize(int stage)
{
}

void HandoverController::handleMessage(cMessage *msg)
{
}

void HandoverController::LtePhyUe_handoverHandler(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    if (!phy_->enableHandover_) {
        delete frame;
        delete lteInfo;
        return;
    }

    if (phy_->handoverTrigger_ != nullptr && phy_->handoverTrigger_->isScheduled()) {
        EV << "Handover already in progress, ignoring beacon packet." << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    // Check if the eNodeB is a secondary node
    if (NrPhyUe *phyAsNr = dynamic_cast<NrPhyUe*>(phy_)) {
        MacNodeId sourceId = lteInfo->getSourceId();
        MacNodeId masterNodeId = phy_->binder_->getMasterNodeOrSelf(sourceId);
        if (masterNodeId != sourceId) {
            // The node has a master node, check if the other PHY of this UE is attached to that master.
            // If not, the UE cannot attach to this secondary node and the packet must be deleted.
            if (phyAsNr->otherPhy_->getMasterId() != masterNodeId) {
                EV << "Received beacon packet from " << sourceId << ", which is a secondary node to a master [" << masterNodeId << "] different from the one this UE is attached to. Delete packet." << endl;
                delete lteInfo;
                delete frame;
                return;
            }
        }
    }

    lteInfo->setDestId(phy_->nodeId_);
    frame->setControlInfo(lteInfo);

    double rssi = phy_->computeReceivedBeaconPacketRssi(frame, lteInfo);
    EV << "UE " << phy_->nodeId_ << " broadcast frame from " << lteInfo->getSourceId() << " with RSSI: " << rssi << " at " << simTime() << endl;

    if (lteInfo->getSourceId() != phy_->masterId_ && rssi < phy_->minRssi_) {
        EV << "Signal too weak from a candidate master - minRssi[" << phy_->minRssi_ << "]" << endl;
        delete frame;
        return;
    }

    if (rssi > phy_->candidateMasterRssi_ + phy_->hysteresisTh_) {
        if (lteInfo->getSourceId() == phy_->masterId_) {
            // receiving even stronger broadcast from current master
            phy_->currentMasterRssi_ = rssi;
            phy_->candidateMasterId_ = phy_->masterId_;
            phy_->candidateMasterRssi_ = rssi;
            phy_->hysteresisTh_ = updateHysteresisTh(phy_->currentMasterRssi_);
            phy_->cancelEvent(phy_->handoverStarter_);
        }
        else {
            // broadcast from another master with higher RSSI
            phy_->candidateMasterId_ = lteInfo->getSourceId();
            phy_->candidateMasterRssi_ = rssi;
            phy_->hysteresisTh_ = updateHysteresisTh(rssi);
            phy_->binder_->addHandoverTriggered(phy_->nodeId_, phy_->masterId_, phy_->candidateMasterId_);

            // schedule self message to evaluate handover parameters after
            // all broadcast messages have arrived
            if (!phy_->handoverStarter_->isScheduled()) {
                // all broadcast messages are scheduled at the very same time, a small delta
                // guarantees the ones belonging to the same turn have been received
                phy_->scheduleAt(simTime() + phy_->handoverDelta_, phy_->handoverStarter_);
            }
        }
    }
    else {
        if (lteInfo->getSourceId() == phy_->masterId_) {
            if (rssi >= phy_->minRssi_) {
                phy_->currentMasterRssi_ = rssi;
                phy_->candidateMasterRssi_ = rssi;
                phy_->hysteresisTh_ = updateHysteresisTh(rssi);
            }
            else { // lost connection from current master
                if (phy_->candidateMasterId_ == phy_->masterId_) { // trigger detachment
                    phy_->candidateMasterId_ = NODEID_NONE;
                    phy_->currentMasterRssi_ = -999.0;
                    phy_->candidateMasterRssi_ = -999.0; // set candidate RSSI very bad as we currently do not have any.
                                                   // this ensures that each candidate with is at least as 'bad'
                                                   // as the minRssi_ has a chance.

                    phy_->hysteresisTh_ = updateHysteresisTh(0);
                    phy_->binder_->addHandoverTriggered(phy_->nodeId_, phy_->masterId_, phy_->candidateMasterId_);

                    if (!phy_->handoverStarter_->isScheduled()) {
                        // all broadcast messages are scheduled at the very same time, a small delta
                        // guarantees the ones belonging to the same turn have been received
                        phy_->scheduleAt(simTime() + phy_->handoverDelta_, phy_->handoverStarter_);
                    }
                }
                // else do nothing, a stronger RSSI from another nodeB has been found already
            }
        }
    }

    delete frame;
}

void HandoverController::LtePhyUe_triggerHandover()
{
    ASSERT(phy_->masterId_ != phy_->candidateMasterId_);

    EV << "####Handover starting:####" << endl;
    EV << "current master: " << phy_->masterId_ << endl;
    EV << "current RSSI: " << phy_->currentMasterRssi_ << endl;
    EV << "candidate master: " << phy_->candidateMasterId_ << endl;
    EV << "candidate RSSI: " << phy_->candidateMasterRssi_ << endl;
    EV << "############" << endl;

    if (phy_->candidateMasterRssi_ == 0)
        EV << NOW << " LtePhyUe::triggerHandover - UE " << phy_->nodeId_ << " lost its connection to eNB " << phy_->masterId_ << ". Now detaching... " << endl;
    else if (phy_->masterId_ == NODEID_NONE)
        EV << NOW << " LtePhyUe::triggerHandover - UE " << phy_->nodeId_ << " is starting attachment procedure to eNB " << phy_->candidateMasterId_ << "... " << endl;
    else
        EV << NOW << " LtePhyUe::triggerHandover - UE " << phy_->nodeId_ << " is starting handover to eNB " << phy_->candidateMasterId_ << "... " << endl;

    phy_->binder_->addUeHandoverTriggered(phy_->nodeId_);

    // inform the UE's Ip2Nic module to start holding downstream packets
    phy_->ip2nic_->triggerHandoverUe(phy_->candidateMasterId_);
    phy_->binder_->removeHandoverTriggered(phy_->nodeId_);

    // inform the eNB's Ip2Nic module to forward data to the target eNB
    if (phy_->masterId_ != NODEID_NONE && phy_->candidateMasterId_ != NODEID_NONE) {
        Ip2Nic *enbIp2Nic = check_and_cast<Ip2Nic *>(phy_->binder_->getNodeModule(phy_->masterId_)->getSubmodule("cellularNic")->getSubmodule("ip2nic"));
        enbIp2Nic->triggerHandoverSource(phy_->nodeId_, phy_->candidateMasterId_);
    }

    double handoverLatency;
    if (phy_->masterId_ == NODEID_NONE)                                                // attachment only
        handoverLatency = phy_->handoverAttachment_;
    else if (phy_->candidateMasterId_ == NODEID_NONE)                                                // detachment only
        handoverLatency = phy_->handoverDetachment_;
    else
        handoverLatency = phy_->handoverDetachment_ + phy_->handoverAttachment_;

    phy_->handoverTrigger_ = new cMessage("handoverTrigger");
    phy_->scheduleAt(simTime() + handoverLatency, phy_->handoverTrigger_);
}

void HandoverController::LtePhyUeD2D_triggerHandover()
{
    if (phy_->masterId_ != NODEID_NONE) {
        // Stop active D2D flows (go back to Infrastructure mode).
        // Currently, DM is possible only for UEs served by the same cell.

        // Trigger D2D mode switch.
        cModule *enb = phy_->binder_->getNodeModule(phy_->masterId_);
        D2dModeSelectionBase *d2dModeSelection = check_and_cast<D2dModeSelectionBase *>(enb->getSubmodule("cellularNic")->getSubmodule("d2dModeSelection"));
        d2dModeSelection->doModeSwitchAtHandover(phy_->nodeId_, false);
    }
    LtePhyUe_triggerHandover();
}

void HandoverController::NrPhyUe_triggerHandover()
{
    NrPhyUe *phy_ = check_and_cast<NrPhyUe*>(this->phy_);
    ASSERT(phy_->masterId_ == NODEID_NONE || phy_->masterId_ != phy_->candidateMasterId_);  // "we can be unattached, but never hand over to ourselves"

    EV << "####Handover starting:####" << endl;
    EV << "Current master: " << phy_->masterId_ << endl;
    EV << "Current RSSI: " << phy_->currentMasterRssi_ << endl;
    EV << "Candidate master: " << phy_->candidateMasterId_ << endl;  // note: can be NODEID_NONE!
    EV << "Candidate RSSI: " << phy_->candidateMasterRssi_ << endl;
    EV << "############" << endl;

    MacNodeId masterNode = phy_->binder_->getMasterNodeOrSelf(phy_->candidateMasterId_);
    if (masterNode != phy_->candidateMasterId_) { // The candidate is a secondary node
        if (phy_->otherPhy_->getMasterId() == masterNode) {
            MacNodeId otherNodeId = phy_->otherPhy_->getMacNodeId();
            const std::pair<MacNodeId, MacNodeId> *handoverPair = phy_->binder_->getHandoverTriggered(otherNodeId);
            if (handoverPair != nullptr) {
                if (handoverPair->second == phy_->candidateMasterId_) {
                    // Delay this handover
                    double delta = phy_->handoverDelta_;
                    if (handoverPair->first != NODEID_NONE) // The other "stack" is performing a complete handover
                        delta += phy_->handoverDetachment_ + phy_->handoverAttachment_;
                    else                                                   // The other "stack" is attaching to an eNodeB
                        delta += phy_->handoverAttachment_;

                    EV << NOW << " NrPhyUe::triggerHandover - Wait for the handover completion for the other stack. Delay this handover." << endl;

                    // Need to wait for the other stack to complete handover
                    phy_->scheduleAt(simTime() + delta, phy_->handoverStarter_);
                    return;
                }
                else {
                    // Cancel this handover
                    phy_->binder_->removeHandoverTriggered(phy_->nodeId_);
                    EV << NOW << " NrPhyUe::triggerHandover - UE " << phy_->nodeId_ << " is canceling its handover to eNB " << phy_->candidateMasterId_ << " since the master is performing handover" << endl;
                    return;
                }
            }
        }
    }
    // Else it is a master itself

    if (phy_->candidateMasterRssi_ == 0)
        EV << NOW << " NrPhyUe::triggerHandover - UE " << phy_->nodeId_ << " lost its connection to eNB " << phy_->masterId_ << ". Now detaching... " << endl;
    else if (phy_->masterId_ == NODEID_NONE)
        EV << NOW << " NrPhyUe::triggerHandover - UE " << phy_->nodeId_ << " is starting attachment procedure to eNB " << phy_->candidateMasterId_ << "... " << endl;
    else
        EV << NOW << " NrPhyUe::triggerHandover - UE " << phy_->nodeId_ << " is starting handover to eNB " << phy_->candidateMasterId_ << "... " << endl;

    if (phy_->otherPhy_->getMasterId() != NODEID_NONE) {
        // Check if there are secondary nodes connected
        MacNodeId otherMasterId = phy_->binder_->getMasterNodeOrSelf(phy_->otherPhy_->getMasterId());
        if (otherMasterId == phy_->masterId_) {
            EV << NOW << " NrPhyUe::triggerHandover - Forcing detachment from " << phy_->otherPhy_->getMasterId() << " which was a secondary node to " << phy_->masterId_ << ". Delay this handover." << endl;

            // Need to wait for the other stack to complete detachment
            phy_->scheduleAt(simTime() + phy_->handoverDetachment_ + phy_->handoverDelta_, phy_->handoverStarter_);

            // The other stack is connected to a node which is a secondary node of the master from which this stack is leaving
            // Trigger detachment (handover to node 0)
            phy_->otherPhy_->forceHandover();

            return;
        }
    }

    phy_->binder_->addUeHandoverTriggered(phy_->nodeId_);

    // Inform the UE's Ip2Nic module to start holding downstream packets
    phy_->ip2nic_->triggerHandoverUe(phy_->candidateMasterId_, phy_->isNr_);

    // Inform the eNB's Ip2Nic module to forward data to the target eNB
    if (phy_->masterId_ != NODEID_NONE && phy_->candidateMasterId_ != NODEID_NONE) {
        Ip2Nic *enbIp2nic = check_and_cast<Ip2Nic *>(phy_->binder_->getNodeModule(phy_->masterId_)->getSubmodule("cellularNic")->getSubmodule("ip2nic"));
        enbIp2nic->triggerHandoverSource(phy_->nodeId_, phy_->candidateMasterId_);
    }

    if (phy_->masterId_ != NODEID_NONE) {
        // Stop active D2D flows (go back to Infrastructure mode)
        // Currently, DM is possible only for UEs served by the same cell

        // Trigger D2D mode switch
        cModule *enb = phy_->binder_->getNodeModule(phy_->masterId_);
        D2dModeSelectionBase *d2dModeSelection = check_and_cast<D2dModeSelectionBase *>(enb->getSubmodule("cellularNic")->getSubmodule("d2dModeSelection"));
        d2dModeSelection->doModeSwitchAtHandover(phy_->nodeId_, false);
    }

    double handoverLatency;
    if (phy_->masterId_ == NODEID_NONE)                                                // Attachment only
        handoverLatency = phy_->handoverAttachment_;
    else if (phy_->candidateMasterId_ == NODEID_NONE)                                                // Detachment only
        handoverLatency = phy_->handoverDetachment_;
    else                                                // Complete handover time
        handoverLatency = phy_->handoverDetachment_ + phy_->handoverAttachment_;

    phy_->handoverTrigger_ = new cMessage("handoverTrigger");
    phy_->scheduleAt(simTime() + handoverLatency, phy_->handoverTrigger_);
}

void HandoverController::LtePhyUe_doHandover()
{
    // if phy_->masterId_ == 0, it means the UE was not attached to any eNodeB, so it only has to perform attachment procedures
    // if phy_->candidateMasterId_ == 0, it means the UE is detaching from its eNodeB, so it only has to perform detachment procedures

    if (phy_->masterId_ != NODEID_NONE) {
        // Delete Old Buffers
        LtePhyUe_deleteOldBuffers(phy_->masterId_);

        // amc calls
        LteAmc *oldAmc = phy_->getAmcModule(phy_->masterId_);
        oldAmc->detachUser(phy_->nodeId_, UL);
        oldAmc->detachUser(phy_->nodeId_, DL);
    }

    if (phy_->candidateMasterId_ != NODEID_NONE) {
        LteAmc *newAmc = phy_->getAmcModule(phy_->candidateMasterId_);
        assert(newAmc != nullptr);
        newAmc->attachUser(phy_->nodeId_, UL);
        newAmc->attachUser(phy_->nodeId_, DL);
    }

    // binder calls
    if (phy_->masterId_ != NODEID_NONE)
        phy_->binder_->unregisterServingNode(phy_->masterId_, phy_->nodeId_);

    if (phy_->candidateMasterId_ != NODEID_NONE) {
        phy_->binder_->registerServingNode(phy_->candidateMasterId_, phy_->nodeId_);
    }
    phy_->binder_->updateUeInfoCellId(phy_->nodeId_, phy_->candidateMasterId_);

    // @author Alessandro Noferi
    if (phy_->hasCollector) {
        phy_->binder_->moveUeCollector(phy_->nodeId_, phy_->masterId_, phy_->candidateMasterId_);
    }

    // change masterId and notify handover to the MAC layer
    MacNodeId oldMaster = phy_->masterId_;
    phy_->masterId_ = phy_->candidateMasterId_;
    phy_->mac_->doHandover(phy_->candidateMasterId_);  // do MAC operations for handover
    phy_->currentMasterRssi_ = phy_->candidateMasterRssi_;
    phy_->hysteresisTh_ = updateHysteresisTh(phy_->currentMasterRssi_);

    // update reference to master node's mobility module
    if (phy_->masterId_ == NODEID_NONE)
        phy_->masterMobility_ = nullptr;
    else {
        cModule *masterModule = phy_->binder_->getModuleByMacNodeId(phy_->masterId_);
        phy_->masterMobility_ = check_and_cast<IMobility *>(masterModule->getSubmodule("mobility"));
    }

    // update cellInfo
    if (oldMaster != NODEID_NONE)
        phy_->cellInfo_->detachUser(phy_->nodeId_);

    if (phy_->masterId_ != NODEID_NONE) {
        LteMacEnb *newMacEnb = check_and_cast<LteMacEnb *>(phy_->binder_->getMacByNodeId(phy_->masterId_));
        phy_->cellInfo_ = newMacEnb->getCellInfo();
        phy_->cellInfo_->attachUser(phy_->nodeId_);
    }

    // update DL feedback generator
    phy_->fbGen_->handleHandover(phy_->masterId_);

    // collect stat
    phy_->emit(phy_->servingCellSignal_, (long)phy_->masterId_);

    if (phy_->masterId_ == NODEID_NONE)
        EV << NOW << " LtePhyUe::doHandover - UE " << phy_->nodeId_ << " detached from the network" << endl;
    else
        EV << NOW << " LtePhyUe::doHandover - UE " << phy_->nodeId_ << " has completed handover to eNB " << phy_->masterId_ << "... " << endl;
    phy_->binder_->removeUeHandoverTriggered(phy_->nodeId_);

    // inform the UE's Ip2Nic module to forward held packets
    phy_->ip2nic_->signalHandoverCompleteUe();

    // inform the eNB's Ip2Nic module to forward data to the target eNB
    if (oldMaster != NODEID_NONE && phy_->candidateMasterId_ != NODEID_NONE) {
        Ip2Nic *enbIp2Nic = check_and_cast<Ip2Nic *>(phy_->binder_->getNodeModule(phy_->masterId_)->getSubmodule("cellularNic")->getSubmodule("ip2nic"));
        enbIp2Nic->signalHandoverCompleteTarget(phy_->nodeId_, oldMaster);
    }

}

void HandoverController::LtePhyUeD2D_doHandover()
{
    // AMC calls.
    if (phy_->masterId_ != NODEID_NONE) {
        LteAmc *oldAmc = phy_->getAmcModule(phy_->masterId_);
        oldAmc->detachUser(phy_->nodeId_, D2D);
    }

    if (phy_->candidateMasterId_ != NODEID_NONE) {
        LteAmc *newAmc = phy_->getAmcModule(phy_->candidateMasterId_);
        assert(newAmc != nullptr);
        newAmc->attachUser(phy_->nodeId_, D2D);
    }

    LtePhyUe_doHandover();

    if (phy_->candidateMasterId_ != NODEID_NONE) {
        // Send a self-message to schedule the possible mode switch at the end of the TTI (after all UEs have performed the handover).
        cMessage *msg = new cMessage("doModeSwitchAtHandover");
        msg->setSchedulingPriority(10);
        phy_->scheduleAt(NOW, msg);
    }

}

void HandoverController::NrPhyUe_doHandover()
{
    // if phy_->masterId_ == 0, it means the UE was not attached to any eNodeB, so it only has to perform attachment procedures
    // if phy_->candidateMasterId_ == 0, it means the UE is detaching from its eNodeB, so it only has to perform detachment procedures

    if (phy_->masterId_ != NODEID_NONE) {
        // Delete Old Buffers
        NrPhyUe_deleteOldBuffers(phy_->masterId_);

        // amc calls
        LteAmc *oldAmc = phy_->getAmcModule(phy_->masterId_);
        oldAmc->detachUser(phy_->nodeId_, UL);
        oldAmc->detachUser(phy_->nodeId_, DL);
        oldAmc->detachUser(phy_->nodeId_, D2D);
    }

    if (phy_->candidateMasterId_ != NODEID_NONE) {
        LteAmc *newAmc = phy_->getAmcModule(phy_->candidateMasterId_);
        assert(newAmc != nullptr);
        newAmc->attachUser(phy_->nodeId_, UL);
        newAmc->attachUser(phy_->nodeId_, DL);
        newAmc->attachUser(phy_->nodeId_, D2D);
    }

    // binder calls
    if (phy_->masterId_ != NODEID_NONE)
        phy_->binder_->unregisterServingNode(phy_->masterId_, phy_->nodeId_);

    if (phy_->candidateMasterId_ != NODEID_NONE) {
        phy_->binder_->registerServingNode(phy_->candidateMasterId_, phy_->nodeId_);
    }
    phy_->binder_->updateUeInfoCellId(phy_->nodeId_, phy_->candidateMasterId_);
    // @author Alessandro Noferi
    if (phy_->hasCollector) {
        phy_->binder_->moveUeCollector(phy_->nodeId_, phy_->masterId_, phy_->candidateMasterId_);
    }

    // change masterId and notify handover to the MAC layer
    MacNodeId oldMaster = phy_->masterId_;
    phy_->masterId_ = phy_->candidateMasterId_;
    phy_->mac_->doHandover(phy_->candidateMasterId_);  // do MAC operations for handover
    phy_->currentMasterRssi_ = phy_->candidateMasterRssi_;
    phy_->hysteresisTh_ = updateHysteresisTh(phy_->currentMasterRssi_);

    // update mobility pointer
    if (phy_->masterId_ == NODEID_NONE)
        phy_->masterMobility_ = nullptr;
    else {
        cModule *masterModule = phy_->binder_->getModuleByMacNodeId(phy_->masterId_);
        phy_->masterMobility_ = check_and_cast<IMobility *>(masterModule->getSubmodule("mobility"));
    }
    // update cellInfo
    if (oldMaster != NODEID_NONE)
        phy_->cellInfo_->detachUser(phy_->nodeId_);

    if (phy_->masterId_ != NODEID_NONE) {
        LteMacEnb *newMacEnb = check_and_cast<LteMacEnb *>(phy_->binder_->getMacByNodeId(phy_->masterId_));
        phy_->cellInfo_ = newMacEnb->getCellInfo();
        phy_->cellInfo_->attachUser(phy_->nodeId_);

        // send a self-message to schedule the possible mode switch at the end of the TTI (after all UEs have performed the handover)
        cMessage *msg = new cMessage("doModeSwitchAtHandover");
        msg->setSchedulingPriority(10);
        phy_->scheduleAt(NOW, msg);
    }

    // update DL feedback generator
    phy_->fbGen_->handleHandover(phy_->masterId_);

    // collect stat
    phy_->emit(phy_->servingCellSignal_, (long)phy_->masterId_);

    if (phy_->masterId_ == NODEID_NONE)
        EV << NOW << " NrPhyUe::doHandover - UE " << phy_->nodeId_ << " detached from the network" << endl;
    else
        EV << NOW << " NrPhyUe::doHandover - UE " << phy_->nodeId_ << " has completed handover to eNB " << phy_->masterId_ << "... " << endl;

    phy_->binder_->removeUeHandoverTriggered(phy_->nodeId_);
    phy_->binder_->removeHandoverTriggered(phy_->nodeId_);

    // inform the UE's Ip2Nic module to forward held packets
    phy_->ip2nic_->signalHandoverCompleteUe(phy_->isNr_);

    // inform the eNB's Ip2Nic module to forward data to the target eNB
    if (oldMaster != NODEID_NONE && phy_->candidateMasterId_ != NODEID_NONE) {
        Ip2Nic *enbIp2nic = check_and_cast<Ip2Nic *>(phy_->binder_->getNodeModule(phy_->masterId_)->getSubmodule("cellularNic")->getSubmodule("ip2nic"));
        enbIp2nic->signalHandoverCompleteTarget(phy_->nodeId_, oldMaster);
    }

}

void HandoverController::NrPhyUe_forceHandover(MacNodeId targetMasterNode, double targetMasterRssi)
{
    phy_->candidateMasterId_ = targetMasterNode;
    phy_->candidateMasterRssi_ = targetMasterRssi;
    phy_->hysteresisTh_ = updateHysteresisTh(phy_->currentMasterRssi_);

    phy_->cancelEvent(phy_->handoverStarter_);  // if any
    phy_->scheduleAt(NOW, phy_->handoverStarter_);
}

void HandoverController::LtePhyUe_deleteOldBuffers(MacNodeId masterId)
{
    // Delete Mac Buffers

    // delete macBuffer[nodeId_] at old master
    LteMacEnb *masterMac = check_and_cast<LteMacEnb *>(phy_->binder_->getMacByNodeId(masterId));
    masterMac->deleteQueues(phy_->nodeId_);

    // delete queues for master at this ue
    phy_->mac_->deleteQueues(phy_->masterId_);

    // Delete Rlc UM Buffers

    // delete UmTxQueue[nodeId_] at old master
    LteRlcUm *masterRlcUm = check_and_cast<LteRlcUm *>(phy_->binder_->getRlcByNodeId(masterId, UM));
    masterRlcUm->deleteQueues(phy_->nodeId_);

    // delete queues for master at this ue
    phy_->rlcUm_->deleteQueues(phy_->nodeId_);

    // Delete PDCP Entities
    // delete pdcpEntities[nodeId_] at old master
    LtePdcpEnb *masterPdcp = check_and_cast<LtePdcpEnb *>(phy_->binder_->getPdcpByNodeId(masterId));
    masterPdcp->deleteEntities(phy_->nodeId_);

    // delete queues for master at this ue
    phy_->pdcp_->deleteEntities(phy_->masterId_);
}

void HandoverController::NrPhyUe_deleteOldBuffers(MacNodeId masterId)
{
    // Delete MAC Buffers

    // delete macBuffer[nodeId_] at old master
    LteMacEnb *masterMac = check_and_cast<LteMacEnb *>(phy_->binder_->getMacByNodeId(masterId));
    masterMac->deleteQueues(phy_->nodeId_);

    // delete queues for master at this UE
    phy_->mac_->deleteQueues(phy_->masterId_);

    // Delete RLC UM Buffers

    // delete UmTxQueue[nodeId_] at old master
    LteRlcUm *masterRlcUm = check_and_cast<LteRlcUm *>(phy_->binder_->getRlcByNodeId(masterId, UM));
    masterRlcUm->deleteQueues(phy_->nodeId_);

    // delete queues for master at this UE
    phy_->rlcUm_->deleteQueues(phy_->nodeId_);

    // Delete PDCP Entities
    // delete pdcpEntities[nodeId_] at old master
    // in case of NR dual connectivity, the master can be a secondary node, hence we have to delete PDCP entities residing in the node's master
    MacNodeId masterNodeId = phy_->binder_->getMasterNodeOrSelf(masterId);
    LtePdcpEnb *masterPdcp = check_and_cast<LtePdcpEnb *>(phy_->binder_->getPdcpByNodeId(masterNodeId));
    masterPdcp->deleteEntities(phy_->nodeId_);

    // delete queues for master at this UE
    phy_->pdcp_->deleteEntities(phy_->masterId_);
}

double HandoverController::updateHysteresisTh(double v)
{
    if (phy_->hysteresisFactor_ == 0)
        return 0;
    else
        return v / phy_->hysteresisFactor_;
}


} //namespace
