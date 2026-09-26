//
//                  Simu5G
//
// Authors: Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/rrc/BearerManagement.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/stack/rrc/ConnectionControlBase.h"
#include "simu5g/stack/rrc/Registration.h"
#include "simu5g/stack/mac/LteMacBase.h"
#include "simu5g/stack/mac/LteMacEnb.h"
#include "simu5g/stack/ip2nic/Ip2Nic.h"
#include "simu5g/stack/ip2nic/HandoverPacketHolderUe.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/stack/rlc/RlcMux.h"
#include "simu5g/stack/rlc/RlcRxEntityBase.h"
#include "simu5g/stack/rlc/RlcTxEntityBase.h"
#include "simu5g/stack/pdcp/PdcpMux.h"
#include "simu5g/stack/pdcp/DcMux.h"
#include "simu5g/stack/pdcp/DcPdcpLegSplitter.h"
#include "simu5g/stack/pdcp/PdcpTxEntityBase.h"
#include "simu5g/stack/pdcp/PdcpRxEntityBase.h"
#include "simu5g/stack/sdap/NrSdap.h"
#include "simu5g/stack/sdap/QosFlowClassifier.h"
#include "simu5g/common/InitStages.h"

namespace simu5g {

using namespace inet;

Define_Module(BearerManagement);

// Build the FlowDescriptor a bearer-establishment call pushes into MAC and stores as the
// RLC entity's FlowControlInfo prototype. FlowControlInfo carries identity only, so this
// is just flow as a FlowDescriptor; bearer configuration (BearerRequest) travels
// separately (LogicalChannelConfig into MAC, the "rlcMode" NED param into PDCP).
static FlowDescriptor makeFlowDescriptor(const FlowId& flow)
{
    return FlowDescriptor::fromFlowControlInfo(FlowControlInfo::fromFlowId(flow));
}

// Value for the PDCP entity's "rlcMode" NED param (@enum(TM,UM,AM), matching case): unlike
// rlcModeToA() -- lowercase, used only for logging -- this must be an exact enum literal.
static const char *rlcTypeToParamValue(RlcMode type)
{
    switch (type) {
        case TM: return "TM";
        case UM: return "UM";
        case AM: return "AM";
        default: throw cRuntimeError("BearerManagement: invalid rlcMode %d for PDCP entity", (int)type);
    }
}

void BearerManagement::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        registration_ = inet::getModuleFromPar<Registration>(par("registrationModule"), this);

        if (par("headerCompressedSize").intValue() != -1)
            throw cRuntimeError("The headerCompressedSize parameter is no longer supported: header compression (ROHC) is "
                    "configured per bearer, by the \"rohc\" field of the bearer definitions (see BearerConfigurator), "
                    "and the compressed header sizes by the rohcSoHeaderSizes parameter of the PDCP TX entities");

        // Resolve PDCP entity types
        pdcpEntityModuleType_ = cModuleType::get(par("pdcpEntityModuleType").stringValue());
        pdcpRelayEntityModuleType_ = cModuleType::get(par("pdcpRelayEntityModuleType").stringValue());

        // Resolve RLC entity types (compound modules packaging the TX and RX sides);
        // lteRlc* serve LTE-FI bearers, nrRlc* serve NR (SI/SO) bearers
        rlcTmEntityModuleType_ = cModuleType::get(par("rlcTmEntityModuleType").stringValue());
        lteRlcUmEntityModuleType_ = cModuleType::get(par("lteRlcUmEntityModuleType").stringValue());
        lteRlcAmEntityModuleType_ = cModuleType::get(par("lteRlcAmEntityModuleType").stringValue());
        nrRlcUmEntityModuleType_ = cModuleType::get(par("nrRlcUmEntityModuleType").stringValue());
        nrRlcAmEntityModuleType_ = cModuleType::get(par("nrRlcAmEntityModuleType").stringValue());

        nicModule_ = inet::getContainingNicModule(this);

        binderModule.reference(this, "binderModule", true);
        connectionControlModule.reference(this, "connectionControlModule", true);
        drbTableModule.reference(this, "drbTableModule", true);
        rlcMuxModule.reference(this, "rlcMuxModule", true);
        nrRlcMuxModule.reference(this, "nrRlcMuxModule", false);
        macModule.reference(this, "macModule", true);
        nrMacModule.reference(this, "nrMacModule", false);
        sdapModule.reference(this, "sdapModule", false);
        qosFlowClassifierModule.reference(this, "qosFlowClassifierModule", false);
        ip2nicModule_ = inet::findModuleFromPar<Ip2Nic>(par("ip2nicModule"), this);

        t311_ = par("t311");
        t301_ = par("t301");

        dualConnectivityEnabled_ = par("dualConnectivityEnabled");

        WATCH(servingNodeId_);
        WATCH(nrServingNodeId_);
        WATCH(pendingRlf_);
    }
    else if (stage == INITSTAGE_SIMU5G_BINDER_ACCESS) {
        // Seed the stack attachment ledger from the Binder and deliver it (UE only; see
        // the servingNodeId_ member note). This is still too early to see the result
        // of dynamic cell association.
        if (registration_->getNodeType() == UE) {
            if (registration_->getLteNodeId() != NODEID_NONE)
                servingNodeId_ = binderModule->getServingNode(registration_->getLteNodeId());
            if (registration_->getNrNodeId() != NODEID_NONE)
                nrServingNodeId_ = binderModule->getServingNode(registration_->getNrNodeId());
            handoverPacketHolderModule_ = inet::getModuleFromPar<HandoverPacketHolderUe>(par("handoverPacketHolderModule"), this);
            pushServingNodeIds();
        }
    }
}

void BearerManagement::setServingNodeId(MacNodeId servingNodeId)
{
    Enter_Method_Silent("setServingNodeId");
    ASSERT(registration_->getNodeType() == UE);
    servingNodeId_ = servingNodeId;
    pushServingNodeIds();
}

void BearerManagement::setNrServingNodeId(MacNodeId servingNodeId)
{
    Enter_Method_Silent("setNrServingNodeId");
    ASSERT(registration_->getNodeType() == UE);
    nrServingNodeId_ = servingNodeId;
    pushServingNodeIds();
}

void BearerManagement::pushServingNodeIds()
{
    ASSERT(registration_->getNodeType() == UE);
    if (servingNodeId_ != NODEID_NONE && nrServingNodeId_ != NODEID_NONE && !dualConnectivityEnabled_)
        throw cRuntimeError("This UE is attached with both its LTE and its NR stack, but dual "
                "connectivity is off -- dual attachment outside dual connectivity is not supported");
    if (ip2nicModule_ != nullptr)
        ip2nicModule_->setServingNodeIds(servingNodeId_, nrServingNodeId_);
    handoverPacketHolderModule_->setServingNodeIds(servingNodeId_, nrServingNodeId_);
    for (auto& [id, module] : pdcpEntities_)
        if (cModule *splitter = module->getSubmodule("splitter"))
            check_and_cast<DcPdcpLegSplitter *>(splitter)->setServingNodeIds(servingNodeId_, nrServingNodeId_);
}

// Whether this base station serves only a non-anchor leg of the given UE's bearers:
// it is a secondary node, and the UE is also attached to its master, where the
// bearers' PDCP -- and any SDAP -- terminates (MN-terminated, TS 37.340). This
// node's contribution to such a bearer is RLC/MAC legs (or being the X2 relay
// target), independent of the bearer's kind.
bool BearerManagement::servesNonAnchorLegOnly(MacNodeId ueId)
{
    ASSERT(registration_->getNodeType() == NODEB);
    MacNodeId myId = getOwnNodeId();
    MacNodeId myMaster = binderModule->getMasterNodeOrSelf(myId);
    if (myMaster == myId)
        return false;   // a master or standalone node anchors the bearers of the UEs it serves
    cModule *ueModule = binderModule->getNodeModule(ueId);
    for (const auto& [nodeId, info] : binderModule->getNodeInfoMap())
        if (info.moduleRef == ueModule && binderModule->getServingNode(nodeId) == myMaster)
            return true;
    return false;
}

