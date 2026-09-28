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

#include <inet/common/ModuleAccess.h>

#include "simu5g/common/InitStages.h"
#include "simu5g/corenetwork/bearerConfigurator/BearerConfigurator.h"
#include "simu5g/corenetwork/userPlaneNodeControl/UserPlaneNodeControl.h"
#include "simu5g/stack/ip2nic/HandoverPacketHolderEnb.h"
#include "simu5g/stack/rrc/BearerManagement.h"
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
        binder_->subscribe(Binder::servingNodeChangedSignal_, this);
    }
    else if (stage == INITSTAGE_SIMU5G_BINDER_ACCESS) {
        // After INITSTAGE_SIMU5G_NODE_RELATIONSHIPS, so the UEs' serving nodes are known
        takeGtpEndpoints();
        deliverQfiRules();
    }
    else if (stage == inet::INITSTAGE_LAST) {
        establishPduSessions();
    }
}

void CoreControl::receiveSignal(cComponent *source, simsignal_t signalID, long nodeId, cObject *details)
{
    Enter_Method_Silent("receiveSignal");
    MacNodeId id = MacNodeId(nodeId);

    if (signalID == Binder::servingNodeChangedSignal_) {
        // during initialization, attachments are still being settled; the sessions of
        // the UEs attached by the end of it are established in the last stage
        if (!pduSessionsEstablished_)
            return;
        auto it = sessionOfNode_.find(id);
        if (it != sessionOfNode_.end())
            switchPath(sessions_.at(it->second));
        else if (binder_->nodeExists(id))
            establishSession(id);
        return;
    }

    ASSERT(signalID == Binder::nodeUnregisteredSignal_);
    releaseSession(id);
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
    // Downlink: each user plane node gets the rules scoped to it, over N4
    for (const GtpEndpoint& endpoint : gtpEndpoints_)
        if (endpoint.userPlaneNode != nullptr)
            endpoint.userPlaneNode->setDownlinkClassifierRules(bearerConfigurator_->getDownlinkQfiRules(endpoint.node));

    // Uplink: each SDAP UE's classifier gets the rules scoped to it, through its RRC
    std::map<cModule *, std::vector<MacNodeId>> ueNodeIds;
    for (const auto& [nodeId, info] : binder_->getNodeInfoMap())
        if (getNodeTypeById(nodeId) == UE && info.moduleRef != nullptr)
            ueNodeIds[info.moduleRef].push_back(nodeId);
    for (const auto& [ueModule, nodeIds] : ueNodeIds) {
        if (!BearerConfigurator::ueStackHasSdap(ueModule))
            continue;   // no SDAP, no uplink QoS-flow classification
        auto *ueRrc = check_and_cast<BearerManagement *>(binder_->getRrcByNodeId(nodeIds.front())->getSubmodule("bearerManagement"));
        ueRrc->setUplinkQfiRules(bearerConfigurator_->getUplinkQfiRules(ueModule));
    }
}

