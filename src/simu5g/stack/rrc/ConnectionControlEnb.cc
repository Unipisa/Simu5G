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

#include "simu5g/stack/rrc/ConnectionControlEnb.h"

#include <algorithm>

#include <inet/common/ModuleAccess.h>
#include <inet/common/stlutils.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include "simu5g/stack/rrc/HandoverController.h"
#include "simu5g/stack/rrc/Registration.h"

namespace simu5g {

Define_Module(ConnectionControlEnb);

using namespace omnetpp;
using namespace inet;

typedef BearerConfigurator::AuthoredBearer AuthoredBearer;

void ConnectionControlEnb::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        nodeId_ = MacNodeId(par("macNodeId").intValue());

        // the core network gateway, unless the node is not connected to a core network
        // (a secondary node)
        cModule *node = getContainingNode(this);
        if (node->gate("ppp$o")->isConnected()) {
            gateway_ = par("gateway").stdstringValue();
            if (gateway_.empty())
                throw cRuntimeError("The required 'gateway' parameter is empty");
        }

        binder_.reference(this, "binderModule", true);
        bearerConfigurator_.reference(this, "bearerConfiguratorModule", true);
        coreControl_.reference(this, "coreControlModule", true);
        bearerManagement_.reference(this, "bearerManagementModule", true);
        mac_.reference(this, "macModule", true);
        handoverPacketHolder_.reference(this, "handoverPacketHolderModule", true);
        gtpUser_.reference(this, "gtpUserModule", true);
        gtpUserX2_.reference(this, "gtpUserX2Module", true);
    }
}

Teid ConnectionControlEnb::allocateTeid()
{
    if (lastTeid_ == Teid(UINT32_MAX))
        throw cRuntimeError("ConnectionControlEnb: the TEID space of %s is exhausted", getContainingNode(this)->getFullPath().c_str());
    lastTeid_ = Teid(num(lastTeid_) + 1);
    return lastTeid_;
}

const L3Address& ConnectionControlEnb::getAddress()
{
    if (address_.isUnspecified())
        address_ = L3AddressResolver().resolve(getContainingNode(this)->getFullPath().c_str());
    return address_;
}

ConnectionControlBase *ConnectionControlEnb::controlOf(MacNodeId nodeId)
{
    // a UE's entry point is per leg: the controller of the leg the id names
    cModule *rrc = binder_->getRrcByNodeId(nodeId);
    const char *name = getNodeTypeById(nodeId) == NODEB ? "connectionControl" : isNrUe(nodeId) ? "nrHandoverController" : "handoverController";
    auto *control = rrc != nullptr ? dynamic_cast<ConnectionControlBase *>(rrc->getSubmodule(name)) : nullptr;
    if (control == nullptr)
        throw cRuntimeError("ConnectionControlEnb: node %d has no rrc.%s module", (int)num(nodeId), name);
    return control;
}

ConnectionControlEnb *ConnectionControlEnb::baseStationControl(MacNodeId bsId)
{
    if (bsId == nodeId_)
        return this;
    auto *bs = dynamic_cast<ConnectionControlEnb *>(controlOf(bsId));
    if (bs == nullptr)
        throw cRuntimeError("ConnectionControlEnb: node %d is no base station", (int)num(bsId));
    return bs;
}

HandoverController *ConnectionControlEnb::ueControl(MacNodeId legId)
{
    auto *ue = dynamic_cast<HandoverController *>(controlOf(legId));
    if (ue == nullptr)
        throw cRuntimeError("ConnectionControlEnb: node %d is no UE", (int)num(legId));
    return ue;
}

// ---- handover ----

void ConnectionControlEnb::measurementReport(MacNodeId legId, const MeasurementReport& report)
{
    Enter_Method("measurementReport");
    MacNodeId target = selectHandoverTarget(legId, report);
    if (target == NODEID_NONE) {
        EV_INFO << "ConnectionControlEnb: no handover for leg " << legId << " on its report (serving " << report.servingCell
                << " at " << report.servingRssi << ", best " << report.bestCell << " at " << report.bestRssi << ")" << endl;
        return;
    }
    handoverTargets_[legId] = target;
    ueControl(legId)->handoverCommand(target);
}

MacNodeId ConnectionControlEnb::selectHandoverTarget(MacNodeId legId, const MeasurementReport& report)
{
    return report.bestCell;
}

void ConnectionControlEnb::reconfigurationComplete(MacNodeId legId, MacNodeId sourceBsId)
{
    Enter_Method("reconfigurationComplete");
    attachAtAmc(legId);
    coreControl_->pathSwitchRequest(legId, this);
    baseStationControl(sourceBsId)->ueContextRelease(legId);
    handoverPacketHolder_->signalHandoverCompleteTarget(legId, sourceBsId);
}

void ConnectionControlEnb::ueContextRelease(MacNodeId legId)
{
    Enter_Method("ueContextRelease");
    auto it = handoverTargets_.find(legId);
    if (it == handoverTargets_.end())
        throw cRuntimeError("ConnectionControlEnb: base station %d is no handover source of leg %d", (int)num(nodeId_), (int)num(legId));
    MacNodeId target = it->second;
    handoverTargets_.erase(it);
    releaseLeg(legId);
    detachAtAmc(legId);
    handoverPacketHolder_->signalHandoverCompleteSource(legId, target);
}