// Take delivery of one bearer's configuration from the serving base station's control
// plane (see ConnectionControlEnb::pushDrbToRrcs()). RRC records it, to establish the bearer
// from later, and
// pushes on what the local layers consume: SDAP's QFI-to-DRB view and the eNB MAC's
// per-bearer QoS profile are working copies those modules never author themselves.
void BearerManagement::configureDrb(const DrbDesc& drb)
{
    Enter_Method("configureDrb(drb %d)", (int)num(drb.getDrbId()));
    EV << "BearerManagement::configureDrb - " << drb << endl;

    // The bearer's stated architecture must match this stack: 5gc bearers need SDAP
    // to map their QoS flows, eps bearers a stack that classifies without it. The
    // check applies where SDAP responsibilities live -- at the UE and at the node
    // anchoring the bearer's PDCP. A secondary node serving a non-anchor leg does no
    // SDAP work whatever the bearer's kind (the leg is RLC/MAC only; PDCP terminates
    // at its master), so it accepts either -- while still recording the descriptor
    // and pushing the QoS profile into its MAC below.
    bool hasSdap = sdapModule.getNullable() != nullptr;
    bool anchorEnd = registration_->getNodeType() == UE || !servesNonAnchorLegOnly(drb.key.getNodeId());
    if (anchorEnd && drb.coreNetwork == CN_5GC && !hasSdap)
        throw cRuntimeError("configureDrb: DRB %d is a \"5gc\" bearer, but this stack has no SDAP to map its QoS flows",
                (int)num(drb.getDrbId()));
    if (anchorEnd && drb.coreNetwork == CN_EPC && hasSdap)
        throw cRuntimeError("configureDrb: DRB %d is an \"epc\" bearer, but this stack has SDAP -- its bearers are selected by QFI, not by packet filters",
                (int)num(drb.getDrbId()));

    drbTableModule->addConfiguredDrb(drb);

    if (sdapModule.getNullable() != nullptr)
        sdapModule->configureDrb(drb);
    if (drb.hasQosProfile && registration_->getNodeType() == NODEB)
        check_and_cast<LteMacEnb *>(macModule.get())->configureDrbQos(drb.key, drb.qos);
}

std::vector<DrbDesc> BearerManagement::getConfiguredDrbs(MacNodeId peerId)
{
    std::vector<DrbDesc> drbs;
    for (const auto& [key, drb] : drbTableModule->getConfiguredDrbs())
        if (key.getNodeId() == peerId)
            drbs.push_back(drb);
    return drbs;
}

void BearerManagement::setUplinkQfiRules(QfiRuleSet&& rules)
{
    Enter_Method_Silent("setUplinkQfiRules");
    if (qosFlowClassifierModule.getNullable() == nullptr)
        throw cRuntimeError("setUplinkQfiRules: this stack has no QoS-flow classifier to deliver uplink QFI rules to");
    qosFlowClassifierModule->setQfiRules(std::move(rules));
}

BearerManagement::~BearerManagement()
{
    cancelAndDelete(rlfTrigger_);
    for (auto& [msg, peerId] : t311Timers_)
        cancelAndDelete(msg);
    for (auto& [msg, peerId] : t301Timers_)
        cancelAndDelete(msg);
}

void BearerManagement::handleMessage(cMessage *msg)
{
    if (msg == rlfTrigger_) {
        // Drain deferred radio-link-failure teardowns at a safe point (a fresh event),
        // never from inside RLC/PDCP processing.
        auto pending = pendingRlf_;   // copy: teardown deletes entity modules
        pendingRlf_.clear();
        for (const auto &[nodeId, nrStack] : pending)
            handleRadioLinkFailure(nodeId, nrStack);
        return;
    }
    // RRC re-establishment (TS 38.331 5.3.7), two phases; see releaseLink().
    auto t1 = t311Timers_.find(msg);
    if (t1 != t311Timers_.end()) {
        // T311 expiry: a suitable cell is assumed selected (single-cell). Send the
        // RRCReestablishmentRequest and start T301 (request -> complete).
        MacNodeId peerId = t1->second;
        t311Timers_.erase(t1);
        EV << NOW << " BearerManagement - RRC re-establishment: T311 done for node " << peerId
           << ", suitable cell selected; RRCReestablishmentRequest sent, T301 started ("
           << t301_ << "s)" << endl;
        cMessage *t301 = new cMessage("rrcT301");
        t301Timers_[t301] = peerId;
        scheduleAfter(t301_, t301);
        delete msg;
        return;
    }
    auto t3 = t301Timers_.find(msg);
    if (t3 != t301Timers_.end()) {
        // T301 expiry: RRCReestablishmentComplete -- un-release the peer so its bearer
        // re-establishes on demand and DL/UL traffic resumes.
        MacNodeId peerId = t3->second;
        t301Timers_.erase(t3);
        if (ip2nicModule_ != nullptr)
            ip2nicModule_->resumeUe(peerId);
        EV << NOW << " BearerManagement - RRC re-establishment complete for node " << peerId
           << "; bearer re-establishes on demand, traffic resumes" << endl;
        delete msg;
        return;
    }
    throw cRuntimeError("This module does not process messages");
}

void BearerManagement::releaseDrbIdOf(DrbKey bearer)
{
    connectionControlModule->bearerReleased(bearer);
}

void BearerManagement::notifyBearerEstablished(DrbKey key)
{
    // Ip2Nic learns of the bearer through the flow binding installed in
    // createOutgoingConnection(), which is where the flow it carries is known.
    if (sdapModule.getNullable() != nullptr)
        sdapModule->bearerEstablished(key);
}

void BearerManagement::notifyBearerReleased(DrbKey key)
{
    if (ip2nicModule_ != nullptr)
        ip2nicModule_->releaseFlowBindings(key);
    if (sdapModule.getNullable() != nullptr)
        sdapModule->bearerReleased(key);
}

void BearerManagement::scheduleRadioLinkFailure(MacNodeId nodeId, bool nrStack)
{
    Enter_Method_Silent("scheduleRadioLinkFailure()");
    EV << NOW << " BearerManagement::scheduleRadioLinkFailure - node " << nodeId
       << (nrStack ? " (NR)" : " (LTE)") << endl;
    pendingRlf_.insert({nodeId, nrStack});
    if (!rlfTrigger_)
        rlfTrigger_ = new cMessage("rlfTrigger");
    if (!rlfTrigger_->isScheduled())
        scheduleAt(simTime(), rlfTrigger_);
}

void BearerManagement::handleRadioLinkFailure(MacNodeId nodeId, bool nrStack)
{
    EV << NOW << " BearerManagement::handleRadioLinkFailure - RLF for node " << nodeId
       << (nrStack ? " (NR)" : " (LTE)") << endl;
    // Release + tear down the link on BOTH ends, so if RRC re-establishment is enabled the
    // bearer rebuilds fresh, SN-consistent entities on both sides: this end here, the
    // peer's through the node's control-plane entry point, which tells the peer's.
    releaseLink(nodeId);
    // The peer keys its entities (PDCP/RLC/MAC) for this link by OUR node id on the
    // failing leg (the source/dest id it saw on the bearer): the nrNodeId of a
    // standalone NR gNB, whose lteNodeId is NODEID_NONE.
    MacNodeId myId = nrStack ? registration_->getNrNodeId() : registration_->getLteNodeId();
    connectionControlModule->radioLinkFailure(myId, nodeId);
}

