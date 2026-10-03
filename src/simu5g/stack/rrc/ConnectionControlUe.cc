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

#include "ConnectionControlUe.h"

#include "simu5g/stack/ip2nic/HandoverPacketHolderUe.h"
#include "simu5g/stack/phy/PhyUe.h"
#include "simu5g/stack/rrc/BearerManagement.h"
#include "simu5g/stack/rrc/ConnectionControlEnb.h"
#include "simu5g/stack/phy/feedback/LteDlFeedbackGenerator.h"
#include <inet/common/ModuleAccess.h>
#include "simu5g/common/binder/Binder.h"
#include "simu5g/common/InitStages.h"
#include "simu5g/stack/mac/LteMacUe.h"
#include "simu5g/stack/mac/LteMacEnb.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(ConnectionControlUe);

simsignal_t ConnectionControlUe::servingCellSignal_ = registerSignal("servingCell");

ConnectionControlUe::~ConnectionControlUe()
{
    cancelAndDelete(handoverStarter_);
    cancelAndDelete(handoverTrigger_);
}

void ConnectionControlUe::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        binder_.reference(this, "binderModule", true);
        mac_.reference(this, "macModule", true);
        bearerManagement_ = inet::getModuleFromPar<BearerManagement>(par("bearerManagementModule"), this);
        handoverPacketHolder_.reference(this, "handoverPacketHolderModule", true);
        fbGen_.reference(this, "feedbackGeneratorModule", true);
        otherConnectionControl_.reference(this, "otherConnectionControlModule", false);

        isNr_ = par("isNr");
        nodeId_ = MacNodeId(par("macNodeId").intValue());
        sessionType_ = aToSessionType(par("sessionType").stdstringValue());

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

        EV << "ConnectionControlUe::initialize - Attaching to serving node " << servingNodeId_ << endl;

        phy_->changeServingNode(servingNodeId_);
        emit(servingCellSignal_, (long)servingNodeId_);
    }
    else if (stage == inet::INITSTAGE_LAST) {
        // D2D communication is between IP peers; the non-IP sessions are not supported
        // on a leg that takes part in it
        if (!isIpSessionType(sessionType_) && getCapabilities().d2d)
            throw cRuntimeError("A \"%s\" session is not supported on a D2D-capable UE (see the sessionType and hasD2D parameters of the UE)",
                    sessionTypeToA(sessionType_).c_str());

        // The RRC connection: a leg attached at initialization requests it from its
        // serving base station, which registers the UE with the core network and has
        // the session's resources and the static bearers set up (see
        // ConnectionControlEnb::connectionSetupRequest()). The UEs do this in module order,
        // after the network-level modules' last stage.
        if (servingNodeId_ != NODEID_NONE)
            baseStationControl(servingNodeId_)->connectionSetupRequest(inet::getContainingNode(this), nodeId_, this, getCapabilities(), sessionType_);
    }
}

void ConnectionControlUe::finish()
{
    if (getSimulation()->getSimulationStage() != CTX_FINISH) {
        // do this only during the deletion of the module during the simulation

        // do this only if this PHY layer is connected to a serving base station
        if (servingNodeId_ != NODEID_NONE) {
            // The UE's own entities are left alone: they are submodules of the NIC
            // that is about to be destroyed anyway, and deleting them here would
            // mutate the submodule list that OMNeT++'s callFinish() is enumerating.
            // The base station releases its own state for the leg.
            deleteOwnBuffers(servingNodeId_, /*localNodeIsBeingDeleted=*/true);
            baseStationControl(servingNodeId_)->connectionLost(nodeId_);

            // binder call
            binder_->unregisterServingNode(servingNodeId_, nodeId_);
        }
    }
}

void ConnectionControlUe::handleMessage(cMessage *msg)
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
        throw cRuntimeError("ConnectionControlUe::handleMessage: unknown self-message '%s'", msg->getName());
}