void ConnectionControlEnb::connectionLost(MacNodeId legId)
{
    Enter_Method("connectionLost");
    releaseLeg(legId);
    detachAtAmc(legId);
    coreControl_->ueContextReleaseRequest(legId);
}

void ConnectionControlEnb::downlinkPathSwitched(MacNodeId ueLteId, MacNodeId ueNrId, MacNodeId fromBaseStation)
{
    Enter_Method("downlinkPathSwitched");
    handoverPacketHolder_->switchDownlinkPath(ueLteId, ueNrId, fromBaseStation);
}

void ConnectionControlEnb::releasePdcpEntities(MacNodeId legId)
{
    Enter_Method("releasePdcpEntities");
    bearerManagement_->deleteLocalPdcpEntities(legId);
}

MacNodeId ConnectionControlEnb::otherLegOf(MacNodeId legId)
{
    auto *reg = check_and_cast<Registration *>(binder_->getRrcByNodeId(legId)->getSubmodule("registration"));
    return legId == reg->getLteNodeId() ? reg->getNrNodeId() : reg->getLteNodeId();
}

void ConnectionControlEnb::releaseLeg(MacNodeId legId)
{
    mac_->deleteQueues(legId);
    bearerManagement_->deleteLocalRlcQueues(legId, isNrUe(legId));

    // The leg's PDCP entities live at the node anchoring its bearers: at this node's
    // master when this node is a secondary (the bypass entities keyed by the leg stay
    // here as well), else here
    MacNodeId masterId = binder_->getMasterNodeOrSelf(nodeId_);
    if (masterId != nodeId_)
        baseStationControl(masterId)->releasePdcpEntities(legId);
    bearerManagement_->deleteLocalPdcpEntities(legId);

    MacNodeId otherLegId = otherLegOf(legId);
    if (otherLegId != NODEID_NONE && binder_->getServingNode(otherLegId) == NODEID_NONE) {
        MacNodeId secondaryId = binder_->getSecondaryNode(nodeId_);
        if (secondaryId != NODEID_NONE)
            baseStationControl(secondaryId)->releaseLeg(otherLegId);
    }
}

void ConnectionControlEnb::attachAtAmc(MacNodeId legId)
{
    LteAmc *amc = mac_->getAmc();
    amc->attachUser(legId, UL);
    amc->attachUser(legId, DL);
}

void ConnectionControlEnb::detachAtAmc(MacNodeId legId)
{
    LteAmc *amc = mac_->getAmc();
    amc->detachUser(legId, UL);
    amc->detachUser(legId, DL);
}

// ---- the node's tunnels ----

FTeid ConnectionControlEnb::sessionTunnelSetup(const SessionRef& session, const UplinkTunnels& uplink)
{
    Enter_Method("sessionTunnelSetup");
    return setUpSessionTunnels(session, uplink);
}

FTeid ConnectionControlEnb::setUpSessionTunnels(const SessionRef& session, const UplinkTunnels& uplink)
{
    FTeid dl{getAddress(), allocateTeid()};
    gtpUser_->addTunnel(dl.teid, session);
    gtpUser_->setUplinkTunnels(session, uplink);
    // the same TEID receives the downlink a handover source forwards over X2-U
    gtpUserX2_->addTunnel(dl.teid, session);
    return dl;
}

// ---- attach ----

void ConnectionControlEnb::connectionSetupRequest(cModule *ueModule, MacNodeId legId, ConnectionControlBase *ueRrc)
{
    Enter_Method("connectionSetupRequest");
    // at initialization the UE's MAC attached itself at the AMC already (LteMacUe)
    if (getSimulation()->getContextType() != CTX_INITIALIZE)
        attachAtAmc(legId);
    coreControl_->initialUeMessage(legId, this);
}

FTeid ConnectionControlEnb::sessionResourceSetup(MacNodeId legId, const SessionRef& session, const UplinkTunnels& uplink, QfiRuleSet&& ulQfiRules)
{
    Enter_Method("sessionResourceSetup");
    FTeid dl = setUpSessionTunnels(session, uplink);

    // the UE's uplink QoS rules: a stack with SDAP classifies its uplink QoS flows by them
    cModule *ueModule = binder_->getNodeModule(legId);
    ASSERT(ueModule != nullptr);
    if (BearerConfigurator::ueStackHasSdap(ueModule))
        controlOf(legId)->setUplinkQfiRules(std::move(ulQfiRules));

    if (carriesStaticDrbs(ueModule, legId))
        establishStaticDrbs(ueModule, legId);
    return dl;
}

