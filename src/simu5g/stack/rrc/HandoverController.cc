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

#include "simu5g/stack/ip2nic/HandoverPacketHolderUe.h"
#include "simu5g/stack/ip2nic/HandoverPacketHolderEnb.h"
#include "simu5g/stack/phy/PhyUe.h"
#include "simu5g/stack/rrc/BearerManagement.h"
#include "simu5g/stack/phy/feedback/LteDlFeedbackGenerator.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/common/InitStages.h"
#include "simu5g/stack/mac/LteMacUe.h"
#include "simu5g/stack/mac/LteMacEnb.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(HandoverController);

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
        bearerManagement_ = inet::getModuleFromPar<BearerManagement>(par("bearerManagementModule"), this);
        handoverPacketHolder_.reference(this, "handoverPacketHolderModule", true);
        fbGen_.reference(this, "feedbackGeneratorModule", true);
        otherHandoverController_.reference(this, "otherHandoverControllerModule", false);
        bearerConfigurator_.reference(this, "bearerConfiguratorModule", true);

        isNr_ = par("isNr");
        nodeId_ = MacNodeId(par("macNodeId").intValue());

        enableHandover_ = par("enableHandover");
        handoverDetachmentTime_ = par("handoverDetachmentTime").doubleValue();
        handoverAttachmentTime_ = par("handoverAttachmentTime").doubleValue();
        hysteresisFactor_ = par("hysteresisFactor").doubleValue();

        if (par("minRssiDefault").boolValue())
            minRssi_ = binder_->phyPisaData.minSnr();
        else
            minRssi_ = par("minRssi").doubleValue();

        hasCollector = par("hasCollector");

        handoverStarter_ = new cMessage("handoverStarter");

        WATCH(servingNodeId_);
        WATCH(candidateServingNodeId_);
        WATCH(servingNodeRssi_);
        WATCH(candidateServingNodeRssi_);
        WATCH(hysteresisThreshold_);
        WATCH(hysteresisFactor_);
        WATCH(handoverDelta_);
        WATCH(handoverDetachmentTime_);
        WATCH(handoverAttachmentTime_);
        WATCH(minRssi_);
        WATCH(enableHandover_);

    }
    else if (stage == INITSTAGE_SIMU5G_PHYSICAL_LAYER) {
        // get serving cell from configuration
        servingNodeId_ = binder_->getServingNode(nodeId_);
        candidateServingNodeId_ = servingNodeId_;

        // find the best candidate cell
        bool dynamicCellAssociation = par("dynamicCellAssociation").boolValue();
        if (dynamicCellAssociation) {
            phy_->findCandidateEnb(candidateServingNodeId_, candidateServingNodeRssi_);

            // Keep the configured serving cell if no candidate was found, otherwise the UE
            // would be left detached while the binder still records the configured cell.
            if (candidateServingNodeId_ != NODEID_NONE) {
                // binder calls, if dynamicCellAssociation selected a different cell
                if (candidateServingNodeId_ != servingNodeId_) {
                    binder_->unregisterServingNode(servingNodeId_, nodeId_);
                    binder_->registerServingNode(candidateServingNodeId_, nodeId_);
                }
                servingNodeId_ = candidateServingNodeId_;
                servingNodeRssi_ = candidateServingNodeRssi_;
            }
        }

        EV << "HandoverController::initialize - Attaching to serving node " << servingNodeId_ << endl;

        phy_->changeServingNode(servingNodeId_);
        emit(servingCellSignal_, (long)servingNodeId_);
    }
}

void HandoverController::finish()
{
    if (getSimulation()->getSimulationStage() != CTX_FINISH) {
        // do this only during the deletion of the module during the simulation

        // do this only if this PHY layer is connected to a serving base station
        if (servingNodeId_ != NODEID_NONE) {
            // clear buffers
            deleteOldBuffers(servingNodeId_, /*localNodeIsBeingDeleted=*/true);

            // amc calls
            LteAmc *amc = getAmcModule(servingNodeId_);
            if (amc != nullptr) {
                amc->detachUser(nodeId_, UL);
                amc->detachUser(nodeId_, DL);
            }

            onNodeLeaving();

            // binder call
            binder_->unregisterServingNode(servingNodeId_, nodeId_);
        }
    }
}