void BearerManagement::releaseLink(MacNodeId peerId)
{
    Enter_Method_Silent("releaseLink()");
    EV << NOW << " BearerManagement::releaseLink - releasing link to " << peerId << endl;
    // UE Context Release: stop traffic at Ip2Nic first, so no packet is pushed at a torn-down
    // bearer (which would otherwise hit PdcpMux's TX-entity assert).
    if (ip2nicModule_ != nullptr) {
        ip2nicModule_->releaseUe(peerId);
        // RRC re-establishment (TS 38.331 5.3.7): start the cell-(re)selection timer T311. When it
        // fires a suitable cell is assumed selected and the RRCReestablishmentRequest->Complete
        // (T301) exchange runs, after which the peer is un-released and its bearer re-establishes
        // on demand. t311_ = 0 disables re-establishment (the peer stays released -> RRC_IDLE).
        if (t311_ > SIMTIME_ZERO) {
            cMessage *t311 = new cMessage("rrcT311");
            t311Timers_[t311] = peerId;
            scheduleAfter(t311_, t311);
            EV << NOW << " BearerManagement::releaseLink - RRC re-establishment: T311 (cell selection) started for node "
               << peerId << " (" << t311_ << "s)" << endl;
        }
    }
    // Tear down BOTH legs' MAC/HARQ + RLC (leg-agnostic: a bearer may live on the LTE-FI or
    // NR-SO leg, and on a gNB every entity is on the base leg regardless of the UE being NR),
    // then the shared PDCP entities. On a UE these delete all local entities (not nodeId-
    // filtered on the UE side), which is what RLF -- a link-level event -- requires.
    if (macModule)
        macModule->deleteQueuesRadioLinkFailure(peerId);
    if (nrMacModule)
        nrMacModule->deleteQueuesRadioLinkFailure(peerId);
    deleteLocalRlcQueues(peerId, /*nrStack=*/false);
    deleteLocalRlcQueues(peerId, /*nrStack=*/true);
    deleteLocalPdcpEntities(peerId);
}

const DrbDesc *BearerManagement::lookupConfiguredDrb(const FlowId& flow, MacNodeId peerId)
{
    // Authored entries describe infrastructure bearers only; multicast and D2D bearers
    // have DRB ids from other key spaces and must not match them.
    if (flow.d2dGroupId != NODEID_NONE || flow.d2dTxPeerId != NODEID_NONE || flow.d2dRxPeerId != NODEID_NONE)
        return nullptr;
    // Authored entries are keyed by NODEID_NONE ("my serving node") on the UE side,
    // and by the UE's node id on the gNB side.
    MacNodeId cfgPeer = (registration_->getNodeType() == UE) ? NODEID_NONE : peerId;
    return drbTableModule->findConfiguredDrb(DrbKey(cfgPeer, flow.drbId));
}

BearerRequest BearerManagement::resolveBearerRequest(const BearerRequest& reqIn, const FlowId& flow, MacNodeId peerId)
{
    BearerRequest req = reqIn;

    // The authored configuration is RRC's own decision for this bearer and takes
    // precedence; a conflicting explicit request is a configuration error, not a tie
    // to break silently.
    const DrbDesc *cfg = lookupConfiguredDrb(flow, peerId);
    if (cfg && cfg->rlcMode != UNKNOWN_RLC_MODE) {
        if (req.rlcMode != UNKNOWN_RLC_MODE && req.rlcMode != cfg->rlcMode)
            throw cRuntimeError("Bearer establishment request for DRB %d (peer %d) asks for RLC mode %s, "
                    "but the binder's staticDrbs configures it as %s -- conflicting configuration",
                    (int)num(flow.drbId), (int)num(peerId),
                    rlcModeToA(req.rlcMode).c_str(), rlcModeToA(cfg->rlcMode).c_str());
        req.rlcMode = cfg->rlcMode;
    }

    if (req.rlcMode == UNKNOWN_RLC_MODE)
        throw cRuntimeError("Bearer establishment request for DRB %d (peer %d) carries no RLC mode -- "
                "the bearer configurator resolves every request from the bearer's definition entry, "
                "so an unresolved request reaching RRC is a requester bug",
                (int)num(flow.drbId), (int)num(peerId));
    return req;
}

void BearerManagement::createIncomingConnection(const FlowId& flow, const BearerRequest& reqIn, bool withPdcp)
{
    Enter_Method_Silent("createIncomingConnection()");

    // Resolve rlcMode == UNKNOWN_RLC_MODE ("RRC decides") before any use: entity type
    // selection (findOrCreateRlcEntity) and materializeDrb must see only resolved values.
    const BearerRequest req = resolveBearerRequest(reqIn, flow, flow.sourceId);

    EV << "BearerManagement::createIncomingConnection - " << " srcId=" << flow.sourceId << " destId=" << flow.destId
        << " groupId=" << flow.d2dGroupId << " drbId=" << flow.drbId
        << " direction=" << dirToA(flow.direction)
        << " withPdcp=" << (withPdcp ? "yes" : "no") << endl;

    ASSERT(isLocalNodeId(flow.destId) || flow.d2dGroupId != NODEID_NONE);

    // A DC master anchoring an SCG bearer: PDCP only, wired to the X2 path (see
    // createOutgoingConnection())
    if (isPdcpAnchorOnly(flow, flow.sourceId)) {
        ASSERT(withPdcp);
        installPdcpRxSide(DrbKey(flow.sourceId, flow.drbId), flow, req.rlcMode, rlcMuxModule.get(), false);
        return;
    }

    // Idempotence guard: with duplex bearer establishment this half may already
    // exist (e.g. re-establishment after a partial teardown); skip instead of
    // crashing on duplicate MAC/RLC/PDCP creation.
    DrbKey rlcId = flow.rxDrbKey();

    // Which of this node's legs does the bearer belong to? For a unicast bearer the
    // destination *is* this node, so its id answers that. A multicast bearer has no single
    // destination -- destId then names some other node -- but it is transmitted on the
    // sender's leg and delivered only to peers on the matching leg (D2dUePhy::sendMulticast
    // skips receivers whose isNrUe() disagrees with the transmitting PHY's isNr_), so there
    // the sender's id selects the leg. This mirrors createOutgoingConnection(), which uses
    // sourceId because there the sender is this node.
    MacNodeId legNodeId = (flow.d2dGroupId != NODEID_NONE) ? flow.sourceId : flow.destId;
    bool isNr = (registration_->getNodeType() == UE && isNrUe(legNodeId));
    cModule *existingRlcEnt = lookupRlcEntityModule(rlcId, isNr);
    if (existingRlcEnt != nullptr && existingRlcEnt->gate("lowerIn")->isConnectedOutside()) {
        EV << "BearerManagement::createIncomingConnection - entities for " << rlcId.str() << " already exist, skipping\n";
        return;
    }

    // RLC entity creation, then the bearer descriptor (needs the RLC entity in place for
    // snFieldLength), then MAC's logical-channel configuration -- all before the MAC
    // connection is created below, since LteMacBase::createOutgoingConnection's lcgMap_
    // insertion reads the pushed configuration back via getLogicalChannelConfig().
    auto *rlcMux = isNr ? nrRlcMuxModule.get() : rlcMuxModule.get();
    installRlcRxSide(rlcId, flow, req, rlcMux, isNr);

    const DrbDesc& drb = materializeDrb(flow, req, flow.sourceId, rlcId, isNr);

    MacNodeId senderId = flow.sourceId;
    auto mac = isNr ? nrMacModule.get() : macModule.get();
    LogicalCid lcid = mac->drbIdToLcid(flow.drbId);
    MacCid cid = MacCid(senderId, lcid);
    mac->configureLogicalChannel(cid, LogicalChannelConfig{drb.rlcMode, drb.soFraming, drb.snFieldLength, drb.lcg});

    // Create MAC incoming connection
    FlowDescriptor desc = makeFlowDescriptor(flow);
    mac->createIncomingConnection(cid, desc);

    // PDCP entity creation (compound: TX+RX, see PdcpEntityBase). At a DC secondary the bearer's
    // PDCP lives at the master, so an PdcpRelayEntity stands in (UL half wired here).
    if (withPdcp) {
        DrbKey id = DrbKey(flow.sourceId, flow.drbId);
        installPdcpRxSide(id, flow, drb.rlcMode, rlcMux, isNr);
    }
    else {
        // DC secondary node: forward the UL PDU from RLC to the master over X2 unprocessed
        // (PdcpRelayEntity's UL half, see PdcpRelayEntity).
        auto *pdcpDcMux = inet::findModuleFromPar<DcMux>(par("pdcpDcMuxModule"), this);
        ASSERT(pdcpDcMux != nullptr); // relay entities are eNB-only
        DrbKey id = DrbKey(flow.sourceId, flow.drbId);
        cModule *relay = findOrCreatePdcpRelayEntity(id, rlcMux);

        // Wire RLC entity upperOut → relay legIn[0] (direct per-DRB connection)
        cModule *rlcEnt2 = lookupRlcEntityModule(rlcId, isNr);
        ASSERT(rlcEnt2 != nullptr);
        rlcEnt2->gate("upperOut")->connectTo(relay->gate("legIn", 0));

        // Wire relay x2Out → DcMux (UL PDU tunneled to the master)
        int fromIdx = pdcpDcMux->gateSize("fromEntity");
        pdcpDcMux->setGateSize("fromEntity", fromIdx + 1);
        relay->gate("x2Out")->connectTo(pdcpDcMux->gate("fromEntity", fromIdx));
    }
}