bool ConnectionControlEnb::carriesStaticDrbs(cModule *ueModule, MacNodeId legId)
{
    // the UE's registered node id(s) -- one per stack
    MacNodeId lteUeId = NODEID_NONE, nrUeId = NODEID_NONE;
    for (const auto& [nodeId, info] : binder_->getNodeInfoMap())
        if (info.moduleRef == ueModule)
            (num(nodeId) >= NR_UE_MIN_ID ? nrUeId : lteUeId) = nodeId;

    bool lteAttached = lteUeId != NODEID_NONE && binder_->getServingNode(lteUeId) != NODEID_NONE;
    bool nrAttached = nrUeId != NODEID_NONE && binder_->getServingNode(nrUeId) != NODEID_NONE;
    ASSERT(lteAttached || nrAttached);   // this leg is
    MacNodeId lteNodeB = lteAttached ? binder_->getServingNode(lteUeId) : NODEID_NONE;
    bool dcSetup = lteNodeB != NODEID_NONE &&
            (binder_->getSecondaryNode(lteNodeB) != NODEID_NONE || binder_->getMasterNodeOrSelf(lteNodeB) != lteNodeB);
    MacNodeId ueId = (lteAttached && nrAttached && dcSetup) ? lteUeId :
                     nrAttached ? nrUeId : lteUeId;
    return ueId == legId;
}

void ConnectionControlEnb::establishStaticDrbs(cModule *ueModule, MacNodeId legId)
{
    // the descriptors first, which the establishment consults
    for (const AuthoredBearer& ab : bearerConfigurator_->getBearerDefinitions())
        if (!ab.onDemand && ab.ueModule == ueModule)
            pushDrbToRrcs(ueModule, ab.desc);

    for (const AuthoredBearer& ab : bearerConfigurator_->getBearerDefinitions()) {
        if (ab.onDemand || ab.ueModule != ueModule)
            continue;
        FlowId flow;
        flow.sourceId = legId;
        flow.destId = binder_->getServingNode(legId);
        flow.direction = UL;
        flow.drbId = ab.desc.getDrbId();

        EV << "ConnectionControlEnb::establishStaticDrbs - establishing DRB " << flow.drbId << " of UE '"
           << ueModule->getFullPath() << "' (nodeId=" << legId << ") towards serving node "
           << flow.destId << endl;
        establishDataConnection(flow, BearerRequest{ab.desc.rlcMode, ab.desc.lcg});
    }
}

void ConnectionControlEnb::setForwardingTeid(const SessionRef& session, MacNodeId bsId, Teid teid)
{
    Enter_Method("setForwardingTeid");
    gtpUserX2_->setForwardingTeid(session, bsId, teid);
}

void ConnectionControlEnb::sessionRelease(const SessionRef& session)
{
    Enter_Method("sessionRelease");
    gtpUser_->removeSession(session);
    gtpUserX2_->removeSession(session);
}

Teid ConnectionControlEnb::addDcTunnel(MacNodeId ueNodeId, DrbId drbId, Direction direction)
{
    Enter_Method("addDcTunnel");
    Teid teid = allocateTeid();
    gtpUserX2_->addDcTunnel(teid, ueNodeId, drbId, direction);
    return teid;
}

void ConnectionControlEnb::setDcTunnelTeid(MacNodeId ueLteId, MacNodeId ueNrId, DrbId drbId, Direction direction, Teid teid)
{
    Enter_Method("setDcTunnelTeid");
    gtpUserX2_->setDcTunnelTeid(ueLteId, ueNrId, drbId, direction, teid);
}

// ---- DRB identities ----

std::set<DrbId>& ConnectionControlEnb::foreignPairPool(const std::pair<MacNodeId, MacNodeId>& pair)
{
    throw cRuntimeError("ConnectionControlEnb: base station %d is no party to the node pair (%d, %d), and has no D2D support to pool its DRB ids",
            (int)num(nodeId_), (int)num(pair.first), (int)num(pair.second));
}

DrbId ConnectionControlEnb::assignDrbId(MacNodeId a, MacNodeId b)
{
    auto pair = std::minmax(a, b);
    std::pair<MacNodeId, MacNodeId> pairKey{pair.first, pair.second};
    auto& inUse = (a == nodeId_ || b == nodeId_) ? drbIdsInUse_[pairKey] : foreignPairPool(pairKey);

    // Lowest free ID: identities released with their bearer are handed out again, which
    // is what keeps the space bounded for a UE that establishes and releases bearers
    // repeatedly (at every handover, say).
    unsigned short id = 1;
    while (inUse.count(DrbId(id)))
        id++;
    if (id > MAX_DRB_ID)
        throw cRuntimeError("ConnectionControlEnb::assignDrbId - out of DRB identities for the node pair (%hu, %hu): "
                "all %d are in use", num(pair.first), num(pair.second), MAX_DRB_ID);

    inUse.insert(DrbId(id));
    return DrbId(id);
}

void ConnectionControlEnb::reserveDrbId(MacNodeId a, MacNodeId b, DrbId drbId)
{
    Enter_Method_Silent("reserveDrbId");
    auto pair = std::minmax(a, b);
    std::pair<MacNodeId, MacNodeId> pairKey{pair.first, pair.second};
    auto& inUse = (a == nodeId_ || b == nodeId_) ? drbIdsInUse_[pairKey] : foreignPairPool(pairKey);
    inUse.insert(drbId);
}