void HandoverController::handleMessage(cMessage *msg)
{
    ASSERT(msg->isSelfMessage());

    if (msg->isName("handoverStarter"))
        triggerHandover();
    else if (msg->isName("handoverTrigger")) {
        doHandover();
        delete msg;
        handoverTrigger_ = nullptr;
    }
    else
        throw cRuntimeError("HandoverController::handleMessage: unknown self-message '%s'", msg->getName());
}

// Handover lifecycle hooks: no-ops in the core controller. The D2D behavior lives
// in HandoverControllerD2D, which the D2D-capable UE NICs select.
void HandoverController::onHandoverCompleted()
{
}

void HandoverController::onHandoverStarting()
{
}

void HandoverController::onHandoverExecuting()
{
}

void HandoverController::onNodeLeaving()
{
}

void HandoverController::beaconReceived(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    Enter_Method("beaconReceived");
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

    // Dual-stack UE: check if the beacon comes from a DC Secondary node
    if (hasOtherLeg()) {
        MacNodeId sourceId = lteInfo->getSourceId();
        MacNodeId masterNodeId = binder_->getMasterNodeOrSelf(sourceId);
        if (masterNodeId != sourceId) {
            // The node has a DC Master node, check if the other PHY of this UE is attached to that Master.
            // If not, the UE cannot attach to this Secondary node and the packet must be deleted.
            if (otherHandoverController_->getServingNodeId() != masterNodeId) {
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

    if (lteInfo->getSourceId() != servingNodeId_ && rssi < minRssi_) {
        EV << "Signal from candidate too weak - minRssi[" << minRssi_ << "]" << endl;
        delete frame;
        return;
    }

    if (rssi > candidateServingNodeRssi_ + hysteresisThreshold_) {
        if (lteInfo->getSourceId() == servingNodeId_) {
            // receiving even stronger broadcast from current serving node
            servingNodeRssi_ = rssi;
            candidateServingNodeId_ = servingNodeId_;
            candidateServingNodeRssi_ = rssi;
            updateHysteresisThreshold(servingNodeRssi_);
            cancelEvent(handoverStarter_);
        }
        else {
            // broadcast from another serving node with higher RSSI
            candidateServingNodeId_ = lteInfo->getSourceId();
            candidateServingNodeRssi_ = rssi;
            updateHysteresisThreshold(rssi);
            binder_->addHandoverTriggered(nodeId_, servingNodeId_, candidateServingNodeId_);

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
        if (lteInfo->getSourceId() == servingNodeId_) {
            if (rssi >= minRssi_) {
                servingNodeRssi_ = rssi;
                candidateServingNodeRssi_ = rssi;
                updateHysteresisThreshold(rssi);
            }
            else { // lost connection with current serving node
                if (candidateServingNodeId_ == servingNodeId_) { // trigger detachment
                    candidateServingNodeId_ = NODEID_NONE;
                    servingNodeRssi_ = -999.0;
                    candidateServingNodeRssi_ = -999.0; // set candidate RSSI very bad as we currently do not have any.
                                                   // this ensures that each candidate with is at least as 'bad'
                                                   // as the minRssi_ has a chance.

                    updateHysteresisThreshold(0);
                    binder_->addHandoverTriggered(nodeId_, servingNodeId_, candidateServingNodeId_);

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
    // NR legs exist only on dual-stack UEs
    if (!hasOtherLeg())
        ASSERT(!isNr_);

    // Dual-stack UE: check for dual connectivity scenarios with early returns
    if (hasOtherLeg()) {
        MacNodeId masterNode = binder_->getMasterNodeOrSelf(candidateServingNodeId_);
        if (masterNode != candidateServingNodeId_) { // The candidate is a secondary node
            if (otherHandoverController_->getServingNodeId() == masterNode) {
                MacNodeId otherNodeId = otherHandoverController_->getNodeId();
                const std::pair<MacNodeId, MacNodeId> *handoverPair = binder_->getHandoverTriggered(otherNodeId);
                if (handoverPair != nullptr) {
                    if (handoverPair->second == candidateServingNodeId_) {
                        // Delay this handover
                        double delta = handoverDelta_;
                        if (handoverPair->first != NODEID_NONE) // The other "stack" is performing a complete handover
                            delta += handoverDetachmentTime_ + handoverAttachmentTime_;
                        else                                                   // The other "stack" is attaching to an eNodeB
                            delta += handoverAttachmentTime_;

                        EV << NOW << " HandoverController::triggerHandover - Wait for the handover completion for the other stack. Delay this handover." << endl;

                        // Need to wait for the other stack to complete handover
                        scheduleAt(simTime() + delta, handoverStarter_);
                        return;
                    }
                    else {
                        // Cancel this handover
                        binder_->removeHandoverTriggered(nodeId_);
                        EV << NOW << " HandoverController::triggerHandover - UE " << nodeId_ << " is canceling its handover to eNB " << candidateServingNodeId_ << " since the master is performing handover" << endl;
                        return;
                    }
                }
            }
        }

        if (otherHandoverController_->getServingNodeId() != NODEID_NONE) {
            // Check if there are secondary nodes connected
            MacNodeId otherMasterId = binder_->getMasterNodeOrSelf(otherHandoverController_->getServingNodeId());
            if (otherMasterId == servingNodeId_) {
                EV << NOW << " HandoverController::triggerHandover - Forcing detachment from " << otherHandoverController_->getServingNodeId() << " which was a secondary node to " << servingNodeId_ << ". Delay this handover." << endl;

                // Need to wait for the other stack to complete detachment
                scheduleAt(simTime() + handoverDetachmentTime_ + handoverDelta_, handoverStarter_);

                // The other stack is connected to a node which is a secondary node of the master from which this stack is leaving:
                // Trigger detachment
                otherHandoverController_->forceHandover();

                return;
            }
        }
    }

    onHandoverStarting();

    // On a dual-stack UE either leg can legitimately be detached; a single-stack UE is always attached
    if (hasOtherLeg())
        ASSERT(servingNodeId_ == NODEID_NONE || servingNodeId_ != candidateServingNodeId_);  // "we can be unattached, but never hand over to ourselves"
    else
        ASSERT(servingNodeId_ != candidateServingNodeId_);

    EV << "####Handover starting:####" << endl;
    EV << "Current serving node: " << servingNodeId_ << endl;
    EV << "Current RSSI: " << servingNodeRssi_ << endl;
    EV << "Candidate serving node: " << candidateServingNodeId_ << endl;  // note: can be NODEID_NONE!
    EV << "Candidate RSSI: " << candidateServingNodeRssi_ << endl;
    EV << "############" << endl;

    // Status messages
    if (candidateServingNodeRssi_ == 0)
        EV << NOW << " HandoverController::triggerHandover - UE " << nodeId_ << " lost its connection to eNB " << servingNodeId_ << ". Now detaching... " << endl;
    else if (servingNodeId_ == NODEID_NONE)
        EV << NOW << " HandoverController::triggerHandover - UE " << nodeId_ << " is starting attachment procedure to eNB " << candidateServingNodeId_ << "... " << endl;
    else
        EV << NOW << " HandoverController::triggerHandover - UE " << nodeId_ << " is starting handover to eNB " << candidateServingNodeId_ << "... " << endl;

    // Add UE handover trigger
    binder_->addUeHandoverTriggered(nodeId_);

    // RRC's stack attachment ledger changes the instant the handover begins, ahead of
    // its execution: packets steered from now on must already see the new attachment
    // (see BearerManagement::pushServingNodeIds())
    if (isNr_)
        bearerManagement_->setNrServingNodeId(candidateServingNodeId_);
    else
        bearerManagement_->setServingNodeId(candidateServingNodeId_);

    // Inform the UE's HandoverPacketHolder module to start holding downstream packets
    handoverPacketHolder_->triggerHandoverUe(candidateServingNodeId_);

    // Single-stack UE: no other leg reads the handoverTriggered record, so remove it right away.
    // (Dual-stack UEs keep it until doHandover(), so the other leg can see the handover in progress.)
    if (!hasOtherLeg())
        binder_->removeHandoverTriggered(nodeId_);

    // Inform the eNB's HandoverPacketHolder module to forward data to the target eNB,
    // on the downlink tunnel the handover preparation gives the UE's PDU session there
    if (servingNodeId_ != NODEID_NONE && candidateServingNodeId_ != NODEID_NONE) {
        bearerConfigurator_->prepareHandover(nodeId_, candidateServingNodeId_);
        HandoverPacketHolderEnb *enbIp2nic = check_and_cast<HandoverPacketHolderEnb *>(binder_->getHandoverPacketHolderByNodeId(servingNodeId_));
        enbIp2nic->triggerHandoverSource(nodeId_, candidateServingNodeId_);
    }

    // Calculate handover latency and schedule trigger message
    double handoverLatency;
    if (servingNodeId_ == NODEID_NONE)                                                // attachment only
        handoverLatency = handoverAttachmentTime_;
    else if (candidateServingNodeId_ == NODEID_NONE)                                                // detachment only
        handoverLatency = handoverDetachmentTime_;
    else                                                // complete handover time
        handoverLatency = handoverDetachmentTime_ + handoverAttachmentTime_;

    handoverTrigger_ = new cMessage("handoverTrigger");
    scheduleAt(simTime() + handoverLatency, handoverTrigger_);
}

void HandoverController::doHandover()
{
    // if currentServingNodeId_ == 0, it means the UE was not attached to any eNodeB, so it only has to perform attachment procedures
    // if candidateServingNodeId_ == 0, it means the UE is detaching from its eNodeB, so it only has to perform detachment procedures

    onHandoverExecuting();

    // Delete old buffers and detach/attach from AMC
    if (servingNodeId_ != NODEID_NONE) {
        // Delete Old Buffers
        deleteOldBuffers(servingNodeId_);

        // AMC calls
        LteAmc *oldAmc = getAmcModule(servingNodeId_);
        oldAmc->detachUser(nodeId_, UL);
        oldAmc->detachUser(nodeId_, DL);
    }

    if (candidateServingNodeId_ != NODEID_NONE) {
        LteAmc *newAmc = getAmcModule(candidateServingNodeId_);
        newAmc->attachUser(nodeId_, UL);
        newAmc->attachUser(nodeId_, DL);
    }

    // Binder calls
    if (servingNodeId_ != NODEID_NONE)
        binder_->unregisterServingNode(servingNodeId_, nodeId_);

    if (candidateServingNodeId_ != NODEID_NONE) {
        binder_->registerServingNode(candidateServingNodeId_, nodeId_);
    }
    binder_->updateUeInfoCellId(nodeId_, candidateServingNodeId_);

    // Move collector (if configured)
    if (hasCollector) {
        binder_->moveUeCollector(nodeId_, servingNodeId_, candidateServingNodeId_);
    }

    // Change masterId and notify handover to the MAC layer
    MacNodeId oldServingNodeId = servingNodeId_;
    servingNodeId_ = candidateServingNodeId_;
    phy_->changeServingNode(servingNodeId_);

    mac_->doHandover(candidateServingNodeId_);  // do MAC operations for handover
    servingNodeRssi_ = candidateServingNodeRssi_;
    updateHysteresisThreshold(servingNodeRssi_);

    // Update DL feedback generator
    fbGen_->handleHandover(servingNodeId_);

    // Collect stat
    emit(servingCellSignal_, (long)servingNodeId_);

    EV << NOW << " " << getClassName() << "::doHandover - UE " << nodeId_ << " has completed handover to eNB " << servingNodeId_ << "... " << endl;

    // Remove UE handover triggered
    binder_->removeUeHandoverTriggered(nodeId_);

    // Dual-stack UE: the handoverTriggered record was kept alive for the other leg's
    // benefit (see triggerHandover()); remove it now that the handover has completed
    if (hasOtherLeg())
        binder_->removeHandoverTriggered(nodeId_);

    // Inform the UE's HandoverPacketHolder module to forward held packets
    handoverPacketHolder_->signalHandoverCompleteUe(isNr_);

    // Inform the eNB's HandoverPacketHolder module to forward data to the target eNB
    if (oldServingNodeId != NODEID_NONE && candidateServingNodeId_ != NODEID_NONE) {
        HandoverPacketHolderEnb *enbIp2nic = check_and_cast<HandoverPacketHolderEnb *>(binder_->getHandoverPacketHolderByNodeId(servingNodeId_));
        enbIp2nic->signalHandoverCompleteTarget(nodeId_, oldServingNodeId);
    }
}

void HandoverController::forceHandover()
{
    candidateServingNodeId_ = NODEID_NONE;
    candidateServingNodeRssi_ = 0.0;
    updateHysteresisThreshold(servingNodeRssi_);

    cancelEvent(handoverStarter_);  // if any
    scheduleAt(NOW, handoverStarter_);
}

void HandoverController::deleteOldBuffers(MacNodeId servingNodeId, bool localNodeIsBeingDeleted)
{
    // Delete MAC Buffers

    // delete macBuffer[nodeId_] at old serving node
    LteMacEnb *servingNodeMac = check_and_cast<LteMacEnb *>(binder_->getMacByNodeId(servingNodeId));
    servingNodeMac->deleteQueues(nodeId_);

    // delete queues for serving node at this UE
    mac_->deleteQueues(servingNodeId);

    // Delete RLC entities

    // delete RLC entities for nodeId_ at old serving node
    BearerManagement *servingBm = check_and_cast<BearerManagement *>(binder_->getRrcByNodeId(servingNodeId)->getSubmodule("bearerManagement"));
    servingBm->deleteLocalRlcQueues(nodeId_, isNr_);

    // delete RLC entities for serving node at this UE. Keyed by the serving node, matching
    // both the comment and the deleteLocalPdcpEntities(servingNodeId) call below; passing
    // nodeId_ (this UE's own id) used to work only because the UE side ignored the argument
    // and deleted every entity.
    if (!localNodeIsBeingDeleted)
        bearerManagement_->deleteLocalRlcQueues(servingNodeId, isNr_);

    // Delete PDCP Entities
    // delete pdcpEntities[nodeId_] at old serving node
    // In case of NR dual connectivity, the master can be a secondary node, hence we have to delete PDCP entities residing in the node's master
    MacNodeId pdcpNodeId = binder_->getMasterNodeOrSelf(servingNodeId);
    BearerManagement *masterBm = check_and_cast<BearerManagement *>(binder_->getRrcByNodeId(pdcpNodeId)->getSubmodule("bearerManagement"));
    masterBm->deleteLocalPdcpEntities(nodeId_);

    // If the old serving node is a DC secondary, also delete the per-UE bypass PDCP entities
    // residing on the secondary itself (keyed by this UE's id there) -- the master-side call
    // above does not reach them
    if (pdcpNodeId != servingNodeId)
        servingBm->deleteLocalPdcpEntities(nodeId_);

    // delete PDCP entities for serving node at this UE
    if (!localNodeIsBeingDeleted)
        bearerManagement_->deleteLocalPdcpEntities(servingNodeId);

    // Flow establishment (BearerConfigurator::establishDataConnection) provisions NR-leg
    // entities for this UE at the serving node's DC secondary regardless of whether the UE's
    // NR leg is attached to it. If the NR leg is attached, its own (forced) detachment cleans
    // them up; if it is detached now, remove them together with the master-side state --
    // otherwise they are orphaned here, and a later re-establishment collides with them when
    // the UE returns to this master (duplicate MAC CID assert / duplicate module errors).
    if (otherHandoverController_ != nullptr && otherHandoverController_->getServingNodeId() == NODEID_NONE) {
        MacNodeId secondaryNodeId = binder_->getSecondaryNode(servingNodeId);
        if (secondaryNodeId != NODEID_NONE)
            otherHandoverController_->deleteOldBuffers(secondaryNodeId, localNodeIsBeingDeleted);
    }
}

LteAmc *HandoverController::getAmcModule(MacNodeId nodeId)
{
    return check_and_cast<LteMacEnb *>(binder_->getMacFromMacNodeId(nodeId))->getAmc();
}

void HandoverController::updateHysteresisThreshold(double rssi)
{
    hysteresisThreshold_ = (hysteresisFactor_ == 0) ? 0 : rssi / hysteresisFactor_;
}

} //namespace