void BearerManagement::createOutgoingConnection(const FlowId& flow, const BearerRequest& reqIn, bool withPdcp)
{
    Enter_Method_Silent("createOutgoingConnection()");

    // Resolve rlcMode == UNKNOWN_RLC_MODE ("RRC decides") before any use: entity type
    // selection (findOrCreateRlcEntity) and materializeDrb must see only resolved values.
    const BearerRequest req = resolveBearerRequest(reqIn, flow, flow.destId);

    EV << "BearerManagement::createOutgoingConnection - " << " srcId=" << flow.sourceId << " destId=" << flow.destId
        << " groupId=" << flow.d2dGroupId << " drbId=" << flow.drbId
        << " direction=" << dirToA(flow.direction)
        << " withPdcp=" << (withPdcp ? "yes" : "no") << endl;

    ASSERT(isLocalNodeId(flow.sourceId));

    // Bind the requester's flow to this bearer in the local classifier. Both endpoints
    // are configured this way, each by its own RRC and each with the flow key as it
    // sees it, so reverse traffic resolves to this bearer instead of establishing a
    // parallel one. Ip2Nic never authors these bindings itself.
    if (req.flowBindingKey.has_value()) {
        if (ip2nicModule_ != nullptr)
            ip2nicModule_->configureFlowBinding(*req.flowBindingKey, DrbKey(flow.destId, flow.drbId));
    }

    // A DC master anchoring an SCG bearer terminates the bearer's PDCP -- the core
    // network delivers the UE's traffic here -- but no cell group of its own carries
    // it: the PDCP legs are wired to the X2 path, and no local RLC/MAC state exists.
    if (isPdcpAnchorOnly(flow, flow.destId)) {
        ASSERT(withPdcp);
        installPdcpTxSide(DrbKey(flow.destId, flow.drbId), flow, req.rlcMode, rlcMuxModule.get(), false);
        return;
    }

    // Idempotence guard: with duplex bearer establishment this half may already
    // exist (e.g. re-establishment after a partial teardown); skip instead of
    // crashing on duplicate MAC/RLC/PDCP creation.
    DrbKey rlcId = flow.txDrbKey();
    bool isNr = (registration_->getNodeType()==UE && isNrUe(flow.sourceId));
    cModule *existingRlcEnt = lookupRlcEntityModule(rlcId, isNr);
    if (existingRlcEnt != nullptr && existingRlcEnt->gate("lowerOut")->isConnectedOutside()) {
        EV << "BearerManagement::createOutgoingConnection - entities for " << rlcId.str() << " already exist, skipping\n";
        return;
    }

    // RLC entity creation, then the bearer descriptor (needs the RLC entity in place for
    // snFieldLength), then MAC's logical-channel configuration -- all before the MAC
    // connection is created below, since LteMacBase::createOutgoingConnection's lcgMap_
    // insertion reads the pushed configuration back via getLogicalChannelConfig().
    auto *rlcMux = isNr ? nrRlcMuxModule.get() : rlcMuxModule.get();
    installRlcTxSide(rlcId, flow, req, rlcMux, isNr);

    const DrbDesc& drb = materializeDrb(flow, req, flow.destId, rlcId, isNr);

    MacNodeId destId = flow.destId;
    auto mac = (registration_->getNodeType()==UE && isNrUe(flow.sourceId)) ? nrMacModule.get() : macModule.get();
    LogicalCid lcid = mac->drbIdToLcid(flow.drbId);
    MacCid cid = MacCid(destId, lcid);
    mac->configureLogicalChannel(cid, LogicalChannelConfig{drb.rlcMode, drb.soFraming, drb.snFieldLength, drb.lcg});

    // Create MAC outgoing connection
    FlowDescriptor desc = makeFlowDescriptor(flow);
    mac->createOutgoingConnection(cid, desc);

    // PDCP entity creation (compound: TX+RX, see PdcpEntityBase). At a DC secondary the bearer's
    // PDCP lives at the master, so an PdcpRelayEntity stands in (DL half wired here).
    if (withPdcp) {
        DrbKey id = DrbKey(flow.destId, flow.drbId);
        installPdcpTxSide(id, flow, drb.rlcMode, rlcMux, isNr);
    }
    else {
        // DC secondary node: the DL PDU arrives already PDCP-processed from the master over X2;
        // forward it straight to RLC (PdcpRelayEntity's DL half, see PdcpRelayEntity).
        auto *pdcpDcMux = inet::findModuleFromPar<DcMux>(par("pdcpDcMuxModule"), this);
        ASSERT(pdcpDcMux != nullptr); // relay entities are eNB-only
        DrbKey id = DrbKey(flow.destId, flow.drbId);
        cModule *relay = findOrCreatePdcpRelayEntity(id, rlcMux);

        // Wire DcMux → relay x2In (DcMux dispatches the incoming DL X2 PDU)
        int idx = pdcpDcMux->gateSize("toRelayEntity");
        pdcpDcMux->setGateSize("toRelayEntity", idx + 1);
        pdcpDcMux->gate("toRelayEntity", idx)->connectTo(relay->gate("x2In"));

        // Wire relay legOut[0] → RLC entity upperIn (direct per-DRB connection)
        cModule *rlcEnt2 = lookupRlcEntityModule(rlcId, isNr);
        ASSERT(rlcEnt2 != nullptr);
        relay->gate("legOut", 0)->connectTo(rlcEnt2->gate("upperIn"));
    }
}

void BearerManagement::setRlcEntityParams(cModule *entity, bool isNr)
{
    // 'entity' is the RlcTm/Um/AmEntity COMPOUND; these paths are relative to it (^ = the NIC).
    // The compound NED absPath()-resolves them and passes them to its tx/rx submodules (see
    // '*.macModule = default(absPath(this.macModule))'). hasPar() guards because not every
    // compound/leg declares both (TM has neither; AM has no rlcMux). Per leg: a UE's NR leg uses
    // nrMac/nrRlcMux; everything else (incl. a gNB's NR bearers, which have no nrMac) uses mac/rlcMux.
    if (entity->hasPar("macModule"))
        entity->par("macModule").setStringValue(isNr ? "^.nrMac" : "^.mac");
    if (entity->hasPar("rlcMuxModule"))
        entity->par("rlcMuxModule").setStringValue(isNr ? "^.nrRlcMux" : "^.rlcMux");
    // The RLC entity's wire format is not a parameter: it is inherent to the entity
    // class selected per bearer in findOrCreateRlcEntity() (NR-SO vs LTE-FI, by the same
    // RAT predicate as here), and RRC records the choice in the bearer's descriptor
    // (DrbDesc::soFraming, see materializeDrb()). (The NR-leg marker proper is the
    // ip2nic.isNr parameter, not set here.)
}

void BearerManagement::setEntityDisplayPosition(cModule *entity, bool isPdcpEntity, cModule *rlcMux, int bearerIndex)
{
    auto *pdcpMux = inet::getModuleFromPar<PdcpMux>(par("pdcpMuxModule"), this);
    if (!pdcpMux || !rlcMux)
        return;

    int uy = atoi(pdcpMux->getDisplayString().getTagArg("p", 1));
    int lx = atoi(rlcMux->getDisplayString().getTagArg("p", 0));
    int ly = atoi(rlcMux->getDisplayString().getTagArg("p", 1));

    int x = lx + 60 * bearerIndex;
    int y = isPdcpEntity ? uy + (ly - uy) / 3 : uy + 2 * (ly - uy) / 3;

    entity->getDisplayString().setTagArg("p", 0, x);
    entity->getDisplayString().setTagArg("p", 1, y);
}

cModule *BearerManagement::lookupRlcEntityModule(DrbKey id, bool isNr)
{
    auto& entities = isNr ? nrRlcEntities_ : rlcEntities_;
    auto it = entities.find(id);
    return it != entities.end() ? it->second : nullptr;
}