void ConnectionControlEnb::releaseDrbId(MacNodeId a, MacNodeId b, DrbId drbId)
{
    auto pair = std::minmax(a, b);
    std::pair<MacNodeId, MacNodeId> pairKey{pair.first, pair.second};
    bool released;
    if (a == nodeId_ || b == nodeId_) {
        auto it = drbIdsInUse_.find(pairKey);
        released = it != drbIdsInUse_.end() && it->second.erase(drbId) != 0;
    }
    else
        released = foreignPairPool(pairKey).erase(drbId) != 0;
    if (released)
        EV << "ConnectionControlEnb::releaseDrbId - DRB " << drbId << " of the node pair (" << pair.first
           << ", " << pair.second << ") is free again" << endl;
}

void ConnectionControlEnb::forgetOnDemandDrbId(cModule *ueModule, MacNodeId a, MacNodeId b, DrbId drbId)
{
    std::pair<MacNodeId, MacNodeId> pairKey = std::minmax(a, b);
    for (const AuthoredBearer& ab : bearerConfigurator_->getBearerDefinitions()) {
        if (!ab.onDemand || ab.ueModule != ueModule)
            continue;
        auto it = onDemandIds_.find({&ab, pairKey});
        if (it != onDemandIds_.end() && it->second == drbId)
            onDemandIds_.erase(it);
    }
}

void ConnectionControlEnb::bearerReleased(DrbKey bearer)
{
    Enter_Method_Silent("bearerReleased");
    // A static definition owns its id for the whole run: releasing it would let
    // assignDrbId() hand the id to an unrelated bearer while the definition still
    // names it. An on-demand definition's id is pair-scoped like any other bearer's:
    // it returns to the pool with its bearer, and the definition materializes afresh
    // on the next match -- which after a handover is a new node pair. A base station's
    // bearers are keyed by their UE.
    MacNodeId ueNodeId = bearer.getNodeId();
    cModule *ueModule = binder_->getNodeModule(ueNodeId);
    if (ueModule != nullptr && bearerConfigurator_->findStaticDrbDefinition(ueModule, bearer.getDrbId()) != nullptr)
        return;
    releaseDrbId(nodeId_, ueNodeId, bearer.getDrbId());
    if (ueModule != nullptr)
        forgetOnDemandDrbId(ueModule, nodeId_, ueNodeId, bearer.getDrbId());
}

// ---- bearer installation at this node, from a peer's control plane ----

void ConnectionControlEnb::configureDrb(const DrbDesc& drb)
{
    Enter_Method("configureDrb");
    bearerManagement_->configureDrb(drb);
}

void ConnectionControlEnb::createIncomingConnection(const FlowId& flow, const BearerRequest& req, bool withPdcp)
{
    Enter_Method("createIncomingConnection");
    bearerManagement_->createIncomingConnection(flow, req, withPdcp);
}

void ConnectionControlEnb::createOutgoingConnection(const FlowId& flow, const BearerRequest& req, bool withPdcp)
{
    Enter_Method("createOutgoingConnection");
    bearerManagement_->createOutgoingConnection(flow, req, withPdcp);
}

void ConnectionControlEnb::setUplinkQfiRules(QfiRuleSet&& rules)
{
    throw cRuntimeError("ConnectionControlEnb: a base station takes no uplink QFI rules");
}

// ---- bearer establishment ----

void ConnectionControlEnb::installStaticDrb(cModule *ueModule, const DrbDesc& drb)
{
    Enter_Method("installStaticDrb");
    pushDrbToRrcs(ueModule, drb);
}

void ConnectionControlEnb::pushDrbToRrcs(cModule *ueModule, const DrbDesc& drb)
{
    // node ids of the UE module, one per stack
    std::vector<MacNodeId> nodeIds;
    for (const auto& [nodeId, info] : binder_->getNodeInfoMap())
        if (getNodeTypeById(nodeId) == UE && info.moduleRef == ueModule)
            nodeIds.push_back(nodeId);
    ASSERT(!nodeIds.empty());

    DrbId drbId = drb.getDrbId();

    // The UE keys its bearers by "my serving node" (NODEID_NONE), its serving
    // node by the UE. A dual-stack UE has one bearer per stack id, and the
    // serving node of each stack is told about the one that is its own.
    DrbDesc ueDrb = drb;
    ueDrb.key = DrbKey(NODEID_NONE, drbId);
    controlOf(nodeIds.front())->configureDrb(ueDrb);

    for (MacNodeId ueId : nodeIds) {
        MacNodeId servingNodeId = binder_->getServingNode(ueId);
        if (servingNodeId == NODEID_NONE)
            continue;   // this stack is not attached to a cell
        DrbDesc enbDrb = drb;
        enbDrb.key = DrbKey(ueId, drbId);
        ConnectionControlEnb *servingBs = baseStationControl(servingNodeId);
        if (servingBs == this)
            bearerManagement_->configureDrb(enbDrb);
        else
            servingBs->configureDrb(enbDrb);

        // The configuration names the bearer, so its id is taken out of the pool
        // that assignDrbId() hands out to bearers that are not configured here
        servingBs->reserveDrbId(ueId, servingNodeId, drbId);
    }
}

DrbId ConnectionControlEnb::establishD2dBearer(const FlowId& flow, const FlowBindingKey& key)
{
    throw cRuntimeError("ConnectionControlEnb: base station %d has no D2D support to establish a sidelink bearer for flow %d -> %d",
            (int)num(nodeId_), (int)num(flow.sourceId), (int)num(flow.destId));
}