void ConnectionControlUe::beaconReceived(LteAirFrame *frame, UserControlInfo *lteInfo)
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
            if (otherConnectionControl_->getServingNodeId() != masterNodeId) {
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
            triggeredHandover_ = std::make_pair(servingNodeId_, candidateServingNodeId_);

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
                    triggeredHandover_ = std::make_pair(servingNodeId_, candidateServingNodeId_);

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

void ConnectionControlUe::triggerHandover()
{
    // NR legs exist only on dual-stack UEs
    if (!hasOtherLeg())
        ASSERT(!isNr_);

    // Dual-stack UE: check for dual connectivity scenarios with early returns
    if (hasOtherLeg()) {
        MacNodeId masterNode = binder_->getMasterNodeOrSelf(candidateServingNodeId_);
        if (masterNode != candidateServingNodeId_) { // The candidate is a secondary node
            if (otherConnectionControl_->getServingNodeId() == masterNode) {
                const std::pair<MacNodeId, MacNodeId> *handoverPair = otherConnectionControl_->getTriggeredHandover();
                if (handoverPair != nullptr) {
                    if (handoverPair->second == candidateServingNodeId_) {
                        // Delay this handover
                        double delta = handoverDelta_;
                        if (handoverPair->first != NODEID_NONE) // The other "stack" is performing a complete handover
                            delta += handoverDetachmentTime_ + handoverAttachmentTime_;
                        else                                                   // The other "stack" is attaching to an eNodeB
                            delta += handoverAttachmentTime_;

                        EV << NOW << " ConnectionControlUe::triggerHandover - Wait for the handover completion for the other stack. Delay this handover." << endl;

                        // Need to wait for the other stack to complete handover
                        scheduleAt(simTime() + delta, handoverStarter_);
                        return;
                    }
                    else {
                        // Cancel this handover
                        triggeredHandover_.reset();
                        EV << NOW << " ConnectionControlUe::triggerHandover - UE " << nodeId_ << " is canceling its handover to eNB " << candidateServingNodeId_ << " since the master is performing handover" << endl;
                        return;
                    }
                }
            }
        }

        if (otherConnectionControl_->getServingNodeId() != NODEID_NONE) {
            // Check if there are secondary nodes connected
            MacNodeId otherMasterId = binder_->getMasterNodeOrSelf(otherConnectionControl_->getServingNodeId());
            if (otherMasterId == servingNodeId_) {
                EV << NOW << " ConnectionControlUe::triggerHandover - Forcing detachment from " << otherConnectionControl_->getServingNodeId() << " which was a secondary node to " << servingNodeId_ << ". Delay this handover." << endl;

                // Need to wait for the other stack to complete detachment
                scheduleAt(simTime() + handoverDetachmentTime_ + handoverDelta_, handoverStarter_);

                // The other stack is connected to a node which is a secondary node of the master from which this stack is leaving:
                // Trigger detachment
                otherConnectionControl_->forceHandover();

                return;
            }
        }
    }

    // The handover decision is the serving base station's: report the measurements
    // and let it command the handover (handoverCommand()), which runs the rest.
    // Attachment from nowhere and detachment are the UE's own.
    if (servingNodeId_ != NODEID_NONE && candidateServingNodeId_ != NODEID_NONE) {
        MeasurementReport report{servingNodeId_, servingNodeRssi_, candidateServingNodeId_, candidateServingNodeRssi_};
        baseStationControl(servingNodeId_)->measurementReport(nodeId_, report);
        return;
    }
    startHandover();
}

void ConnectionControlUe::handoverCommand(MacNodeId targetNodeId)
{
    Enter_Method("handoverCommand");
    ASSERT(targetNodeId != NODEID_NONE && servingNodeId_ != NODEID_NONE);
    candidateServingNodeId_ = targetNodeId;
    startHandover();
}

void ConnectionControlUe::radioLinkFailure(MacNodeId bsId)
{
    Enter_Method("radioLinkFailure");
    bearerManagement_->releaseLink(bsId);
}

void ConnectionControlUe::radioLinkFailure(MacNodeId localId, MacNodeId peerId)
{
    Enter_Method("radioLinkFailure");
    baseStationControl(peerId)->radioLinkFailure(localId);
}

void ConnectionControlUe::startHandover()
{
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
        EV << NOW << " ConnectionControlUe::triggerHandover - UE " << nodeId_ << " lost its connection to eNB " << servingNodeId_ << ". Now detaching... " << endl;
    else if (servingNodeId_ == NODEID_NONE)
        EV << NOW << " ConnectionControlUe::triggerHandover - UE " << nodeId_ << " is starting attachment procedure to eNB " << candidateServingNodeId_ << "... " << endl;
    else
        EV << NOW << " ConnectionControlUe::triggerHandover - UE " << nodeId_ << " is starting handover to eNB " << candidateServingNodeId_ << "... " << endl;

    // RRC's stack attachment ledger changes the instant the handover begins, ahead of
    // its execution: packets steered from now on must already see the new attachment
    // (see BearerManagement::pushServingNodeIds())
    if (isNr_)
        bearerManagement_->setNrServingNodeId(candidateServingNodeId_);
    else
        bearerManagement_->setServingNodeId(candidateServingNodeId_);

    // Inform the UE's HandoverPacketHolder module to start holding downstream packets
    handoverPacketHolder_->triggerHandoverUe(candidateServingNodeId_);

    // Single-stack UE: no other leg reads the triggered handover, so forget it right away.
    // (Dual-stack UEs keep it until doHandover(), so the other leg can see the handover in progress.)
    if (!hasOtherLeg())
        triggeredHandover_.reset();

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

void ConnectionControlUe::doHandover()
{
    // if currentServingNodeId_ == 0, it means the UE was not attached to any eNodeB, so it only has to perform attachment procedures
    // if candidateServingNodeId_ == 0, it means the UE is detaching from its eNodeB, so it only has to perform detachment procedures

    // The UE's own state toward the old serving node goes; the base station releases
    // its own when told below (ueContextRelease, connectionLost)
    if (servingNodeId_ != NODEID_NONE)
        deleteOwnBuffers(servingNodeId_);

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

    // Dual-stack UE: the triggered handover was kept for the other leg's benefit (see
    // triggerHandover()); forget it now that the handover has completed
    if (hasOtherLeg())
        triggeredHandover_.reset();

    // Inform the UE's HandoverPacketHolder module to forward held packets
    handoverPacketHolder_->signalHandoverCompleteUe(isNr_);

    // The network side: the target completes the handover (RRCReconfigurationComplete:
    // it takes the leg on, switches the path, releases the source), or the new base
    // station takes the attaching leg on (RRC connection setup), or the old one is told the
    // leg is gone
    if (oldServingNodeId != NODEID_NONE && servingNodeId_ != NODEID_NONE)
        baseStationControl(servingNodeId_)->reconfigurationComplete(nodeId_);
    else if (servingNodeId_ != NODEID_NONE)
        baseStationControl(servingNodeId_)->connectionSetupRequest(inet::getContainingNode(this), nodeId_, this, getCapabilities(), sessionType_);
    else
        baseStationControl(oldServingNodeId)->connectionLost(nodeId_);
}

void ConnectionControlUe::forceHandover()
{
    candidateServingNodeId_ = NODEID_NONE;
    candidateServingNodeRssi_ = 0.0;
    updateHysteresisThreshold(servingNodeRssi_);

    cancelEvent(handoverStarter_);  // if any
    scheduleAt(NOW, handoverStarter_);
}

void ConnectionControlUe::deleteOwnBuffers(MacNodeId servingNodeId, bool localNodeIsBeingDeleted)
{
    // delete queues for serving node at this UE
    mac_->deleteQueues(servingNodeId);

    // delete RLC entities for serving node at this UE. Keyed by the serving node, matching
    // both the comment and the deleteLocalPdcpEntities(servingNodeId) call below; passing
    // nodeId_ (this UE's own id) used to work only because the UE side ignored the argument
    // and deleted every entity.
    if (!localNodeIsBeingDeleted)
        bearerManagement_->deleteLocalRlcQueues(servingNodeId, isNr_);

    // delete PDCP entities for serving node at this UE
    if (!localNodeIsBeingDeleted)
        bearerManagement_->deleteLocalPdcpEntities(servingNodeId);

    // Flow establishment (ConnectionControlEnb::establishBearer) provisions NR-leg
    // entities for this UE at the serving node's DC secondary regardless of whether the UE's
    // NR leg is attached to it. If the NR leg is attached, its own (forced) detachment cleans
    // them up; if it is detached now, remove them together with the master-side state --
    // otherwise they are orphaned here, and a later re-establishment collides with them when
    // the UE returns to this master (duplicate MAC CID assert / duplicate module errors).
    // The base station does the same for its side (see ConnectionControlEnb::releaseLeg()).
    if (otherConnectionControl_ != nullptr && otherConnectionControl_->getServingNodeId() == NODEID_NONE) {
        MacNodeId secondaryNodeId = binder_->getSecondaryNode(servingNodeId);
        if (secondaryNodeId != NODEID_NONE)
            otherConnectionControl_->deleteOwnBuffers(secondaryNodeId, localNodeIsBeingDeleted);
    }
}

void ConnectionControlUe::updateHysteresisThreshold(double rssi)
{
    hysteresisThreshold_ = (hysteresisFactor_ == 0) ? 0 : rssi / hysteresisFactor_;
}

ConnectionControlEnb *ConnectionControlUe::baseStationControl(MacNodeId bsId)
{
    cModule *rrc = bsId != NODEID_NONE ? binder_->getRrcByNodeId(bsId) : nullptr;
    auto *bs = rrc != nullptr ? dynamic_cast<ConnectionControlEnb *>(rrc->getSubmodule("connectionControl")) : nullptr;
    if (bs == nullptr)
        throw cRuntimeError("ConnectionControlUe: UE %d has no base station to ask (base station %d has no rrc.connectionControl)",
                (int)num(nodeId_), (int)num(bsId));
    return bs;
}

ConnectionControlEnb *ConnectionControlUe::baseStationFor(const FlowId& flow)
{
    MacNodeId bsId = getNodeTypeById(flow.destId) == NODEB ? flow.destId : binder_->getServingNode(flow.sourceId);
    return baseStationControl(bsId);
}

DrbId ConnectionControlUe::establishBearer(const FlowId& flow, SessionId session, const FlowBindingKey& key, const inet::Packet *pkt)
{
    Enter_Method_Silent("establishBearer");
    return baseStationFor(flow)->establishBearer(flow, session, key, pkt);
}

DrbId ConnectionControlUe::establishBearer(const FlowId& flow, const BearerRequest& req)
{
    Enter_Method_Silent("establishBearer");
    return baseStationFor(flow)->establishBearer(flow, req);
}

DrbId ConnectionControlUe::resolveDrbForQfi(MacNodeId ueNodeId, SessionId session, Qfi qfi)
{
    Enter_Method_Silent("resolveDrbForQfi");
    return baseStationControl(binder_->getServingNode(ueNodeId))->resolveDrbForQfi(ueNodeId, session, qfi);
}

void ConnectionControlUe::bearerReleased(DrbKey bearer)
{
    Enter_Method_Silent("bearerReleased");
}

void ConnectionControlUe::multicastGroupJoined(MacNodeId nodeId, MacNodeId groupId)
{
    Enter_Method_Silent("multicastGroupJoined");
    // A stack attached nowhere has no base station to tell, and receives nothing
    // until it attaches; the group's senders known by then are provisioned at its
    // serving base station when they start (see ConnectionControlEnbD2D).
    MacNodeId servingNodeId = binder_->getServingNode(nodeId);
    if (servingNodeId == NODEID_NONE) {
        EV_INFO << "ConnectionControlUe: stack " << nodeId << " joined multicast group " << groupId << " while attached nowhere" << endl;
        return;
    }
    baseStationControl(servingNodeId)->multicastGroupJoined(nodeId, groupId);
}

void ConnectionControlUe::configureDrb(const DrbDesc& drb)
{
    Enter_Method("configureDrb");
    // The bearer belongs to the UE's session, whose payload the NIC carries: the NIC of
    // an IP session carries every IP type, that of another type that type only
    bool sameFamily = isIpSessionType(sessionType_) ? isIpSessionType(drb.sessionType) : drb.sessionType == sessionType_;
    if (!sameFamily)
        throw cRuntimeError("ConnectionControlUe: DRB %d is installed for a session of type \"%s\", but the UE requested a \"%s\" session",
                (int)num(drb.getDrbId()), sessionTypeToA(drb.sessionType).c_str(), sessionTypeToA(sessionType_).c_str());
    bearerManagement_->configureDrb(drb);
}

void ConnectionControlUe::createIncomingConnection(const FlowId& flow, const BearerRequest& req, bool withPdcp)
{
    Enter_Method("createIncomingConnection");
    bearerManagement_->createIncomingConnection(flow, req, withPdcp);
}

void ConnectionControlUe::createOutgoingConnection(const FlowId& flow, const BearerRequest& req, bool withPdcp)
{
    Enter_Method("createOutgoingConnection");
    bearerManagement_->createOutgoingConnection(flow, req, withPdcp);
}

void ConnectionControlUe::setUplinkQfiRules(QfiRuleSet&& rules)
{
    Enter_Method("setUplinkQfiRules");
    bearerManagement_->setUplinkQfiRules(std::move(rules));
}

} //namespace