cModule *BearerManagement::findOrCreateRlcEntity(DrbKey id, RlcMode rlcMode, const FlowId& flow, RlcMux *rlcMux, bool isNr)
{
    auto& entities = isNr ? nrRlcEntities_ : rlcEntities_;
    auto it = entities.find(id);
    if (it != entities.end())
        return it->second;

    // Create the per-bearer RLC entity module (compound: TX + RX sides). With duplex
    // bearer establishment the first-processed direction creates it; the other
    // direction finds it here and just installs (wires) its own side.
    // The entity TYPE (LTE-FI vs NR-SO wire format) must be identical at both ends of
    // the bearer, so it keys on whether the bearer's UE is an NR UE — a symmetric
    // property of the source/dest node ids — not on the local-node isNr flag (which is
    // only ever true at a UE, leaving the gNB on the LTE type and breaking the wire).
    // For a DC UE this still separates the NR-secondary leg from the LTE-master leg,
    // since those bearers reference the UE's NR vs LTE node id respectively.
    bool isNrBearer = isNrUe(flow.sourceId) || isNrUe(flow.destId);
    cModuleType *moduleType;
    const char *prefix;
    switch (rlcMode) {
        case TM: moduleType = rlcTmEntityModuleType_; prefix = "tm"; break;
        case AM: moduleType = isNrBearer ? nrRlcAmEntityModuleType_ : lteRlcAmEntityModuleType_; prefix = "am"; break;
        default: moduleType = isNrBearer ? nrRlcUmEntityModuleType_ : lteRlcUmEntityModuleType_; prefix = "um"; break;
    }
    std::string name = std::string(isNr ? "nrRlc-" : "rlc-") + prefix + "-" + std::to_string(num(id.getNodeId())) + "-" + std::to_string(num(id.getDrbId()));
    auto *module = moduleType->create(name.c_str(), nicModule_);
    // Set the leg's MAC/RLC-mux paths on the COMPOUND before finalize; its NED passes them down
    // to the tx/rx submodules (see '*.macModule = this.macModule' in RlcUmEntityBase/RlcAmEntityBase), so
    // the submodule params carry the right value at build time -- no post-build @mutable write.
    setRlcEntityParams(module, isNr);
    module->finalizeParameters();
    module->buildInside();
    setEntityDisplayPosition(module, false, rlcMux, num(id.getDrbId()));

    module->scheduleStart(simTime());
    module->callInitialize();

    entities[id] = module;
    return module;
}

RlcTxEntityBase *BearerManagement::installRlcTxSide(DrbKey id, const FlowId& flow, const BearerRequest& req, RlcMux *rlcMux, bool isNr)
{
    cModule *module = findOrCreateRlcEntity(id, req.rlcMode, flow, rlcMux, isNr);
    auto *txEnt = check_and_cast<RlcTxEntityBase *>(module->getSubmodule("tx"));

    if (module->gate("lowerOut")->isConnectedOutside()) {
        EV << "BearerManagement::installRlcTxSide - TX side of " << id.str() << " already installed\n";
        return txEnt;
    }

    // Wire entity lowerOut gate → RlcMux fromTxEntity
    int fromIdx = rlcMux->gateSize("fromTxEntity");
    rlcMux->setGateSize("fromTxEntity", fromIdx + 1);
    module->gate("lowerOut")->connectTo(rlcMux->gate("fromTxEntity", fromIdx));

    // Wire RlcMux macToTxEntity → entity macIn gate, and tell the mux which gate serves
    // this DRB, exactly as installRlcRxSide() registers the RX side
    int macIdx = rlcMux->gateSize("macToTxEntity");
    rlcMux->setGateSize("macToTxEntity", macIdx + 1);
    rlcMux->gate("macToTxEntity", macIdx)->connectTo(module->gate("macIn"));
    rlcMux->registerTxEntity(id, macIdx);

    FlowDescriptor proto = makeFlowDescriptor(flow);
    txEnt->setFlowControlInfo(&proto); // note: a D2D UM TX entity also registers itself with the D2D mode controller here


    return txEnt;
}

RlcRxEntityBase *BearerManagement::installRlcRxSide(DrbKey id, const FlowId& flow, const BearerRequest& req, RlcMux *rlcMux, bool isNr)
{
    cModule *module = findOrCreateRlcEntity(id, req.rlcMode, flow, rlcMux, isNr);
    auto *rxEnt = check_and_cast<RlcRxEntityBase *>(module->getSubmodule("rx"));

    if (module->gate("lowerIn")->isConnectedOutside()) {
        EV << "BearerManagement::installRlcRxSide - RX side of " << id.str() << " already installed\n";
        return rxEnt;
    }

    // Wire RlcMux → entity lowerIn gate
    int idx = rlcMux->gateSize("toRxEntity");
    rlcMux->setGateSize("toRxEntity", idx + 1);
    rlcMux->gate("toRxEntity", idx)->connectTo(module->gate("lowerIn"));

    FlowDescriptor proto = makeFlowDescriptor(flow);
    rxEnt->setFlowControlInfo(&proto);

    // Register in mux routing table
    rlcMux->registerRxEntity(id, idx);

    return rxEnt;
}

// Records the configuration of the bearer being established, keyed by the peer node and
// the DRB id, so that the two directions of the duplex establishment fill in the same
// descriptor. At a DC UE the NR leg is established against the secondary node and so gets
// a descriptor of its own; joining the legs of a split bearer into one descriptor is what
// the legs table will do.
const DrbDesc& BearerManagement::materializeDrb(const FlowId& flow, const BearerRequest& req, MacNodeId peerId, DrbKey rlcId, bool isNr)
{
    DrbDesc& drb = drbTableModule->getOrCreateDrb(DrbKey(peerId, flow.drbId));
    drb.lcid = LogicalCid(num(flow.drbId));   // the 1:1 mapping of LteMacBase::drbIdToLcid
    drb.lcg = req.lcg;
    drb.rlcMode = req.rlcMode;

    // soFraming is RRC's own decision: the RAT+mode predicate that also selects the
    // entity type (NR UM gets the SO/no-concat framing of TS 38.322; everything else --
    // LTE, and NR AM, whose MAC-side SO multiplexing MAC does not implement -- gets the
    // FI/concatenation framing of TS 36.322).
    bool isNrBearer = isNrUe(flow.sourceId) || isNrUe(flow.destId);
    drb.soFraming = (drb.rlcMode == UM) && isNrBearer;

    // The SN field length, in contrast, is transcribed off the RLC entity serving the
    // bearer: its source of truth is the entity's own parameters (NrRlcUmTxEntity's
    // sn_FieldLength, NR-AM's AM_Window_Size derivation), not a bearer-level decision.
    cModule *rlcEnt = lookupRlcEntityModule(rlcId, isNr);
    ASSERT(rlcEnt != nullptr);   // the RLC entity is installed before this is called
    auto *txEnt = check_and_cast<RlcTxEntityBase *>(rlcEnt->getSubmodule("tx"));
    drb.snFieldLength = txEnt->snFieldLength();

    // The SDAP half, the selectors and the QoS profile come from the authored
    // configuration, if any, so the established descriptor is the complete record
    // of the bearer.
    if (const DrbDesc *cfg = lookupConfiguredDrb(flow, peerId)) {
        drb.coreNetwork = cfg->coreNetwork;
        drb.sessionType = cfg->sessionType;
        drb.mappedQfis = cfg->mappedQfis;
        drb.isDefault = cfg->isDefault;
        drb.filters = cfg->filters;
        drb.hasQosProfile = cfg->hasQosProfile;
        drb.qos = cfg->qos;
    }

    EV << "BearerManagement::materializeDrb - " << drb << endl;
    return drb;
}