DrbId ConnectionControlEnb::establishBearer(const FlowId& flow, const FlowBindingKey& key, const inet::Packet *pkt)
{
    Enter_Method_Silent("establishBearer");

    // D2D and multicast bearers are outside the definition system (definitions
    // describe infrastructure bearers), and are established with a fixed transitional
    // configuration -- RLC UM on LCG 3, the non-GBR default bearer's group -- until
    // they get definitions of their own.
    if (flow.d2dGroupId != NODEID_NONE || flow.d2dTxPeerId != NODEID_NONE || flow.d2dRxPeerId != NODEID_NONE)
        return establishD2dBearer(flow, key);

    // The requester brings identity only; the bearer's properties come from the
    // definition the flow matches. First matching definition wins, in table order
    // (staticDrbs records are retained ahead of onDemandDrbs ones); the default eps
    // entry catches the flows no filter matched.
    MacNodeId ueId = getNodeTypeById(flow.sourceId) == UE ? flow.sourceId : flow.destId;
    cModule *ueModule = binder_->getNodeModule(ueId);
    if (ueModule != nullptr)
        if (const AuthoredBearer *ab = bearerConfigurator_->findDrbDefinition(ueModule, pkt))
            return establishFromDefinition(*ab, flow, key);

    // Every on-demand bearer's properties come from a definition entry, never from
    // the packet; the onDemandDrbs default value carries catch-all definitions, so
    // only a configuration that replaced them with a non-covering set can get here.
    throw cRuntimeError("no bearer definition covers packet '%s' of UE '%s' (nodeId=%d) -- an on-demand "
            "bearer requires a covering staticDrbs/onDemandDrbs entry",
            pkt->getName(), ueModule ? ueModule->getFullPath().c_str() : "?", (int)num(ueId));
}

DrbId ConnectionControlEnb::establishFromDefinition(const AuthoredBearer& ab, const FlowId& flowIn, const FlowBindingKey& key)
{
    FlowId flow = flowIn;

    if (!ab.onDemand) {
        // A static definition's id is pinned, and its descriptor was delivered to the
        // RRCs at initialization: the flow simply joins the configured bearer.
        flow.drbId = ab.desc.getDrbId();
    }
    else {
        // An on-demand DRB id is pair-scoped, so the definition materializes once per
        // node pair: the first match within a pair assigns the pair's lowest free id
        // and delivers the definition to the RRCs involved, and later flows matching
        // the definition join that bearer. After a handover the new pair assigns
        // afresh, and a torn-down bearer's id returns to its pool (see
        // forgetOnDemandDrbId()) -- exactly the identity lifecycle of a bearer nobody
        // authored.
        std::pair<MacNodeId, MacNodeId> pairKey = std::minmax(flow.sourceId, flow.destId);
        auto it = onDemandIds_.find({&ab, pairKey});
        if (it == onDemandIds_.end()) {
            DrbId drbId = assignDrbId(flow.sourceId, flow.destId);
            it = onDemandIds_.insert({{&ab, pairKey}, drbId}).first;
            DrbDesc desc = ab.desc;
            desc.key = DrbKey(NODEID_NONE, drbId);
            desc.lcid = LogicalCid(num(drbId));
            EV << "ConnectionControlEnb::establishFromDefinition - on-demand definition materialized as DRB " << drbId
               << " for UE " << ab.ueModule->getFullPath() << endl;
            pushDrbToRrcs(ab.ueModule, desc);
        }
        flow.drbId = it->second;
    }
    return establishDataConnection(flow, BearerRequest{ab.desc.rlcMode, ab.desc.lcg, key});
}

DrbId ConnectionControlEnb::resolveDrbForQfi(MacNodeId ueNodeId, Qfi qfi)
{
    Enter_Method_Silent("resolveDrbForQfi");

    cModule *ueModule = binder_->getNodeModule(ueNodeId);
    if (ueModule == nullptr)
        return DRBID_NONE;
    if (const AuthoredBearer *ab = bearerConfigurator_->findDrbDefinitionForQfi(ueModule, qfi))
        return drbOfDefinition(*ab, ueNodeId);
    return DRBID_NONE;
}

DrbId ConnectionControlEnb::drbOfDefinition(const AuthoredBearer& ab, MacNodeId ueNodeId)
{
    // A static definition's bearer was established up front under its pinned id
    if (!ab.onDemand)
        return ab.desc.getDrbId();

    // An on-demand definition materializes once per node pair, like
    // establishFromDefinition(): the id is assigned and the descriptor delivered to the
    // RRCs involved (so it also reaches SDAP's QFI-to-DRB table), and later lookups join
    // the bearer already made.
    MacNodeId servingNodeId = binder_->getServingNode(ueNodeId);
    if (servingNodeId == NODEID_NONE)
        return DRBID_NONE;   // not attached, nowhere to create the bearer
    std::pair<MacNodeId, MacNodeId> pairKey = std::minmax(ueNodeId, servingNodeId);
    auto it = onDemandIds_.find({&ab, pairKey});
    if (it == onDemandIds_.end()) {
        DrbId drbId = assignDrbId(ueNodeId, servingNodeId);
        it = onDemandIds_.insert({{&ab, pairKey}, drbId}).first;
        DrbDesc desc = ab.desc;
        desc.key = DrbKey(NODEID_NONE, drbId);
        desc.lcid = LogicalCid(num(drbId));
        EV << "ConnectionControlEnb::drbOfDefinition - on-demand DRB " << drbId
           << " materialized at UE " << ab.ueModule->getFullPath() << endl;
        pushDrbToRrcs(ab.ueModule, desc);
    }
    return it->second;
}

