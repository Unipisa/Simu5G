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

#include "simu5g/corenetwork/coreControl/CoreControl.h"

#include <algorithm>

#include <inet/common/ModuleAccess.h>

#include "simu5g/common/InitStages.h"
#include "simu5g/corenetwork/bearerConfigurator/BearerConfigurator.h"
#include "simu5g/corenetwork/userPlaneNodeControl/UserPlaneNodeControl.h"
#include "simu5g/stack/rrc/ConnectionControlBase.h"
#include "simu5g/stack/rrc/ConnectionControlEnb.h"

namespace simu5g {

using namespace omnetpp;
using namespace inet;

Define_Module(CoreControl);

void CoreControl::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        binder_.reference(this, "binderModule", true);
        bearerConfigurator_.reference(this, "bearerConfiguratorModule", true);
        binder_->subscribe(Binder::nodeUnregisteredSignal_, this);
    }
    else if (stage == INITSTAGE_SIMU5G_BINDER_ACCESS) {
        // After INITSTAGE_SIMU5G_NODE_RELATIONSHIPS, so the UEs' serving nodes are known
        takeGtpEndpoints();
        deliverQfiRules();
    }
}

void CoreControl::initialUeMessage(MacNodeId legId, ConnectionControlEnb *bs, SessionType sessionType)
{
    Enter_Method("initialUeMessage");

    // the sessions: once per UE, at its first leg's registration; the UE's legs request
    // the same type (the UE's sessionType parameter)
    auto it = sessionsOfNode_.find(legId);
    if (it == sessionsOfNode_.end()) {
        establishSessions(legId, sessionType);
        it = sessionsOfNode_.find(legId);
        if (it == sessionsOfNode_.end())
            return;   // the base station is not connected to a core network
    }
    std::vector<CoreSessionKey> keys = it->second;
    ASSERT(requestedTypes_.at(keys.front().first) == sessionType);

    // the RAN resources of each session: at every base station the UE attaches through
    // (the master of the leg's serving node), once
    MacNodeId bsId = binder_->getMasterNodeOrSelf(bs->getNodeId());
    for (const CoreSessionKey& key : keys) {
        CoreSession& session = sessions_.at(key);
        if (session.dlTunnels.count(bsId) == 0) {
            ConnectionControlEnb *ranBs = gtpEndpoints_.at(bsGtpEndpoints_.at(bsId)).bs;
            // the session's QoS rules; the one QoS flow of an Unstructured session needs none
            QfiRuleSet ulQfiRules = session.type != UNSTRUCTURED ? bearerConfigurator_->getUplinkQfiRules(session.ueModule, session.ref.id) : QfiRuleSet();
            FTeid dl = ranBs->sessionResourceSetup(legId, session.ref, session.type, getUplinkTunnels(session), std::move(ulQfiRules));
            registerRanTunnel(session, bsId, dl);
        }
        updateDownlinkPath(session);
    }
}

void CoreControl::receiveSignal(cComponent *source, simsignal_t signalID, long nodeId, cObject *details)
{
    Enter_Method_Silent("receiveSignal");
    MacNodeId id = MacNodeId(nodeId);
    ASSERT(signalID == Binder::nodeUnregisteredSignal_);
    releaseSessions(id);
}

void CoreControl::pathSwitchRequest(MacNodeId legId, ConnectionControlEnb *bs, const std::vector<SessionResource>& sessions)
{
    Enter_Method("pathSwitchRequest");
    auto it = sessionsOfNode_.find(legId);
    if (it == sessionsOfNode_.end())
        return;   // a UE without a session (its base stations have no core network)
    std::vector<CoreSessionKey> keys = it->second;
    MacNodeId bsId = binder_->getMasterNodeOrSelf(bs->getNodeId());

    // the tunnels the handover preparation set up at the base station: new to this
    // module unless the UE was attached through that base station before
    for (const SessionResource& resource : sessions) {
        auto key = std::find_if(keys.begin(), keys.end(), [&](const CoreSessionKey& k) { return k.second == resource.ref.id; });
        if (key == keys.end())
            throw cRuntimeError("CoreControl: base station %d switches session %d of %s, which the UE does not have",
                    (int)num(bsId), (int)num(resource.ref.id), sessions_.at(keys.front()).ueModule->getFullPath().c_str());
        CoreSession& session = sessions_.at(*key);
        if (resource.type != session.type)
            throw cRuntimeError("CoreControl: base station %d switches the session of %s as one of type \"%s\", but it is of type \"%s\"",
                    (int)num(bsId), session.ueModule->getFullPath().c_str(), sessionTypeToA(resource.type).c_str(), sessionTypeToA(session.type).c_str());
        auto known = session.dlTunnels.find(bsId);
        if (known == session.dlTunnels.end())
            registerRanTunnel(session, bsId, resource.dl);
        else if (known->second.teid != resource.dl.teid)
            throw cRuntimeError("CoreControl: base station %d reports a downlink TEID for the session of %s other than the one it ends it under",
                    (int)num(bsId), session.ueModule->getFullPath().c_str());
    }
    for (const CoreSessionKey& key : keys) {
        CoreSession& session = sessions_.at(key);
        setUpRanTunnels(session, bsId);   // a base station the preparation gave no tunnels
        updateDownlinkPath(session);
    }
}

