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

#include "simu5g/corenetwork/userPlaneNodeControl/UserPlaneNodeControl.h"

#include <cstring>

#include <inet/common/ModuleAccess.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include "simu5g/corenetwork/ethernetSessionBridge/EthernetSessionMux.h"
#include "simu5g/corenetwork/gtp/GtpUser.h"
#include "simu5g/corenetwork/trafficFlowFilter/TrafficFlowFilter.h"

namespace simu5g {

Define_Module(UserPlaneNodeControl);

using namespace omnetpp;
using namespace inet;

// the nodeType parameter: the user plane node types only (a base station is no user plane node)
static CoreNodeType parseNodeType(const char *type)
{
    if (strcmp(type, "UPF") == 0)
        return UPF;
    if (strcmp(type, "PGW") == 0)
        return PGW;
    if (strcmp(type, "UPF_MEC") == 0)
        return UPF_MEC;
    throw cRuntimeError("UserPlaneNodeControl: unknown node type '%s' (expected UPF, PGW or UPF_MEC)", type);
}

void UserPlaneNodeControl::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        nodeType_ = parseNodeType(par("nodeType"));
        gateway_ = par("gateway").stdstringValue();
        if (nodeType_ == UPF_MEC && gateway_.empty())
            throw cRuntimeError("The required 'gateway' parameter is empty");

        binder_.reference(this, "binderModule", true);
        gtpUser_.reference(this, "gtpUserModule", true);
        trafficFlowFilter_.reference(this, "trafficFlowFilterModule", true);
        ethernetBridge_.reference(this, "ethernetBridgeModule", false);

        binder_->registerUserPlaneNode(this, nodeType_, gateway_);
    }
}

Teid UserPlaneNodeControl::allocateTeid()
{
    if (lastTeid_ == Teid(UINT32_MAX))
        throw cRuntimeError("UserPlaneNodeControl: the TEID space of %s is exhausted", getContainingNode(this)->getFullPath().c_str());
    lastTeid_ = Teid(num(lastTeid_) + 1);
    return lastTeid_;
}

const L3Address& UserPlaneNodeControl::getAddress()
{
    if (address_.isUnspecified())
        address_ = L3AddressResolver().resolve(getContainingNode(this)->getFullPath().c_str());
    return address_;
}

void UserPlaneNodeControl::configureN6()
{
    if (n6Configured_)
        return;
    std::string node = getContainingNode(this)->getFullPath();
    const char *where = node.c_str();
    if (nodeType_ != UPF)
        throw cRuntimeError("UserPlaneNodeControl: %s anchors an Unstructured session, but only a UPF can", where);

    // the prefix, e.g. "2001:db8:5::/64" or "10.5.0.0/16"
    std::string prefix = par("unstructuredPrefix").stdstringValue();
    if (prefix.empty())
        throw cRuntimeError("UserPlaneNodeControl: %s anchors an Unstructured session, but its unstructuredPrefix parameter is empty", where);
    std::vector<std::string> parts = cStringTokenizer(prefix.c_str(), "/").asVector();
    if (parts.size() != 2 || !unstructuredPrefix_.tryParse(parts[0].c_str()) || unstructuredPrefix_.getType() == L3Address::NONE)
        throw cRuntimeError("UserPlaneNodeControl: invalid unstructuredPrefix '%s', must be an address and a prefix length, e.g. \"2001:db8:5::/64\"", prefix.c_str());
    bool ipv6 = unstructuredPrefix_.getType() == L3Address::IPv6;
    unstructuredPrefixLength_ = atoi(parts[1].c_str());
    int maxLength = ipv6 ? 96 : 30;   // at least two host bits; an IPv6 session address differs from the prefix in its last 32 bits
    if (unstructuredPrefixLength_ < 1 || unstructuredPrefixLength_ > maxLength)
        throw cRuntimeError("UserPlaneNodeControl: invalid prefix length in unstructuredPrefix '%s', must be 1..%d", prefix.c_str(), maxLength);
    if (unstructuredPrefix_.getPrefix(unstructuredPrefixLength_) != unstructuredPrefix_)
        throw cRuntimeError("UserPlaneNodeControl: unstructuredPrefix '%s' has host bits set", prefix.c_str());

    // the server, of the prefix's family
    std::string server = par("unstructuredServer").stdstringValue();
    if (server.empty())
        throw cRuntimeError("UserPlaneNodeControl: %s anchors an Unstructured session, but its unstructuredServer parameter is empty", where);
    unstructuredServer_ = L3AddressResolver().resolve((binder_->getNetworkName() + "." + server).c_str(), ipv6 ? L3AddressResolver::ADDR_IPv6 : L3AddressResolver::ADDR_IPv4);

    unstructuredServerPort_ = par("unstructuredServerPort");
    unstructuredPort_ = par("unstructuredPort");
    for (int port : {unstructuredServerPort_, unstructuredPort_})
        if (port < 0 || port > 65535)
            throw cRuntimeError("UserPlaneNodeControl: %s anchors an Unstructured session, but its unstructuredServerPort and unstructuredPort parameters "
                    "are not both set to UDP port numbers (TS 29.561 9.2: configured per data network, aligned with the server)", where);
    n6Configured_ = true;
}