// The definition a flow's bearer was authored from, if any: the entry whose UE and DRB id
// the flow names. Definitions describe infrastructure bearers only, so a D2D or multicast
// flow never has one.
const DrbDesc *ConnectionControlEnb::findBearerDefinition(const FlowId& flow)
{
    if (flow.d2dGroupId != NODEID_NONE || flow.d2dTxPeerId != NODEID_NONE || flow.d2dRxPeerId != NODEID_NONE)
        return nullptr;
    MacNodeId ueId = (getNodeTypeById(flow.sourceId) == UE) ? flow.sourceId : flow.destId;
    if (getNodeTypeById(ueId) != UE)
        return nullptr;
    cModule *ueModule = binder_->getNodeModule(ueId);
    std::pair<MacNodeId, MacNodeId> pairKey = std::minmax(flow.sourceId, flow.destId);
    for (const AuthoredBearer& ab : bearerConfigurator_->getBearerDefinitions()) {
        if (ab.ueModule != ueModule)
            continue;
        if (!ab.onDemand && ab.desc.getDrbId() == flow.drbId)
            return &ab.desc;
        if (ab.onDemand) {
            // an on-demand definition's id is per node pair (see establishFromDefinition())
            auto it = onDemandIds_.find({&ab, pairKey});
            if (it != onDemandIds_.end() && it->second == flow.drbId)
                return &ab.desc;
        }
    }
    return nullptr;
}

bool ConnectionControlEnb::isDualConnectivityRequired(const FlowId& flow)
{
    MacNodeId sourceId = flow.sourceId;
    MacNodeId destId = flow.destId;

    // Part 1: Check if NodeB is in DC setup
    MacNodeId nodeB = (getNodeTypeById(sourceId) == UE) ? binder_->getServingNode(sourceId) : sourceId;
    ASSERT(nodeB != NODEID_NONE);

    MacNodeId secondaryNode = binder_->getSecondaryNode(nodeB);
    MacNodeId masterNode = binder_->getMasterNodeOrSelf(nodeB);
    bool nodeBInDC = (secondaryNode != NODEID_NONE) || (masterNode != nodeB);

    // Part 2: Check if UE is dual technology capable
    MacNodeId ue = getNodeTypeById(sourceId) == UE ? sourceId :
                   getNodeTypeById(destId) == UE ? destId :
                   NODEID_NONE;

    bool ueIsDualTech = false;  //TODO true? if a nodeB in DC setup sends multicast, can it use dual connectivity?
    if (ue != NODEID_NONE) {
        Registration *reg = check_and_cast<Registration*>(binder_->getRrcByNodeId(ue)->getSubmodule("registration"));
        ueIsDualTech = reg->isDualTechnology();
    }

    return nodeBInDC && ueIsDualTech;
}

DrbId ConnectionControlEnb::establishBearer(const FlowId& flow, const BearerRequest& req)
{
    Enter_Method_Silent("establishBearer");
    return establishDataConnection(flow, req);
}

