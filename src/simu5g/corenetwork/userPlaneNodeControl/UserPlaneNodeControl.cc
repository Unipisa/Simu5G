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

#include "simu5g/corenetwork/gtp/GtpUser.h"
#include "simu5g/corenetwork/trafficFlowFilter/TrafficFlowFilter.h"

namespace simu5g {

Define_Module(UserPlaneNodeControl);

using namespace omnetpp;
using namespace inet;

// the nodeType parameter: the user plane node types only (a base station has no N4 endpoint)
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

void UserPlaneNodeControl::setDownlinkClassifierRules(QfiRuleSet&& rules)
{
    Enter_Method("setDownlinkClassifierRules");
    trafficFlowFilter_->setQfiRules(std::move(rules));
}

FTeid UserPlaneNodeControl::establishUserPlaneSession(const SessionRef& session)
{
    Enter_Method("establishUserPlaneSession");
    FTeid tunnel{getAddress(), allocateTeid()};
    gtpUser_->addTunnel(tunnel.teid, session);
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
}

} //namespace