std::pair<L3Address, int> UserPlaneNodeControl::getUnstructuredPrefix()
{
    Enter_Method("getUnstructuredPrefix");
    configureN6();
    return {unstructuredPrefix_, unstructuredPrefixLength_};
}

void UserPlaneNodeControl::setDownlinkClassifierRules(QfiRuleSet&& rules)
{
    Enter_Method("setDownlinkClassifierRules");
    trafficFlowFilter_->setQfiRules(std::move(rules));
}

FTeid UserPlaneNodeControl::establishUserPlaneSession(const SessionRef& session, SessionType type, const L3Address& n6Address)
{
    Enter_Method("establishUserPlaneSession");
    FTeid tunnel{getAddress(), allocateTeid()};
    gtpUser_->addTunnel(tunnel.teid, session, type);

    // an Unstructured session's N6 tunnel: the uplink leaves on it, the downlink is
    // recognized by its address
    if (!n6Address.isUnspecified()) {
        ASSERT(type == UNSTRUCTURED);
        configureN6();
        N6Tunnel n6Tunnel{n6Address, unstructuredPort_, unstructuredServer_, unstructuredServerPort_};
        gtpUser_->setN6Tunnel(session, n6Tunnel);
        trafficFlowFilter_->addN6Tunnel(session, n6Tunnel);
    }

    // an Ethernet session's port in the node's bridge
    if (type == ETHERNET) {
        if (ethernetBridge_.getNullable() == nullptr)
            throw cRuntimeError("UserPlaneNodeControl: %s anchors an Ethernet session, but has no Ethernet session bridge (see its hasEthernetBridge parameter)",
                    getContainingNode(this)->getFullPath().c_str());
        ethernetBridge_->addSessionPort(session);
    }
    return tunnel;
}

void UserPlaneNodeControl::updateDownlinkTunnel(const SessionRef& session, const FTeid& dl, const FTeid& oldDl)
{
    Enter_Method("updateDownlinkTunnel");
    gtpUser_->setDownlinkTunnel(session, dl);
    if (oldDl.isSet())
        gtpUser_->sendEndMarker(oldDl);
}

void UserPlaneNodeControl::releaseUserPlaneSession(const SessionRef& session)
{
    Enter_Method("releaseUserPlaneSession");
    gtpUser_->removeSession(session);
    trafficFlowFilter_->removeN6Tunnel(session);
    if (ethernetBridge_.getNullable() != nullptr)
        ethernetBridge_->removeSessionPort(session);
}

} //namespace