DrbId ConnectionControlEnb::establishDataConnection(const FlowId& flowIn, const BearerRequest& reqIn)
{
    // Assign the bearer's DRB id unless the requester brought one (SDAP and the
    // static definitions name their bearers explicitly). IDs are unique per node
    // pair; for multicast the "pair" is (sender, group), there being no single peer.
    FlowId flow = flowIn;
    MacNodeId peerId = (flow.d2dGroupId != NODEID_NONE) ? flow.d2dGroupId : flow.destId;
    if (flow.drbId == DRBID_NONE) {
        flow.drbId = assignDrbId(flow.sourceId, peerId);
        EV << "ConnectionControlEnb::establishBearer - new DRB ID assigned: " << flow.drbId << endl;
    }
    else
        reserveDrbId(flow.sourceId, peerId, flow.drbId);   // named by the requester; keep assignDrbId off it

    // A request that states no RLC mode (SDAP's, for one) takes it, and the LCG, from
    // the bearer's definition entry. Definition entries always state their RLC mode,
    // so the request RRC receives is always concrete.
    BearerRequest req = reqIn;
    if (req.rlcMode == UNKNOWN_RLC_MODE) {
        const DrbDesc *def = findBearerDefinition(flow);
        if (def == nullptr)
            throw cRuntimeError("bearer establishment for DRB %d carries no RLC mode, and no definition "
                    "entry names that DRB -- a request that states no configuration is only valid for "
                    "definition-covered bearers", (int)num(flow.drbId));
        req.rlcMode = def->rlcMode;
        req.lcg = def->lcg;
    }

    bool dualConnected = isDualConnectivityRequired(flow);
    if (!dualConnected) {
        // Without a secondary cell group there is nothing to carry an SCG leg
        if (const DrbDesc *def = findBearerDefinition(flow))
            if (std::any_of(def->legs.begin(), def->legs.end(),
                    [](const RlcBearerDesc& leg) { return leg.cellGroup == SCG; }))
                throw cRuntimeError("ConnectionControlEnb: the definition of DRB %d states an SCG leg, "
                        "but the flow's UE is not served in dual connectivity -- there is no secondary "
                        "cell group to carry it", (int)num(flow.drbId));
        createConnection(flow, req, true);
    }
    else {
        MacNodeId sourceId = flow.sourceId;
        MacNodeId destId = flow.destId;
        bool isGroupcast = flow.d2dGroupId != NODEID_NONE;

        // Get UE registration if any endpoint is UE
        Registration *ueReg = (getNodeTypeById(sourceId) == UE) ? check_and_cast<Registration*>(binder_->getRrcByNodeId(sourceId)->getSubmodule("registration")) :
                     (!isGroupcast && getNodeTypeById(destId) == UE) ? check_and_cast<Registration*>(binder_->getRrcByNodeId(destId)->getSubmodule("registration")) :
                     nullptr;

        // Which of the UE's two ids belongs to which cell group. A UE's stacks pair with
        // their serving nodes by technology, so the master node's technology decides:
        // under EN-DC the master is an eNB and the master cell group is the UE's LTE
        // stack; under NE-DC the master is a gNB and it is the NR stack.
        //
        // The master's technology is asked of the node itself, not of the UE's current
        // attachment. Attachment moves during a handover, and a stack whose serving node
        // is mid-change matches neither cell group for as long as that lasts.
        MacNodeId masterNodeB = binder_->getMasterNodeOrSelf(getNodeTypeById(sourceId) == UE ? destId : sourceId);
        bool masterIsNr = binder_->isNrNodeB(masterNodeB);
        MacNodeId ueMcgId = ueReg ? (masterIsNr ? ueReg->getNrNodeId() : ueReg->getLteNodeId()) : NODEID_NONE;
        MacNodeId ueScgId = ueReg ? (masterIsNr ? ueReg->getLteNodeId() : ueReg->getNrNodeId()) : NODEID_NONE;

        // A bearer whose definition states its legs is established on those and no others:
        // an MCG bearer never reaches the secondary node, and an SCG bearer's traffic is
        // carried by no cell group of the master's own -- though the master still
        // terminates its PDCP, since the core network delivers the UE's traffic there.
        // A definition that leaves the legs to RRC gets both legs, as before.
        bool hasMcgLeg = true, hasScgLeg = true;
        if (const DrbDesc *def = findBearerDefinition(flow)) {
            if (!def->legs.empty()) {
                hasMcgLeg = std::any_of(def->legs.begin(), def->legs.end(),
                        [](const RlcBearerDesc& leg) { return leg.cellGroup == MCG; });
                hasScgLeg = std::any_of(def->legs.begin(), def->legs.end(),
                        [](const RlcBearerDesc& leg) { return leg.cellGroup == SCG; });
            }
        }

        // Master cell group connection
        FlowId lteFlow = flow;
        lteFlow.sourceId = getNodeTypeById(sourceId) == UE ?
                            ueMcgId :
                            binder_->getMasterNodeOrSelf(sourceId);
        if (!isGroupcast) {  // Only set destId for unicast
            lteFlow.destId = getNodeTypeById(destId) == UE ?
                              ueMcgId :
                              binder_->getMasterNodeOrSelf(destId);
        }
        if (hasMcgLeg)
            createConnection(lteFlow, req, true);
        else {
            // An SCG bearer: only the master's own ends are established -- its RRC wires
            // the PDCP legs to the X2 path instead of local RLC (see
            // BearerManagement::createOutgoingConnection()) -- and the UE's MCG stack is
            // not involved at all. The flow keeps the anchor (MCG) ids: they are what the
            // master's PDCP is keyed and addressed by, and the leg splitter maps them to
            // the SCG per PDU, exactly as on a split bearer's secondary leg.
            ASSERT(!isGroupcast);   // definitions never cover D2D/multicast flows
            FlowId revFlow = lteFlow.reversed();
            BearerRequest revReq = req;
            if (revReq.flowBindingKey.has_value())
                revReq.flowBindingKey = revReq.flowBindingKey->reversed();
            if (lteFlow.sourceId == masterNodeB) {
                createOutgoingConnectionOnNode(masterNodeB, lteFlow, req, true);
                createIncomingConnectionOnNode(masterNodeB, revFlow, revReq, true);
            }
            else {
                createIncomingConnectionOnNode(masterNodeB, lteFlow, req, true);
                createOutgoingConnectionOnNode(masterNodeB, revFlow, revReq, true);
            }
        }

        // Secondary cell group connection
        FlowId nrFlow = flow;
        nrFlow.sourceId = getNodeTypeById(sourceId) == UE ?
                           ueScgId :
                           binder_->getSecondaryNode(binder_->getMasterNodeOrSelf(sourceId));
        if (!isGroupcast) {  // Only set destId for unicast
            nrFlow.destId = getNodeTypeById(destId) == UE ?
                             ueScgId :
                             binder_->getSecondaryNode(binder_->getMasterNodeOrSelf(destId));
        }
        if (hasScgLeg) {
            createConnection(nrFlow, req, false);
            if (!isGroupcast && ueReg != nullptr) {
                MacNodeId secondaryNodeB = getNodeTypeById(nrFlow.sourceId) == UE ? nrFlow.destId : nrFlow.sourceId;
                setUpX2DcTunnels(masterNodeB, secondaryNodeB, ueReg->getLteNodeId(), ueReg->getNrNodeId(), ueMcgId, ueScgId, flow.drbId);
            }
        }
    }
    return flow.drbId;
}