void CoreControl::ueContextReleaseRequest(MacNodeId legId)
{
    Enter_Method("ueContextReleaseRequest");
    auto it = sessionsOfNode_.find(legId);
    if (it == sessionsOfNode_.end())
        return;
    for (const CoreSessionKey& key : std::vector<CoreSessionKey>(it->second))
        updateDownlinkPath(sessions_.at(key));
}

void CoreControl::takeGtpEndpoints()
{
    for (const auto& registration : binder_->getUserPlaneNodes()) {
        GtpEndpoint endpoint;
        endpoint.node = getContainingNode(registration.module);
        endpoint.userPlaneNode = registration.module;
        endpoint.type = registration.type;
        endpoint.gateway = registration.gateway;
        gtpEndpoints_.push_back(endpoint);
    }
    for (const auto& [nodeId, info] : binder_->getNodeInfoMap()) {
        if (getNodeTypeById(nodeId) != NODEB || info.moduleRef == nullptr)
            continue;
        auto *bs = dynamic_cast<ConnectionControlEnb *>(binder_->getRrcByNodeId(nodeId)->getSubmodule("connectionControl"));
        if (bs == nullptr)
            throw cRuntimeError("CoreControl: base station %s has no rrc.connectionControl (ConnectionControlEnb) module",
                    info.moduleRef->getFullPath().c_str());
        GtpEndpoint endpoint;
        endpoint.node = info.moduleRef;
        endpoint.bs = bs;
        endpoint.type = binder_->isNrNodeB(nodeId) ? GNB : ENB;
        endpoint.bsId = nodeId;
        endpoint.gateway = bs->getGateway();
        bsGtpEndpoints_[nodeId] = gtpEndpoints_.size();
        gtpEndpoints_.push_back(endpoint);
    }
}

void CoreControl::deliverQfiRules()
{
    // Downlink: each user plane node gets the rules scoped to it. (The uplink
    // rules reach each UE with its session's resource setup, see initialUeMessage().)
    for (const GtpEndpoint& endpoint : gtpEndpoints_)
        if (endpoint.userPlaneNode != nullptr)
            endpoint.userPlaneNode->setDownlinkClassifierRules(bearerConfigurator_->getDownlinkQfiRules(endpoint.node));
}

cModule *CoreControl::findGatewayNode(const std::string& gateway)
{
    // a gateway parameter names its node relative to the network
    std::string path = std::string(getSystemModule()->getFullPath()) + "." + gateway;
    return getSimulation()->findModuleByPath(path.c_str());
}

int CoreControl::findGatewayEndpoint(const std::string& gateway, const GtpEndpoint& from)
{
    cModule *node = findGatewayNode(gateway);
    for (int i = 0; i < (int)gtpEndpoints_.size(); i++) {
        const GtpEndpoint& endpoint = gtpEndpoints_[i];
        if ((endpoint.type == UPF || endpoint.type == PGW) && node != nullptr && endpoint.node == node)
            return i;
    }
    throw cRuntimeError("CoreControl: the gateway '%s' of %s is no user plane node (UPF or PGW)",
            gateway.c_str(), from.node->getFullPath().c_str());
}

MacNodeId CoreControl::findDlBaseStation(MacNodeId lteNodeId, MacNodeId nrNodeId)
{
    for (MacNodeId nodeId : {lteNodeId, nrNodeId}) {
        if (nodeId == NODEID_NONE)
            continue;
        MacNodeId servingNode = binder_->getServingNode(nodeId);
        if (servingNode != NODEID_NONE)
            return binder_->getMasterNodeOrSelf(servingNode);
    }
    return NODEID_NONE;
}

std::vector<CoreControl::SessionPlan> CoreControl::sessionsToEstablish(cModule *ueModule, SessionType requestedType)
{
    // one session, id 1 (the session BearerConfigurator::assignSession() gives every
    // definition), of the requested type, with every address of the UE
    SessionPlan plan;
    plan.id = SessionId(1);
    plan.type = requestedType;
    return {plan};
}