void CoreControl::prepareHandover(MacNodeId ueNodeId, MacNodeId targetNodeId)
{
    Enter_Method_Silent("prepareHandover");
    auto it = sessionOfNode_.find(ueNodeId);
    if (it == sessionOfNode_.end())
        return;   // a UE without a PDU session has no downlink to forward
    CoreSession& session = sessions_.at(it->second);
    setUpRanTunnels(session, binder_->getMasterNodeOrSelf(targetNodeId));
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
    throw cRuntimeError("CoreControl: the gateway '%s' of %s is no UPF or PGW with an N4 endpoint",
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

void CoreControl::establishPduSessions()
{
    // in node id order, so the TEIDs are allocated in a reproducible order
    for (const auto& [nodeId, info] : binder_->getNodeInfoMap())
        if (getNodeTypeById(nodeId) == UE && info.moduleRef != nullptr && sessionOfNode_.count(nodeId) == 0)
            establishSession(nodeId);
    pduSessionsEstablished_ = true;
}

void CoreControl::establishSession(MacNodeId ueNodeId)
{
    cModule *ueModule = binder_->getNodeModule(ueNodeId);
    ASSERT(ueModule != nullptr);

    CoreSession session;
    session.ueModule = ueModule;
    session.ref.id = SessionId(1);
    for (const auto& [nodeId, info] : binder_->getNodeInfoMap())
        if (info.moduleRef == ueModule)
            (isNrUe(nodeId) ? session.ref.nrNodeId : session.ref.lteNodeId) = nodeId;

    // The LTE id, which every UE has, is the UE's identity: once it is unregistered,
    // the UE is leaving the simulation, and its remaining stack's detachment must not
    // establish a new session
    if (session.ref.lteNodeId == NODEID_NONE)
        return;

    MacNodeId dlBaseStation = findDlBaseStation(session.ref.lteNodeId, session.ref.nrNodeId);
    if (dlBaseStation == NODEID_NONE)
        return;   // established when the UE attaches
    const GtpEndpoint& bsEndpoint = gtpEndpoints_.at(bsGtpEndpoints_.at(dlBaseStation));
    if (bsEndpoint.gateway.empty()) {
        EV_INFO << "CoreControl: " << ueModule->getFullPath() << " is attached to base station " << dlBaseStation
                << ", which is not connected to a core network: no PDU session" << endl;
        return;
    }

    // The anchor is the core network gateway of the base station the UE's downlink
    // enters the RAN at, and the session gets an uplink tunnel to each MEC host UPF of
    // that core network too, i.e. those whose gateway is the anchor
    session.anchor = findGatewayEndpoint(bsEndpoint.gateway, bsEndpoint);
    const GtpEndpoint& anchor = gtpEndpoints_[session.anchor];
    session.ulAnchor = anchor.userPlaneNode->establishUserPlaneSession(session.ref);
    for (int i = 0; i < (int)gtpEndpoints_.size(); i++) {
        const GtpEndpoint& endpoint = gtpEndpoints_[i];
        if (endpoint.type == UPF_MEC && findGatewayNode(endpoint.gateway) == anchor.node)
            session.ulMecHosts[i] = endpoint.userPlaneNode->establishUserPlaneSession(session.ref);
    }

    CoreSessionKey key(ueModule->getId(), session.ref.id);
    for (MacNodeId nodeId : {session.ref.lteNodeId, session.ref.nrNodeId})
        if (nodeId != NODEID_NONE)
            sessionOfNode_[nodeId] = key;
    CoreSession& established = sessions_[key] = session;
    EV_INFO << "CoreControl: PDU session " << established.ref.id << " of " << ueModule->getFullPath()
            << " established, anchored at " << anchor.node->getFullPath() << ", uplink F-TEID " << established.ulAnchor;
    for (const auto& [index, tunnel] : established.ulMecHosts)
        EV_INFO << ", to MEC host UPF " << tunnel;
    EV_INFO << endl;
    switchPath(established);
}

void CoreControl::switchPath(CoreSession& session)
{
    for (MacNodeId nodeId : {session.ref.lteNodeId, session.ref.nrNodeId}) {
        if (nodeId == NODEID_NONE)
            continue;
        MacNodeId servingNode = binder_->getServingNode(nodeId);
        if (servingNode != NODEID_NONE)
            setUpRanTunnels(session, binder_->getMasterNodeOrSelf(servingNode));
    }

    MacNodeId dlBaseStation = findDlBaseStation(session.ref.lteNodeId, session.ref.nrNodeId);
    if (dlBaseStation == session.dlBaseStation)
        return;
    session.dlBaseStation = dlBaseStation;
    if (dlBaseStation == NODEID_NONE) {
        session.dl = FTeid();
        EV_INFO << "CoreControl: " << session.ueModule->getFullPath() << " is attached nowhere, the downlink of PDU session "
                << session.ref.id << " has no tunnel" << endl;
    }
    else {
        session.dl = session.dlTunnels.at(dlBaseStation);
        EV_INFO << "CoreControl: the downlink of PDU session " << session.ref.id << " of " << session.ueModule->getFullPath()
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

    auto holder = check_and_cast<HandoverPacketHolderEnb *>(binder_->getHandoverPacketHolderByNodeId(dlBaseStation));
    holder->switchDownlinkPath(session.ref.lteNodeId, session.ref.nrNodeId, oldDlBaseStation);
    session.lastDlBaseStation = dlBaseStation;
}

void CoreControl::setUpRanTunnels(CoreSession& session, MacNodeId bsId)
{
    if (session.dlTunnels.count(bsId) != 0)
        return;
    ConnectionControlEnb *bs = gtpEndpoints_.at(bsGtpEndpoints_.at(bsId)).bs;
    FTeid dl = bs->sessionTunnelSetup(session.ref, getUplinkTunnels(session));

    // the base stations of the session learn each other's downlink TEIDs, to forward
    // the downlink over X2 with
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

void CoreControl::releaseSession(MacNodeId ueNodeId)
{
    auto it = sessionOfNode_.find(ueNodeId);
    if (it == sessionOfNode_.end())
        return;
    CoreSessionKey key = it->second;
    const CoreSession& session = sessions_.at(key);
    EV_INFO << "CoreControl: PDU session " << session.ref.id << " of " << session.ueModule->getFullPath() << " released" << endl;

    // the tunnel ends forget the session's tunnels
    gtpEndpoints_[session.anchor].userPlaneNode->releaseUserPlaneSession(session.ref);
    for (const auto& [index, tunnel] : session.ulMecHosts)
        gtpEndpoints_[index].userPlaneNode->releaseUserPlaneSession(session.ref);
    for (const auto& [bsId, dl] : session.dlTunnels)
        gtpEndpoints_.at(bsGtpEndpoints_.at(bsId)).bs->sessionRelease(session.ref);

    for (MacNodeId nodeId : {session.ref.lteNodeId, session.ref.nrNodeId})
        sessionOfNode_.erase(nodeId);
    sessions_.erase(key);
}

} //namespace
