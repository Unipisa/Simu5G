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
#include "simu5g/stack/ip2nic/Ip2Nic.h"
#include "simu5g/stack/rlc/um/LteRlcUm.h"
#include "simu5g/stack/pdcp/LtePdcp.h"
#include "simu5g/stack/phy/feedback/LteDlFeedbackGenerator.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/common/InitStages.h"
#include "simu5g/stack/mac/LteMacUe.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(HandoverController);

/**
 * Fields still in PHY modules:
 *
 * otherPhy_
 * cellInfo_
 * masterMobility_
 *
 *
 * Fields moved/copied to HandoverController:
 *
 * nodeId_
 * masterId_
 * isNr_
 * hysteresisTh_
 * hysteresisFactor_
 * handoverDelta_
 * handoverAttachment_
 * handoverDetachment_
 * minRssi_
 * enableHandover_
 * mac_
 * binder_
 * ip2nic_
 * fbGen_
 * rlcUm_
 * pdcp_
 */

simsignal_t HandoverController::servingCellSignal_ = registerSignal("servingCell");

HandoverController::~HandoverController()
{
    cancelAndDelete(handoverStarter_);
    cancelAndDelete(handoverTrigger_);
}

void HandoverController::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        binder_.reference(this, "binderModule", true);
        mac_.reference(this, "macModule", true);
        rlcUm_.reference(this, "rlcUmModule", true);
        pdcp_.reference(this, "pdcpModule", true);
        ip2nic_.reference(this, "ip2nicModule", true);
        fbGen_.reference(this, "feedbackGeneratorModule", true);
        otherHandoverController_.reference(this, "otherHandoverControllerModule", false);

        cModule *hostModule = inet::getContainingNode(this);
        isNr_ = dynamic_cast<NrPhyUe*>(phy_) && strcmp(phy_->getFullName(), "nrPhy") == 0; //TODO kludge
        nodeId_ = MacNodeId(hostModule->par(isNr_ ? "nrMacNodeId" : "macNodeId").intValue());

        enableHandover_ = par("enableHandover");
        handoverLatency_ = par("handoverLatency").doubleValue();
        handoverDetachment_ = handoverLatency_ / 2.0;                      // TODO: make this configurable from NED
        handoverAttachment_ = handoverLatency_ - handoverDetachment_;

        if (par("minRssiDefault").boolValue())
            minRssi_ = binder_->phyPisaData.minSnr();
        else
            minRssi_ = par("minRssi").doubleValue();

        hasCollector = par("hasCollector");

        handoverStarter_ = new cMessage("handoverStarter");

        WATCH(masterId_);
        WATCH(candidateMasterId_);
        WATCH(currentMasterRssi_);
        WATCH(candidateMasterRssi_);
        WATCH(hysteresisTh_);
        WATCH(hysteresisFactor_);
        WATCH(handoverDelta_);
        WATCH(handoverLatency_);
        WATCH(handoverDetachment_);
        WATCH(handoverAttachment_);
        WATCH(minRssi_);
        WATCH(enableHandover_);

    }
    else if (stage == INITSTAGE_SIMU5G_PHYSICAL_LAYER) {
        // get serving cell from configuration
        masterId_ = binder_->getServingNode(nodeId_);
        candidateMasterId_ = masterId_;

        // find the best candidate master cell
        bool dynamicCellAssociation = par("dynamicCellAssociation").boolValue();
        if (dynamicCellAssociation) {
            phy_->findCandidateEnb(candidateMasterId_, candidateMasterRssi_);

            // binder calls
            // if dynamicCellAssociation selected a different master
            if (candidateMasterId_ != NODEID_NONE && candidateMasterId_ != masterId_) {
                binder_->unregisterServingNode(masterId_, nodeId_);
                binder_->registerServingNode(candidateMasterId_, nodeId_);
            }
            masterId_ = candidateMasterId_;
            currentMasterRssi_ = candidateMasterRssi_;
            //HandoverCoordinator::updateHysteresisTh(this, candidateMasterRssi_); //TODO was no-op
        }

        EV << "LtePhyUe::initialize - Attaching to eNodeB " << masterId_ << endl;

        phy_->setMasterId(masterId_);
        emit(servingCellSignal_, (long)masterId_);
    }
}