void CoreControl::establishSessions(MacNodeId ueNodeId, SessionType requestedType)
{
    cModule *ueModule = binder_->getNodeModule(ueNodeId);
    ASSERT(ueModule != nullptr);

    SessionRef ueRef;
    for (const auto& [nodeId, info] : binder_->getNodeInfoMap())
        if (info.moduleRef == ueModule)
            (isNrUe(nodeId) ? ueRef.nrNodeId : ueRef.lteNodeId) = nodeId;

    // The LTE id, which every UE has, is the UE's identity: once it is unregistered,
    // the UE is leaving the simulation, and its remaining stack's detachment must not
    // establish new sessions
    if (ueRef.lteNodeId == NODEID_NONE)
        return;

    MacNodeId dlBaseStation = findDlBaseStation(ueRef.lteNodeId, ueRef.nrNodeId);
    if (dlBaseStation == NODEID_NONE)
        return;   // established when the UE attaches
    const GtpEndpoint& bsEndpoint = gtpEndpoints_.at(bsGtpEndpoints_.at(dlBaseStation));
    if (bsEndpoint.gateway.empty()) {
        EV_INFO << "CoreControl: " << ueModule->getFullPath() << " is attached to base station " << dlBaseStation
                << ", which is not connected to a core network: no session" << endl;
        return;
    }

    // The anchor is the core network gateway of the base station the UE's downlink
    // enters the RAN at, and each session gets an uplink tunnel to each MEC host UPF of
    // that core network too, i.e. those whose gateway is the anchor
    int anchorIndex = findGatewayEndpoint(bsEndpoint.gateway, bsEndpoint);
    const GtpEndpoint& anchor = gtpEndpoints_[anchorIndex];

    std::vector<SessionPlan> plans = sessionsToEstablish(ueModule, requestedType);
    if (plans.empty())
        throw cRuntimeError("CoreControl: the policy establishes no session for %s", ueModule->getFullPath().c_str());
    std::set<SessionId> ids;
    for (const SessionPlan& plan : plans)
        if (num(plan.id) < 1 || num(plan.id) > 15 || !ids.insert(plan.id).second)
            throw cRuntimeError("CoreControl: the policy gives %s a session with id %d, which is not in 1..15 or not unique among the UE's sessions",
                    ueModule->getFullPath().c_str(), (int)num(plan.id));
    requestedTypes_[ueModule->getId()] = requestedType;

    for (const SessionPlan& plan : plans) {
        CoreSession session;
        session.ueModule = ueModule;
        session.ref = ueRef;
        session.ref.id = plan.id;
        session.type = plan.type;
        session.addresses = plan.addresses;
        session.anchor = anchorIndex;
        SessionType type = plan.type;

        // An EPC carries IP sessions only (the PDN types of TS 23.401); the other session
        // types are those of a 5G core
        if (anchor.type == PGW && !isIpSessionType(type))
            throw cRuntimeError("CoreControl: %s requests a session of type \"%s\", but its anchor %s is a PGW, whose EPC carries "
                    "IP sessions only (see the UE's sessionType parameter)",
                    ueModule->getFullPath().c_str(), sessionTypeToA(type).c_str(), anchor.node->getFullPath().c_str());

        // An Unstructured session (TS 23.501 5.6.10.3) is a 5G core's, and has one QoS flow,
        // the default one, whose QoS rule has no packet filter (5.7.1.4): the UE's stack
        // must have SDAP, and no QoS rule may be authored for the session. Its N6 address
        // is allocated here.
        // An Ethernet session (5.6.10.2) is a 5G core's too, and its QoS rules classify
        // frames, which have no DSCP field: a dscpAsQfi rule authored for the session is an error.
        if (!isIpSessionType(type) && !BearerConfigurator::ueStackHasSdap(ueModule))
            throw cRuntimeError("CoreControl: %s requests an %s session, but its stack has no SDAP (a 5G core's sessions need it, see the hasSdap parameter of the NIC)",
                    ueModule->getFullPath().c_str(), sessionTypeToA(type).c_str());
        if (type == ETHERNET && bearerConfigurator_->hasUplinkDscpAsQfiRuleScopedTo(ueModule, plan.id))
            throw cRuntimeError("CoreControl: %s requests an Ethernet session, whose frames have no DSCP field, but a dscpAsQfi rule of the ulQfiRules "
                    "parameter of the bearer configurator is scoped to it", ueModule->getFullPath().c_str());
        if (type == UNSTRUCTURED) {
            if (bearerConfigurator_->hasUplinkQfiRulesScopedTo(ueModule, plan.id))
                throw cRuntimeError("CoreControl: %s requests an Unstructured session, which has one QoS flow and no QoS rules, but the ulQfiRules "
                        "parameter of the bearer configurator has a rule scoped to it", ueModule->getFullPath().c_str());
            allocateN6Address(session);
        }

        // The session's uplink tunnels: at the anchor, and at each MEC host UPF of an IP
        // session (MEC steering goes by the IP destination, which a non-IP payload has none of)
        session.ulAnchor = anchor.userPlaneNode->establishUserPlaneSession(session.ref, session.type, session.n6Address);
        for (int i = 0; i < (int)gtpEndpoints_.size() && isIpSessionType(type); i++) {
            const GtpEndpoint& endpoint = gtpEndpoints_[i];
            if (endpoint.type == UPF_MEC && findGatewayNode(endpoint.gateway) == anchor.node)
                session.ulMecHosts[i] = endpoint.userPlaneNode->establishUserPlaneSession(session.ref, session.type, L3Address());
        }

        CoreSessionKey key(ueModule->getId(), session.ref.id);
        for (MacNodeId nodeId : {session.ref.lteNodeId, session.ref.nrNodeId})
            if (nodeId != NODEID_NONE)
                sessionsOfNode_[nodeId].push_back(key);
        CoreSession& established = sessions_[key] = session;
        EV_INFO << "CoreControl: session " << established.ref.id << " (" << sessionTypeToA(established.type) << ") of " << ueModule->getFullPath()
                << " established, anchored at " << anchor.node->getFullPath() << ", uplink F-TEID " << established.ulAnchor;
        for (const auto& [index, tunnel] : established.ulMecHosts)
            EV_INFO << ", to MEC host UPF " << tunnel;
        if (!established.n6Address.isUnspecified())
            EV_INFO << ", N6 address " << established.n6Address;
        for (const AddressPrefix& prefix : established.addresses)
            EV_INFO << ", UE addresses " << prefix;
        EV_INFO << endl;
    }
}