int BearerManagement::getNumLegs(DrbKey id, const FlowId& flow)
{
    // Number of legs of this bearer. Two-leg (split-capable) bearers: at a DC UE, every
    // infrastructure bearer (local LTE + local NR stack legs); at a DC master, every UE bearer
    // (local leg + remote leg via X2 to the secondary). Everything else -- non-DC nodes, D2D
    // and multicast bearers, secondaries (X2 relay only) -- is single-leg.
    bool isEnb = (registration_->getNodeType() == NODEB);
    int numLegs = 1;
    if (dualConnectivityEnabled_ && flow.d2dGroupId == NODEID_NONE) {
        if (!isEnb && getNodeTypeById(id.getNodeId()) == NODEB)
            numLegs = 2;
        else if (isEnb && binderModule->getSecondaryNode(getOwnNodeId()) != NODEID_NONE)
            numLegs = 2;
    }

    // A bearer whose configuration states its legs has them; the derivation above is the
    // fallback for the bearers no definition covers, and for definitions that leave the
    // legs to RRC. A configuration can therefore keep a bearer off the secondary node in a
    // dual-connectivity network, which the derivation alone cannot express.
    if (const DrbDesc *cfg = lookupConfiguredDrb(flow, id.getNodeId()))
        if (!cfg->legs.empty())
            return cfg->legs.size();

    return numLegs;
}

std::vector<CellGroup> BearerManagement::legCellGroups(DrbKey id, const FlowId& flow, int numLegs)
{
    if (const DrbDesc *cfg = lookupConfiguredDrb(flow, id.getNodeId())) {
        if (!cfg->legs.empty()) {
            ASSERT((int)cfg->legs.size() == numLegs);
            std::vector<CellGroup> groups;
            for (const RlcBearerDesc& leg : cfg->legs)
                groups.push_back(leg.cellGroup);
            return groups;
        }
    }
    return numLegs == 2 ? std::vector<CellGroup>{MCG, SCG} : std::vector<CellGroup>{MCG};
}

bool BearerManagement::isPdcpAnchorOnly(const FlowId& flow, MacNodeId peerId)
{
    if (registration_->getNodeType() != NODEB)
        return false;
    // a secondary node never terminates PDCP -- it installs X2 relays, whatever the legs say
    MacNodeId myId = getOwnNodeId();
    if (myId != NODEID_NONE && binderModule->getMasterNodeOrSelf(myId) != myId)
        return false;
    const DrbDesc *cfg = lookupConfiguredDrb(flow, peerId);
    return cfg != nullptr && !cfg->legs.empty() &&
           std::none_of(cfg->legs.begin(), cfg->legs.end(),
                   [](const RlcBearerDesc& leg) { return leg.cellGroup == MCG; });
}

cModule *BearerManagement::findOrCreatePdcpEntity(DrbKey id, const FlowId& flow, RlcMode rlcMode, RlcMux *rlcMux)
{
    auto it = pdcpEntities_.find(id);
    if (it != pdcpEntities_.end())
        return it->second;

    bool isEnb = (registration_->getNodeType() == NODEB);
    int numLegs = getNumLegs(id, flow);
    std::vector<CellGroup> legGroups = legCellGroups(id, flow, numLegs);

    // Create the per-bearer PDCP entity module (compound: TX + RX sides and, on a multi-leg
    // or SCG-leg bearer, the leg splitter/joiner). With duplex bearer establishment the
    // first-processed direction creates it; later calls find it here and just wire their
    // own leg/side.
    std::string name = "pdcp-" + std::to_string(num(id.getNodeId())) + "-" + std::to_string(num(id.getDrbId()));
    auto *module = pdcpEntityModuleType_->create(name.c_str(), nicModule_);
    // PDCP-Config headerCompression, as the bearer's configuration states it
    std::string rohcProfiles;
    if (const DrbDesc *cfg = lookupConfiguredDrb(flow, id.getNodeId()))
        for (const std::string& profile : cfg->rohcProfiles)
            rohcProfiles += (rohcProfiles.empty() ? "" : " ") + profile;
    module->par("rohcProfiles") = rohcProfiles;
    module->par("numLegs") = numLegs;
    std::string legsStr;
    for (CellGroup group : legGroups)
        legsStr += (legsStr.empty() ? "" : " ") + cellGroupToA(group);
    module->par("legs") = legsStr;
    module->par("rlcMode") = rlcTypeToParamValue(rlcMode);
    module->finalizeParameters();
    module->buildInside();

    // The bearer's split policy is pushed into the leg splitter, which buildInside() has
    // just created, the same way every other layer takes its bearer configuration:
    // through a C++ call. A bearer whose definition states nothing keeps the splitter's
    // defaults (primary path MCG, never split). The leg-selection expression is pushed only
    // when the definition carries one for this splitter's direction -- a UE's TX side is
    // uplink, a base station's is downlink -- so the UE gets ulLegSelection and the base
    // station dlLegSelection.
    if (const DrbDesc *cfg = lookupConfiguredDrb(flow, id.getNodeId())) {
        cModule *splitterMod = module->getSubmodule("splitter");
        auto *splitter = splitterMod ? check_and_cast<DcPdcpLegSplitter *>(splitterMod) : nullptr;
        if (splitter != nullptr)
            splitter->setSplitConfig(cfg->primaryPath, cfg->ulDataSplitThreshold);
        const std::string& legSelection = isEnb ? cfg->dlLegSelection : cfg->ulLegSelection;
        if (!legSelection.empty()) {
            if (splitter == nullptr)
                throw cRuntimeError("DRB %d: its definition carries a leg-selection expression, but the bearer was established with a single leg -- there is nothing to steer between",
                        (int)num(id.getDrbId()));
            splitter->setLegSelection(legSelection.c_str());
        }
    }

    // A UE's splitter steers by the stacks' attachment, which RRC pushes: here at
    // creation, and on every handover event (see pushServingNodeIds())
    if (!isEnb)
        if (cModule *splitter = module->getSubmodule("splitter"))
            check_and_cast<DcPdcpLegSplitter *>(splitter)->setServingNodeIds(servingNodeId_, nrServingNodeId_);
    setEntityDisplayPosition(module, true, rlcMux, num(id.getDrbId()));
    module->scheduleStart(simTime());
    module->callInitialize();

    // DC master: wire each remote (SCG/X2) leg to the DcMux right away -- unlike the UE's
    // secondary-stack leg, it has no establishment call of its own (the secondary side is
    // an X2 relay only). Leg 1 of a split bearer; the only leg of an SCG bearer.
    if (isEnb) {
        for (int k = 0; k < numLegs; k++) {
            if (legGroups[k] != SCG)
                continue;
            auto *pdcpDcMux = inet::getModuleFromPar<DcMux>(par("pdcpDcMuxModule"), this);
            int dcIdx = pdcpDcMux->gateSize("fromEntity");
            pdcpDcMux->setGateSize("fromEntity", dcIdx + 1);
            module->gate("legOut", k)->connectTo(pdcpDcMux->gate("fromEntity", dcIdx));
            int rxIdx = pdcpDcMux->gateSize("toRxEntity");
            pdcpDcMux->setGateSize("toRxEntity", rxIdx + 1);
            pdcpDcMux->gate("toRxEntity", rxIdx)->connectTo(module->gate("legIn", k));
        }
    }

    pdcpEntities_[id] = module;
    return module;
}

// At a DC secondary, the stand-in for a bearer's (master-resident) PDCP entity: one PdcpRelayEntity
// compound per bearer, holding both the DL and the UL relay. The first-processed direction creates
// it; the other finds it here and wires its own half (see createOutgoing/IncomingConnection).
cModule *BearerManagement::findOrCreatePdcpRelayEntity(DrbKey id, RlcMux *rlcMux)
{
    auto it = pdcpRelayEntities_.find(id);
    if (it != pdcpRelayEntities_.end())
        return it->second;

    std::string name = "pdcp-relay-" + std::to_string(num(id.getNodeId())) + "-" + std::to_string(num(id.getDrbId()));
    auto *module = pdcpRelayEntityModuleType_->create(name.c_str(), nicModule_);
    module->finalizeParameters();
    module->buildInside();
    setEntityDisplayPosition(module, true, rlcMux, num(id.getDrbId()));
    module->scheduleStart(simTime());
    module->callInitialize();

    pdcpRelayEntities_[id] = module;
    return module;
}