void HandoverController::finish()
{
    if (getSimulation()->getSimulationStage() != CTX_FINISH) {
        // do this only during the deletion of the module during the simulation

        // do this only if this PHY layer is connected to a serving base station
        if (masterId_ != NODEID_NONE) {
            // clear buffers
            deleteOldBuffers(masterId_);

            // amc calls
            LteAmc *amc = phy_->getAmcModule(masterId_);
            if (amc != nullptr) {
                amc->detachUser(nodeId_, UL);
                amc->detachUser(nodeId_, DL);
            }

            // binder call
            binder_->unregisterServingNode(masterId_, nodeId_);
        }
    }
}

void HandoverController::handleMessage(cMessage *msg)
{
    if (msg->isName("handoverStarter"))
        triggerHandover();
    else if (msg->isName("handoverTrigger")) {
        doHandover();
        delete msg;
        handoverTrigger_ = nullptr;
    }
}

void HandoverController::handoverHandler(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    Enter_Method("handoverHandler");
    take(frame);

    if (!enableHandover_) {
        delete frame;
        delete lteInfo;
        return;
    }

    if (handoverTrigger_ != nullptr && handoverTrigger_->isScheduled()) {
        EV << "Handover already in progress, ignoring beacon packet." << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    // Check if the eNodeB is a secondary node
    if (dynamic_cast<NrPhyUe*>(phy_)) {
        MacNodeId sourceId = lteInfo->getSourceId();
        MacNodeId masterNodeId = binder_->getMasterNodeOrSelf(sourceId);
        if (masterNodeId != sourceId) {
            // The node has a master node, check if the other PHY of this UE is attached to that master.
            // If not, the UE cannot attach to this secondary node and the packet must be deleted.
            if (otherHandoverController_->getMasterId() != masterNodeId) {
                EV << "Received beacon packet from " << sourceId << ", which is a secondary node to a master [" << masterNodeId << "] different from the one this UE is attached to. Delete packet." << endl;
                delete lteInfo;
                delete frame;
                return;
            }
        }
    }

    lteInfo->setDestId(nodeId_);
    frame->setControlInfo(lteInfo);

    double rssi = phy_->computeReceivedBeaconPacketRssi(frame, lteInfo);
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

void HandoverController::triggerHandover()
{
    if (dynamic_cast<NrPhyUe*>(phy_))
        NrPhyUe_triggerHandover();
    else if (dynamic_cast<LtePhyUeD2D*>(phy_))
        LtePhyUeD2D_triggerHandover();
    else
        LtePhyUe_triggerHandover();
}

void HandoverController::LtePhyUe_triggerHandover()
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

void HandoverController::LtePhyUeD2D_triggerHandover()
{
    if (masterId_ != NODEID_NONE) {
        // Stop active D2D flows (go back to Infrastructure mode).
        // Currently, DM is possible only for UEs served by the same cell.

        // Trigger D2D mode switch.
        cModule *enb = binder_->getNodeModule(masterId_);
        D2dModeSelectionBase *d2dModeSelection = check_and_cast<D2dModeSelectionBase *>(enb->getSubmodule("cellularNic")->getSubmodule("d2dModeSelection"));
        d2dModeSelection->doModeSwitchAtHandover(nodeId_, false);
    }
    LtePhyUe_triggerHandover();
}

void HandoverController::NrPhyUe_triggerHandover()
{
    ASSERT(masterId_ == NODEID_NONE || masterId_ != candidateMasterId_);  // "we can be unattached, but never hand over to ourselves"

    EV << "####Handover starting:####" << endl;
    EV << "Current master: " << masterId_ << endl;
    EV << "Current RSSI: " << currentMasterRssi_ << endl;
    EV << "Candidate master: " << candidateMasterId_ << endl;  // note: can be NODEID_NONE!
    EV << "Candidate RSSI: " << candidateMasterRssi_ << endl;
    EV << "############" << endl;

    MacNodeId masterNode = binder_->getMasterNodeOrSelf(candidateMasterId_);
    if (masterNode != candidateMasterId_) { // The candidate is a secondary node
        if (otherHandoverController_->getMasterId() == masterNode) {
            MacNodeId otherNodeId = otherHandoverController_->getNodeId();
            const std::pair<MacNodeId, MacNodeId> *handoverPair = binder_->getHandoverTriggered(otherNodeId);
            if (handoverPair != nullptr) {
                if (handoverPair->second == candidateMasterId_) {
                    // Delay this handover
                    double delta = handoverDelta_;
                    if (handoverPair->first != NODEID_NONE) // The other "stack" is performing a complete handover
                        delta += handoverDetachment_ + handoverAttachment_;
                    else                                                   // The other "stack" is attaching to an eNodeB
                        delta += handoverAttachment_;

                    EV << NOW << " NrPhyUe::triggerHandover - Wait for the handover completion for the other stack. Delay this handover." << endl;

                    // Need to wait for the other stack to complete handover
                    scheduleAt(simTime() + delta, handoverStarter_);
                    return;
                }
                else {
                    // Cancel this handover
                    binder_->removeHandoverTriggered(nodeId_);
                    EV << NOW << " NrPhyUe::triggerHandover - UE " << nodeId_ << " is canceling its handover to eNB " << candidateMasterId_ << " since the master is performing handover" << endl;
                    return;
                }
            }
        }
    }
    // Else it is a master itself

    if (candidateMasterRssi_ == 0)
        EV << NOW << " NrPhyUe::triggerHandover - UE " << nodeId_ << " lost its connection to eNB " << masterId_ << ". Now detaching... " << endl;
    else if (masterId_ == NODEID_NONE)
        EV << NOW << " NrPhyUe::triggerHandover - UE " << nodeId_ << " is starting attachment procedure to eNB " << candidateMasterId_ << "... " << endl;
    else
        EV << NOW << " NrPhyUe::triggerHandover - UE " << nodeId_ << " is starting handover to eNB " << candidateMasterId_ << "... " << endl;

    if (otherHandoverController_->getMasterId() != NODEID_NONE) {
        // Check if there are secondary nodes connected
        MacNodeId otherMasterId = binder_->getMasterNodeOrSelf(otherHandoverController_->getMasterId());
        if (otherMasterId == masterId_) {
            EV << NOW << " NrPhyUe::triggerHandover - Forcing detachment from " << otherHandoverController_->getMasterId() << " which was a secondary node to " << masterId_ << ". Delay this handover." << endl;

            // Need to wait for the other stack to complete detachment
            scheduleAt(simTime() + handoverDetachment_ + handoverDelta_, handoverStarter_);

            // The other stack is connected to a node which is a secondary node of the master from which this stack is leaving
            // Trigger detachment (handover to node 0)
            otherHandoverController_->forceHandover(NODEID_NONE, 0.0);

            return;
        }
    }

    binder_->addUeHandoverTriggered(nodeId_);

    // Inform the UE's Ip2Nic module to start holding downstream packets
    ip2nic_->triggerHandoverUe(candidateMasterId_, isNr_);

    // Inform the eNB's Ip2Nic module to forward data to the target eNB
    if (masterId_ != NODEID_NONE && candidateMasterId_ != NODEID_NONE) {
        Ip2Nic *enbIp2nic = check_and_cast<Ip2Nic *>(binder_->getNodeModule(masterId_)->getSubmodule("cellularNic")->getSubmodule("ip2nic"));
        enbIp2nic->triggerHandoverSource(nodeId_, candidateMasterId_);
    }

    if (masterId_ != NODEID_NONE) {
        // Stop active D2D flows (go back to Infrastructure mode)
        // Currently, DM is possible only for UEs served by the same cell

        // Trigger D2D mode switch
        cModule *enb = binder_->getNodeModule(masterId_);
        D2dModeSelectionBase *d2dModeSelection = check_and_cast<D2dModeSelectionBase *>(enb->getSubmodule("cellularNic")->getSubmodule("d2dModeSelection"));
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

void HandoverController::doHandover()
{
    if (dynamic_cast<NrPhyUe*>(phy_))
        NrPhyUe_doHandover();
    else if (dynamic_cast<LtePhyUeD2D*>(phy_))
        LtePhyUeD2D_doHandover();
    else
        LtePhyUe_doHandover();
}

void HandoverController::LtePhyUe_doHandover()
{
    // if masterId_ == 0, it means the UE was not attached to any eNodeB, so it only has to perform attachment procedures
    // if candidateMasterId_ == 0, it means the UE is detaching from its eNodeB, so it only has to perform detachment procedures

    if (masterId_ != NODEID_NONE) {
        // Delete Old Buffers
        LtePhyUe_deleteOldBuffers(masterId_);

        // amc calls
        LteAmc *oldAmc = phy_->getAmcModule(masterId_);
        oldAmc->detachUser(nodeId_, UL);
        oldAmc->detachUser(nodeId_, DL);
    }

    if (candidateMasterId_ != NODEID_NONE) {
        LteAmc *newAmc = phy_->getAmcModule(candidateMasterId_);
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
    phy_->setMasterId(masterId_);

    mac_->doHandover(candidateMasterId_);  // do MAC operations for handover
    currentMasterRssi_ = candidateMasterRssi_;
    hysteresisTh_ = updateHysteresisTh(currentMasterRssi_);

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

void HandoverController::LtePhyUeD2D_doHandover()
{
    // AMC calls.
    if (masterId_ != NODEID_NONE) {
        LteAmc *oldAmc = phy_->getAmcModule(masterId_);
        oldAmc->detachUser(nodeId_, D2D);
    }

    if (candidateMasterId_ != NODEID_NONE) {
        LteAmc *newAmc = phy_->getAmcModule(candidateMasterId_);
        assert(newAmc != nullptr);
        newAmc->attachUser(nodeId_, D2D);
    }

    LtePhyUe_doHandover();

    if (candidateMasterId_ != NODEID_NONE) {
        // Send a self-message to schedule the possible mode switch at the end of the TTI (after all UEs have performed the handover).
        cMessage *msg = new cMessage("doModeSwitchAtHandover");
        msg->setSchedulingPriority(10);
        scheduleAt(NOW, msg);
    }

}

void HandoverController::NrPhyUe_doHandover()
{
    // if masterId_ == 0, it means the UE was not attached to any eNodeB, so it only has to perform attachment procedures
    // if candidateMasterId_ == 0, it means the UE is detaching from its eNodeB, so it only has to perform detachment procedures

    if (masterId_ != NODEID_NONE) {
        // Delete Old Buffers
        NrPhyUe_deleteOldBuffers(masterId_);

        // amc calls
        LteAmc *oldAmc = phy_->getAmcModule(masterId_);
        oldAmc->detachUser(nodeId_, UL);
        oldAmc->detachUser(nodeId_, DL);
        oldAmc->detachUser(nodeId_, D2D);
    }

    if (candidateMasterId_ != NODEID_NONE) {
        LteAmc *newAmc = phy_->getAmcModule(candidateMasterId_);
        assert(newAmc != nullptr);
        newAmc->attachUser(nodeId_, UL);
        newAmc->attachUser(nodeId_, DL);
        newAmc->attachUser(nodeId_, D2D);
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
    phy_->setMasterId(masterId_);

    mac_->doHandover(candidateMasterId_);  // do MAC operations for handover
    currentMasterRssi_ = candidateMasterRssi_;
    hysteresisTh_ = updateHysteresisTh(currentMasterRssi_);

    if (masterId_ != NODEID_NONE) {        // send a self-message to schedule the possible mode switch at the end of the TTI (after all UEs have performed the handover)
        cMessage *msg = new cMessage("doModeSwitchAtHandover");
        msg->setSchedulingPriority(10);
        scheduleAt(NOW, msg);
    }
    // update DL feedback generator
    fbGen_->handleHandover(masterId_);

    // collect stat
    emit(servingCellSignal_, (long)masterId_);

    if (masterId_ == NODEID_NONE)
        EV << NOW << " NrPhyUe::doHandover - UE " << nodeId_ << " detached from the network" << endl;
    else
        EV << NOW << " NrPhyUe::doHandover - UE " << nodeId_ << " has completed handover to eNB " << masterId_ << "... " << endl;

    binder_->removeUeHandoverTriggered(nodeId_);
    binder_->removeHandoverTriggered(nodeId_);

    // inform the UE's Ip2Nic module to forward held packets
    ip2nic_->signalHandoverCompleteUe(isNr_);

    // inform the eNB's Ip2Nic module to forward data to the target eNB
    if (oldMaster != NODEID_NONE && candidateMasterId_ != NODEID_NONE) {
        Ip2Nic *enbIp2nic = check_and_cast<Ip2Nic *>(binder_->getNodeModule(masterId_)->getSubmodule("cellularNic")->getSubmodule("ip2nic"));
        enbIp2nic->signalHandoverCompleteTarget(nodeId_, oldMaster);
    }

}

void HandoverController::forceHandover(MacNodeId targetMasterNode, double targetMasterRssi)
{
    candidateMasterId_ = targetMasterNode;
    candidateMasterRssi_ = targetMasterRssi;
    hysteresisTh_ = updateHysteresisTh(currentMasterRssi_);

    cancelEvent(handoverStarter_);  // if any
    scheduleAt(NOW, handoverStarter_);
}

void HandoverController::deleteOldBuffers(MacNodeId masterId)
{
    if (dynamic_cast<NrPhyUe*>(phy_))
        NrPhyUe_deleteOldBuffers(masterId);
    else
        LtePhyUe_deleteOldBuffers(masterId);
}

void HandoverController::LtePhyUe_deleteOldBuffers(MacNodeId masterId)
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

void HandoverController::NrPhyUe_deleteOldBuffers(MacNodeId masterId)
{
    // Delete MAC Buffers

    // delete macBuffer[nodeId_] at old master
    LteMacEnb *masterMac = check_and_cast<LteMacEnb *>(binder_->getMacByNodeId(masterId));
    masterMac->deleteQueues(nodeId_);

    // delete queues for master at this UE
    mac_->deleteQueues(masterId_);

    // Delete RLC UM Buffers

    // delete UmTxQueue[nodeId_] at old master
    LteRlcUm *masterRlcUm = check_and_cast<LteRlcUm *>(binder_->getRlcByNodeId(masterId, UM));
    masterRlcUm->deleteQueues(nodeId_);

    // delete queues for master at this UE
    rlcUm_->deleteQueues(nodeId_);

    // Delete PDCP Entities
    // delete pdcpEntities[nodeId_] at old master
    // in case of NR dual connectivity, the master can be a secondary node, hence we have to delete PDCP entities residing in the node's master
    MacNodeId masterNodeId = binder_->getMasterNodeOrSelf(masterId);
    LtePdcpEnb *masterPdcp = check_and_cast<LtePdcpEnb *>(binder_->getPdcpByNodeId(masterNodeId));
    masterPdcp->deleteEntities(nodeId_);

    // delete queues for master at this UE
    pdcp_->deleteEntities(masterId_);
}

double HandoverController::updateHysteresisTh(double v)
{
    if (hysteresisFactor_ == 0)
        return 0;
    else
        return v / hysteresisFactor_;
}


} //namespace