void CoreControl::updateDownlinkPath(CoreSession& session)
{
    MacNodeId dlBaseStation = findDlBaseStation(session.ref.lteNodeId, session.ref.nrNodeId);
    if (dlBaseStation == session.dlBaseStation)
        return;
    session.dlBaseStation = dlBaseStation;
    if (dlBaseStation == NODEID_NONE) {
        session.dl = FTeid();
        EV_INFO << "CoreControl: " << session.ueModule->getFullPath() << " is attached nowhere, the downlink of session "
                << session.ref.id << " has no tunnel" << endl;
    }
    else {
        session.dl = session.dlTunnels.at(dlBaseStation);
        EV_INFO << "CoreControl: the downlink of session " << session.ref.id << " of " << session.ueModule->getFullPath()
                << " enters the RAN at base station " << dlBaseStation << ", downlink F-TEID " << session.dl << endl;
    }

    // The anchor ends the downlink on the old path with an End Marker, which the old base
    // station relays to the new one after the downlink it forwards, and the new one holds
    // back the downlink of the new path until then (TS 23.502 4.9.1.2.2). A handover
    // passes through "attached nowhere", so the old path is the one last used. The MEC
    // host UPFs send none, and their downlink is not ordered against the forwarded one
    // (in 3GPP, a MEC branch sits behind the anchor's single N3 tunnel).
    MacNodeId oldDlBaseStation = session.lastDlBaseStation;
    FTeid oldDl;
    if (dlBaseStation != NODEID_NONE && oldDlBaseStation != NODEID_NONE && oldDlBaseStation != dlBaseStation)
        oldDl = session.dlTunnels.at(oldDlBaseStation);

    // the UPFs that send the session's downlink: the anchor, and the MEC host UPFs
    gtpEndpoints_[session.anchor].userPlaneNode->updateDownlinkTunnel(session.ref, session.dl, oldDl);
    for (const auto& [index, tunnel] : session.ulMecHosts)
        gtpEndpoints_[index].userPlaneNode->updateDownlinkTunnel(session.ref, session.dl, FTeid());
    if (dlBaseStation == NODEID_NONE)
        return;

    gtpEndpoints_.at(bsGtpEndpoints_.at(dlBaseStation)).bs->downlinkPathSwitched(session.ref.lteNodeId, session.ref.nrNodeId, oldDlBaseStation);
    session.lastDlBaseStation = dlBaseStation;
}