// Leg selection for the install functions: at a DC UE, the NR-leg establishment (NR peer ids)
// attaches leg 1 of the bearer's compound, which is keyed by the MASTER node -- the bearer is
// ONE entity whose legs split/rejoin below PDCP. The master-keyed lookup must be precise
// (master node + same DRB id): matching the bare DRB id would also match unrelated bearers of
// this UE, since DRB ids are only unique per peer. Everything else is leg 0 of its own compound.
MacNodeId BearerManagement::getOwnNodeId() const
{
    return registration_->getLteNodeId() != NODEID_NONE ?
            registration_->getLteNodeId() : registration_->getNrNodeId();
}

bool BearerManagement::isLocalNodeId(MacNodeId nodeId) const
{
    return nodeId == registration_->getLteNodeId() || nodeId == registration_->getNrNodeId();
}

int BearerManagement::selectPdcpLeg(MacNodeId peerId, const FlowId& flow, DrbKey& compoundId /*inout*/)
{
    // Which cell group this establishment call serves. At a UE the answer is in the peer:
    // a call that reaches a secondary node serves the secondary cell group, whichever of
    // the UE's stacks carries it. At a base station it is the node itself, since a
    // secondary node serves the secondary cell group and nothing else.
    bool isUe = (registration_->getNodeType() == UE);
    bool isSecondaryLeg = false;
    MacNodeId masterNodeId = NODEID_NONE;
    if (isUe) {
        if (dualConnectivityEnabled_ && getNodeTypeById(peerId) == NODEB) {
            masterNodeId = binderModule->getMasterNodeOrSelf(peerId);
            isSecondaryLeg = (masterNodeId != peerId);
        }
    }
    else {
        MacNodeId myId = getOwnNodeId();
        isSecondaryLeg = (myId != NODEID_NONE && binderModule->getMasterNodeOrSelf(myId) != myId);
    }

    const DrbDesc *cfg = lookupConfiguredDrb(flow, compoundId.getNodeId());
    bool authoredLegs = (cfg != nullptr && !cfg->legs.empty());

    // A dual-connectivity UE keys every bearer by its master node, whichever cell groups
    // serve it: the UE has one PDCP entity per DRB, and the ids its packets carry are the
    // technology-neutral ones Ip2Nic assigns before the bearer is even known. Which node
    // the network side terminates PDCP at is a separate question, answered there.
    //
    // Only the UE side has a master node to rekey to. A secondary node reaches this point
    // with none, because a bearer's PDCP does not live there -- it installs a relay
    // instead (see findOrCreatePdcpRelayEntity()) -- so there is nothing to rekey either.
    if (isSecondaryLeg && masterNodeId != NODEID_NONE)
        compoundId = DrbKey(masterNodeId, compoundId.getDrbId());

    // The leg's index is where its cell group sits in the bearer's legs, so the single leg
    // of an SCG bearer is index 0 just as the single leg of an MCG bearer is.
    if (authoredLegs) {
        CellGroup group = isSecondaryLeg ? SCG : MCG;
        // A master anchoring an SCG bearer has no MCG leg for its local establishment
        // call to serve; what it installs is the bearer's X2 leg (see isPdcpAnchorOnly())
        if (!isUe && !isSecondaryLeg &&
                std::none_of(cfg->legs.begin(), cfg->legs.end(),
                        [](const RlcBearerDesc& leg) { return leg.cellGroup == MCG; }))
            group = SCG;
        for (size_t k = 0; k < cfg->legs.size(); k++)
            if (cfg->legs[k].cellGroup == group)
                return (int)k;
        throw cRuntimeError("DRB %d is being established on the %s, which its definition does not list among its legs",
                (int)num(compoundId.getDrbId()), cellGroupToA(group).c_str());
    }
    return isSecondaryLeg ? 1 : 0;
}

void BearerManagement::installPdcpTxSide(DrbKey id, const FlowId& flow, RlcMode rlcMode, RlcMux *rlcMux, bool isNr)
{
    auto *pdcpMux = inet::getModuleFromPar<PdcpMux>(par("pdcpMuxModule"), this);
    // The PDCP entity is keyed by dest (id); the RLC entity it wires to is keyed by
    // flow.txDrbKey(), which for multicast is the group id -- not the same as id.
    DrbKey rlcId = flow.txDrbKey();

    DrbKey compoundId = id;
    int legIdx = selectPdcpLeg(flow.destId, flow, compoundId);

    cModule *pdcpEnt = findOrCreatePdcpEntity(compoundId, flow, rlcMode, rlcMux);

    // A master's X2 leg was wired to the DcMux at compound creation (see
    // findOrCreatePdcpEntity()); only a local leg is wired to its RLC entity here.
    // Idempotence goes by the leg's wiring for a local leg, by the TX registration
    // for an X2 one, whose gate is connected from the start.
    bool x2Leg = isPdcpAnchorOnly(flow, compoundId.getNodeId());
    if (!x2Leg) {
        if (pdcpEnt->gate("legOut", legIdx)->isConnectedOutside()) {
            EV << "BearerManagement::installPdcpTxSide - TX side of " << compoundId.str() << " leg " << legIdx << " already installed\n";
            return;
        }

        // Wire compound legOut[legIdx] (← tx/splitter) → RLC entity upperIn (direct per-DRB connection)
        cModule *rlcEnt = lookupRlcEntityModule(rlcId, isNr);
        ASSERT(rlcEnt != nullptr);
        pdcpEnt->gate("legOut", legIdx)->connectTo(rlcEnt->gate("upperIn"));

        // Give the leg splitter this leg's TX RLC entity, so it can weigh its pending data
        // volume against the split threshold. Only a multi-leg bearer has a splitter, and
        // only a local leg (this branch) has an RLC entity to weigh.
        if (cModule *splitterMod = pdcpEnt->getSubmodule("splitter")) {
            auto *rlcTx = check_and_cast<RlcTxEntityBase *>(rlcEnt->getSubmodule("tx"));
            check_and_cast<DcPdcpLegSplitter *>(splitterMod)->setLegRlc(legIdx, rlcTx);
        }
    }
    else if (pdcpTxEntities_.count(compoundId)) {
        EV << "BearerManagement::installPdcpTxSide - TX side of " << compoundId.str() << " already installed\n";
        return;
    }

    if (legIdx == 0) {
        // Wire PdcpMux → compound upperIn (→ tx.in) and register the TX side
        int idx = pdcpMux->gateSize("toTxEntity");
        pdcpMux->setGateSize("toTxEntity", idx + 1);
        pdcpMux->gate("toTxEntity", idx)->connectTo(pdcpEnt->gate("upperIn"));
        pdcpMux->registerTxEntity(compoundId, idx);
        notifyBearerEstablished(compoundId);

        auto *txEnt = check_and_cast<PdcpTxEntityBase *>(pdcpEnt->getSubmodule("tx"));
        pdcpTxEntities_[compoundId] = txEnt;
    }
}

void BearerManagement::installPdcpRxSide(DrbKey id, const FlowId& flow, RlcMode rlcMode, RlcMux *rlcMux, bool isNr)
{
    auto *pdcpMux = inet::getModuleFromPar<PdcpMux>(par("pdcpMuxModule"), this);
    DrbKey rlcId = flow.rxDrbKey();

    DrbKey compoundId = id;
    int legIdx = selectPdcpLeg(flow.sourceId, flow, compoundId);

    cModule *pdcpEnt = findOrCreatePdcpEntity(compoundId, flow, rlcMode, rlcMux);

    // A master's X2 leg was wired to the DcMux at compound creation (see
    // findOrCreatePdcpEntity() and installPdcpTxSide())
    bool x2Leg = isPdcpAnchorOnly(flow, compoundId.getNodeId());
    if (!x2Leg) {
        if (pdcpEnt->gate("legIn", legIdx)->isConnectedOutside()) {
            EV << "BearerManagement::installPdcpRxSide - RX side of " << compoundId.str() << " leg " << legIdx << " already installed\n";
            return;
        }

        // Wire RLC entity upperOut → compound legIn[legIdx] (→ rx/joiner) (direct per-DRB connection)
        cModule *rlcEnt = lookupRlcEntityModule(rlcId, isNr);
        ASSERT(rlcEnt != nullptr);
        rlcEnt->gate("upperOut")->connectTo(pdcpEnt->gate("legIn", legIdx));
    }
    else if (pdcpRxEntities_.count(compoundId)) {
        EV << "BearerManagement::installPdcpRxSide - RX side of " << compoundId.str() << " already installed\n";
        return;
    }

    if (legIdx == 0) {
        // Wire compound upperOut (← rx.out) → PdcpMux fromRxEntity and register the RX side
        int fromIdx = pdcpMux->gateSize("fromRxEntity");
        pdcpMux->setGateSize("fromRxEntity", fromIdx + 1);
        pdcpEnt->gate("upperOut")->connectTo(pdcpMux->gate("fromRxEntity", fromIdx));

        auto *rxEnt = check_and_cast<PdcpRxEntityBase *>(pdcpEnt->getSubmodule("rx"));
        pdcpRxEntities_[compoundId] = rxEnt;
    }
}

