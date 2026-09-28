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

#include <inet/common/ModuleAccess.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include "simu5g/corenetwork/gtp/GtpUser.h"
#include "simu5g/corenetwork/gtp/GtpUserX2.h"

namespace simu5g {

Define_Module(ConnectionControlEnb);

using namespace omnetpp;
using namespace inet;

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

FTeid ConnectionControlEnb::sessionTunnelSetup(const SessionRef& session, const UplinkTunnels& uplink)
{
    Enter_Method("sessionTunnelSetup");
    FTeid dl{getAddress(), allocateTeid()};
    gtpUser_->addTunnel(dl.teid, session);
    gtpUser_->setUplinkTunnels(session, uplink);
    // the same TEID receives the downlink a handover source forwards over X2-U
    gtpUserX2_->addTunnel(dl.teid, session);
    return dl;
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

} //namespace