void CoreControl::setUpRanTunnels(CoreSession& session, MacNodeId bsId)
{
    if (session.dlTunnels.count(bsId) != 0)
        return;
    ConnectionControlEnb *bs = gtpEndpoints_.at(bsGtpEndpoints_.at(bsId)).bs;
    FTeid dl = bs->sessionTunnelSetup(session.ref, session.type, getUplinkTunnels(session));
    registerRanTunnel(session, bsId, dl);
}

void CoreControl::registerRanTunnel(CoreSession& session, MacNodeId bsId, const FTeid& dl)
{
    ConnectionControlEnb *bs = gtpEndpoints_.at(bsGtpEndpoints_.at(bsId)).bs;
    for (const auto& [otherBsId, otherDl] : session.dlTunnels) {
        gtpEndpoints_.at(bsGtpEndpoints_.at(otherBsId)).bs->setForwardingTeid(session.ref, bsId, dl.teid);
        bs->setForwardingTeid(session.ref, otherBsId, otherDl.teid);
    }
    session.dlTunnels[bsId] = dl;
}

UplinkTunnels CoreControl::getUplinkTunnels(const CoreSession& session)
{
    UplinkTunnels tunnels;
    tunnels.anchor = session.ulAnchor;
    tunnels.toUpf = gtpEndpoints_[session.anchor].type == UPF;
    for (const auto& [index, tunnel] : session.ulMecHosts)
        tunnels.mecHosts[tunnel.address] = tunnel.teid;
    return tunnels;
}

void CoreControl::allocateN6Address(CoreSession& session)
{
    auto [prefix, prefixLength] = gtpEndpoints_[session.anchor].userPlaneNode->getUnstructuredPrefix();
    bool ipv6 = prefix.getType() == L3Address::IPv6;
    int hostBits = (ipv6 ? 128 : 32) - prefixLength;
    uint64_t numAddresses = hostBits >= 32 ? (uint64_t(1) << 32) : (uint64_t(1) << hostBits);
    std::set<uint32_t>& inUse = n6AddressesInUse_[session.anchor];
    uint64_t suffix = 1;
    while (inUse.count(suffix))
        suffix++;
    // the prefix itself, and in IPv4 its broadcast address, name no session
    if (suffix >= numAddresses - (ipv6 ? 0 : 1))
        throw cRuntimeError("CoreControl: the unstructuredPrefix %s/%d of %s has no free N6 address left for the session of %s",
                prefix.str().c_str(), prefixLength, gtpEndpoints_[session.anchor].node->getFullPath().c_str(), session.ueModule->getFullPath().c_str());
    inUse.insert(suffix);
    session.n6Suffix = suffix;
    if (ipv6) {
        const uint32_t *words = prefix.toIpv6().words();
        session.n6Address = Ipv6Address(words[0], words[1], words[2], words[3] + (uint32_t)suffix);
    }
    else
        session.n6Address = Ipv4Address(prefix.toIpv4().getInt() + (uint32_t)suffix);
}

void CoreControl::freeN6Address(const CoreSession& session)
{
    if (session.n6Suffix != 0)
        n6AddressesInUse_[session.anchor].erase(session.n6Suffix);
}

void CoreControl::releaseSessions(MacNodeId ueNodeId)
{
    auto it = sessionsOfNode_.find(ueNodeId);
    if (it == sessionsOfNode_.end())
        return;
    std::vector<CoreSessionKey> keys = it->second;
    for (const CoreSessionKey& key : keys)
        releaseSession(sessions_.at(key));
    const CoreSession& any = sessions_.at(keys.front());
    for (MacNodeId nodeId : {any.ref.lteNodeId, any.ref.nrNodeId})
        sessionsOfNode_.erase(nodeId);
    requestedTypes_.erase(keys.front().first);
    for (const CoreSessionKey& key : keys)
        sessions_.erase(key);
}

void CoreControl::releaseSession(const CoreSession& session)
{
    EV_INFO << "CoreControl: session " << session.ref.id << " of " << session.ueModule->getFullPath() << " released" << endl;

    // the tunnel ends forget the session's tunnels
    gtpEndpoints_[session.anchor].userPlaneNode->releaseUserPlaneSession(session.ref);
    for (const auto& [index, tunnel] : session.ulMecHosts)
        gtpEndpoints_[index].userPlaneNode->releaseUserPlaneSession(session.ref);
    for (const auto& [bsId, dl] : session.dlTunnels)
        gtpEndpoints_.at(bsGtpEndpoints_.at(bsId)).bs->sessionRelease(session.ref);

    freeN6Address(session);
}

} //namespace