RlcTxEntityBase *BearerManagement::createRlcTxBuffer(DrbKey id, const FlowId& flow, const BearerRequest& reqIn)
{
    Enter_Method_Silent("createRlcTxBuffer()");
    const BearerRequest req = resolveBearerRequest(reqIn, flow, flow.destId);
    RlcTxEntityBase *txEnt = installRlcTxSide(id, flow, req, rlcMuxModule.get(), false);
    const DrbDesc& drb = materializeDrb(flow, req, flow.destId, id, false);
    LteMacBase *mac = macModule.get();
    MacCid cid = MacCid(flow.destId, mac->drbIdToLcid(flow.drbId));
    mac->configureLogicalChannel(cid, LogicalChannelConfig{drb.rlcMode, drb.soFraming, drb.snFieldLength, drb.lcg});
    return txEnt;
}

RlcRxEntityBase *BearerManagement::createRlcRxBuffer(DrbKey id, const FlowId& flow, const BearerRequest& reqIn)
{
    Enter_Method_Silent("createRlcRxBuffer()");
    const BearerRequest req = resolveBearerRequest(reqIn, flow, flow.sourceId);
    RlcRxEntityBase *rxEnt = installRlcRxSide(id, flow, req, rlcMuxModule.get(), false);
    const DrbDesc& drb = materializeDrb(flow, req, flow.sourceId, id, false);
    LteMacBase *mac = macModule.get();
    MacCid cid = MacCid(flow.sourceId, mac->drbIdToLcid(flow.drbId));
    mac->configureLogicalChannel(cid, LogicalChannelConfig{drb.rlcMode, drb.soFraming, drb.snFieldLength, drb.lcg});
    return rxEnt;
}

void BearerManagement::deleteLocalPdcpEntities(MacNodeId nodeId)
{
    Enter_Method_Silent("deleteLocalPdcpEntities()");

    auto *pdcpMux = inet::getModuleFromPar<PdcpMux>(par("pdcpMuxModule"), this);
    auto *pdcpDcMux = inet::findModuleFromPar<DcMux>(par("pdcpDcMuxModule"), this); // nullptr on UEs (no X2)

    bool isEnb = (registration_->getNodeType() == NODEB);

    // Per-node (keyed) deletion at eNBs/gNBs and at NR-capable UEs; wipe-all only at plain
    // LTE UEs. This mirrors the pre-flattening per-class behavior (LtePdcpEnb and NrPdcpUe
    // deleted entities keyed by node, LtePdcpUe deleted all). At an NR UE this single module
    // holds the PDCP entities of BOTH legs (keyed by the peer node): a one-leg detach must
    // not delete the other leg's entities, otherwise that leg's RLC RX entities are left
    // forwarding to a dangling gate, and later re-establishment collides with its leftovers.
    bool keyed = isEnb || registration_->getNrNodeId() != NODEID_NONE;

    // The bearer's configuration lives exactly as long as its PDCP entity: a bearer is
    // established on demand, so a stale descriptor would be picked up by the next
    // establishment instead of being rebuilt from the new link's parameters.
    if (keyed)
        drbTableModule->removeDrbsOfPeer(nodeId);
    else
        drbTableModule->removeAllDrbs();

    // Delete full PDCP entity compounds (each deletes its TX and RX side). Unregister the TX
    // from the PdcpMux routing table where one was installed -- a DC NR leg reuses the master's
    // TX via nrOut, so its compound carries only an (idle) TX submodule, never registered.
    for (auto it = pdcpEntities_.begin(); it != pdcpEntities_.end(); ) {
        if (!keyed || it->first.getNodeId() == nodeId) {
            if (pdcpTxEntities_.count(it->first)) {
                pdcpMux->unregisterTxEntity(it->first);
                notifyBearerReleased(it->first);
                pdcpTxEntities_.erase(it->first);
            }
            pdcpRxEntities_.erase(it->first);
            releaseDrbIdOf(it->first);
            it->second->deleteModule();
            it = pdcpEntities_.erase(it);
        } else ++it;
    }

    // Delete the X2 relay compounds (each deletes its DL and UL relay submodule). DC-secondary
    // only, so the DcMux must exist wherever a relay does.
    ASSERT(pdcpRelayEntities_.empty() || pdcpDcMux != nullptr);
    for (auto it = pdcpRelayEntities_.begin(); it != pdcpRelayEntities_.end(); ) {
        if (!keyed || it->first.getNodeId() == nodeId) {
            it->second->deleteModule();
            it = pdcpRelayEntities_.erase(it);
        } else ++it;
    }
}

void BearerManagement::deleteLocalRlcQueues(MacNodeId nodeId, bool nrStack)
{
    Enter_Method_Silent("deleteLocalRlcQueues()");

    bool isEnb = (registration_->getNodeType() == NODEB);

    // At a NODEB, entities are always stored in the default (LTE) maps regardless of which
    // leg the caller serves: createIncoming/OutgoingConnection() computes isNr as
    // (nodeType==UE && ...), which is always false here, and gNB NICs have no nrRlcMux.
    // Honoring the caller's nrStack flag would make this a silent no-op, leaking the UE's
    // entities and crashing on re-establishment after a later handover.
    if (isEnb)
        nrStack = false;

    auto &entities = nrStack ? nrRlcEntities_ : rlcEntities_;
    RlcMux *rlcMux = nrStack ? (nrRlcMuxModule ? nrRlcMuxModule.get() : nullptr) : rlcMuxModule.get();
    if (!rlcMux)
        return;

    // Same keying rule as deleteLocalPdcpEntities(), and for the same reason: an RLC entity
    // and the PDCP entity that feeds it are created together and wired together (the PDCP
    // compound's legOut is connected to the RLC entity's upperIn), so they have to be deleted
    // together. Wiping every RLC entity here while PDCP deletion is keyed by peer left the
    // other peers' PDCP TX forwarding into a disconnected legOut, which the next packet from
    // the application hit ("Gate 'legOut[0]' ... is not connected on the outside").
    bool keyed = isEnb || registration_->getNrNodeId() != NODEID_NONE;

    // Delete the per-bearer RLC entity modules (each deletes its TX and RX side)
    for (auto it = entities.begin(); it != entities.end(); ) {
        if (!keyed || it->first.getNodeId() == nodeId) {
            rlcMux->unregisterRxEntity(it->first);  // no-op if the RX side was never installed
            rlcMux->unregisterTxEntity(it->first);  // likewise for the TX side
            it->second->deleteModule();
            it = entities.erase(it);
        } else ++it;
    }
}

cModule *BearerManagement::lookupPdcpEntityModule(DrbKey id)
{
    auto it = pdcpEntities_.find(id);
    return it != pdcpEntities_.end() ? it->second : nullptr;
}

cModule *BearerManagement::lookupPdcpRelayEntityModule(DrbKey id)
{
    auto it = pdcpRelayEntities_.find(id);
    return it != pdcpRelayEntities_.end() ? it->second : nullptr;
}

PdcpTxEntityBase *BearerManagement::lookupPdcpTxEntity(DrbKey id)
{
    auto it = pdcpTxEntities_.find(id);
    return it != pdcpTxEntities_.end() ? it->second : nullptr;
}

void BearerManagement::pdcpActiveUeUL(std::set<MacNodeId> *ueSet)
{
    for (const auto& [id, rxEntity] : pdcpRxEntities_) {
        if (!rxEntity->isEmpty())
            ueSet->insert(id.getNodeId());
    }
}

} // namespace simu5g