void ConnectionControlEnb::setUpX2DcTunnels(MacNodeId masterId, MacNodeId secondaryId, MacNodeId ueLteId, MacNodeId ueNrId,
        MacNodeId ueMcgId, MacNodeId ueScgId, DrbId drbId)
{
    // The receiving end of each direction allocates its tunnel's TEID: the secondary for
    // the downlink the master relays to it, the master for the uplink the secondary
    // relays back. Each end keys the bearer by the UE's id on the stack it serves.
    ConnectionControlEnb *master = baseStationControl(masterId);
    ConnectionControlEnb *secondary = baseStationControl(secondaryId);
    Teid dlTeid = secondary->addDcTunnel(ueScgId, drbId, DL);
    master->setDcTunnelTeid(ueLteId, ueNrId, drbId, DL, dlTeid);
    Teid ulTeid = master->addDcTunnel(ueMcgId, drbId, UL);
    secondary->setDcTunnelTeid(ueLteId, ueNrId, drbId, UL, ulTeid);
}

void ConnectionControlEnb::createConnection(const FlowId& flow, const BearerRequest& req, bool withPdcp)
{
    MacNodeId sourceId = flow.sourceId;
    MacNodeId destId = flow.destId;
    MacNodeId groupId = flow.d2dGroupId;

    EV << "ConnectionControlEnb::createConnection - establishing connection from sourceId=" << sourceId
       << " to destId=" << destId << " groupId=" << groupId << endl;

    bool sourceIsEnb = getNodeTypeById(sourceId) == NODEB;
    bool destIsEnb = getNodeTypeById(destId) == NODEB;
    ASSERT(!sourceIsEnb || !destIsEnb);  // they cannot be both NodeBs

    bool sourceWithPdcp = getNodeTypeById(sourceId)==UE || withPdcp;
    createOutgoingConnectionOnNode(sourceId, flow, req, sourceWithPdcp);

    if (groupId == NODEID_NONE) {
        bool destWithPdcp = getNodeTypeById(destId)==UE || withPdcp;
        createIncomingConnectionOnNode(destId, flow, req, destWithPdcp);

        // A DRB is bidirectional (TS 38.331): create the reverse leg of the bearer
        // at both endpoints as well, so reverse traffic -- user data or RLC-AM
        // STATUS PDUs -- finds its entities in place instead of establishing a
        // separate unidirectional bearer.
        // The reverse leg is the same bearer with the same configuration, seen from the
        // other end -- including the flow key, which the peer binds as IT sees the flow
        // (addresses swapped, direction reversed).
        FlowId revFlow = flow.reversed();
        BearerRequest revReq = req;
        if (revReq.flowBindingKey.has_value())
            revReq.flowBindingKey = revReq.flowBindingKey->reversed();
        createOutgoingConnectionOnNode(destId, revFlow, revReq, destWithPdcp);
        createIncomingConnectionOnNode(sourceId, revFlow, revReq, sourceWithPdcp);
    }
    else
        createMulticastConnection(flow, req, withPdcp);
}

void ConnectionControlEnb::createMulticastConnection(const FlowId& flow, const BearerRequest& req, bool withPdcp)
{
    throw cRuntimeError("ConnectionControlEnb: base station %d has no D2D support to establish a multicast bearer for group %d",
            (int)num(nodeId_), (int)num(flow.d2dGroupId));
}

void ConnectionControlEnb::multicastGroupJoined(MacNodeId nodeId, MacNodeId groupId)
{
    Enter_Method_Silent("multicastGroupJoined");
    // every UE joins IP multicast groups (all-hosts, for one); without D2D no
    // sidelink multicast bearer exists that the joiner could need an RX leg of
    EV_INFO << "ConnectionControlEnb: node " << nodeId << " joined multicast group " << groupId << "; no sidelink multicast here" << endl;
}

void ConnectionControlEnb::createIncomingConnectionOnNode(MacNodeId nodeId, const FlowId& flow, const BearerRequest& req, bool withPdcp)
{
    if (nodeId == nodeId_)
        bearerManagement_->createIncomingConnection(flow, req, withPdcp);
    else
        controlOf(nodeId)->createIncomingConnection(flow, req, withPdcp);
}

void ConnectionControlEnb::createOutgoingConnectionOnNode(MacNodeId nodeId, const FlowId& flow, const BearerRequest& req, bool withPdcp)
{
    if (nodeId == nodeId_)
        bearerManagement_->createOutgoingConnection(flow, req, withPdcp);
    else
        controlOf(nodeId)->createOutgoingConnection(flow, req, withPdcp);
}

} //namespace
