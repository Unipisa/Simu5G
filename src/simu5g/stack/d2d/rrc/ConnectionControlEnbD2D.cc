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

#include "simu5g/stack/d2d/rrc/ConnectionControlEnbD2D.h"

#include "simu5g/stack/d2d/binder/D2dBinder.h"
#include "simu5g/stack/d2d/mac/ID2dAmc.h"
#include "simu5g/stack/d2d/rrc/D2dModeSelectionBase.h"

namespace simu5g {

Define_Module(ConnectionControlEnbD2D);

using namespace omnetpp;

ConnectionControlEnbD2D::~ConnectionControlEnbD2D()
{
    for (auto& [msg, legId] : modeSwitchTimers_)
        cancelAndDelete(msg);
}

void ConnectionControlEnbD2D::initialize(int stage)
{
    ConnectionControlEnb::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL)
        d2dModeSelection_.reference(this, "d2dModeSelectionModule", false);
}

void ConnectionControlEnbD2D::handleMessage(cMessage *msg)
{
    auto it = modeSwitchTimers_.find(msg);
    if (it == modeSwitchTimers_.end())
        throw cRuntimeError("ConnectionControlEnbD2D: unknown message '%s'", msg->getName());
    MacNodeId legId = it->second;
    modeSwitchTimers_.erase(it);
    delete msg;
    // the handover has completed: can the leg's D2D flows switch (back) to direct mode?
    requestModeSwitch(legId, true);
}

void ConnectionControlEnbD2D::requestModeSwitch(MacNodeId legId, bool handoverCompleted)
{
    if (d2dModeSelection_ != nullptr)
        d2dModeSelection_->doModeSwitchAtHandover(legId, handoverCompleted);
    else
        EV_WARN << "ConnectionControlEnbD2D: base station " << nodeId_ << " has no D2D mode selection - no D2D mode switch" << endl;
}

void ConnectionControlEnbD2D::beforeHandoverCommand(MacNodeId legId, const UeContext& ctx)
{
    if (ctx.capabilities.d2d)
        requestModeSwitch(legId, false);
}

void ConnectionControlEnbD2D::legArrived(MacNodeId legId, const UeContext& ctx)
{
    if (!ctx.capabilities.d2d)
        return;
    LteAmc *amc = mac_->getAmc();
    if (dynamic_cast<ID2dAmc *>(amc) != nullptr)
        amc->attachUser(legId, D2D);
    else
        EV_WARN << "ConnectionControlEnbD2D: the AMC of base station " << nodeId_ << " is not D2D-capable - skipping D2D AMC attach" << endl;
    cMessage *msg = new cMessage("doModeSwitchAtHandover");
    msg->setSchedulingPriority(10);   // at the end of the instant
    modeSwitchTimers_[msg] = legId;
    scheduleAt(simTime(), msg);
}

void ConnectionControlEnbD2D::legLeft(MacNodeId legId, const UeContext& ctx)
{
    if (!ctx.capabilities.d2d)
        return;
    LteAmc *amc = mac_->getAmc();
    if (dynamic_cast<ID2dAmc *>(amc) != nullptr)
        amc->detachUser(legId, D2D);
    else
        EV_WARN << "ConnectionControlEnbD2D: the AMC of base station " << nodeId_ << " is not D2D-capable - skipping D2D AMC detach" << endl;
}

D2dBinder *ConnectionControlEnbD2D::d2dBinder()
{
    return D2dBinder::getInstance(this);
}

std::set<DrbId>& ConnectionControlEnbD2D::foreignPairPool(const std::pair<MacNodeId, MacNodeId>& pair)
{
    return d2dBinder()->sidelinkDrbIdPool(pair);
}

DrbId ConnectionControlEnbD2D::establishD2dBearer(const FlowId& flow, const FlowBindingKey& key)
{
    return establishDataConnection(flow, BearerRequest{UM, Lcg(3), key});
}

void ConnectionControlEnbD2D::createMulticastConnection(const FlowId& flow, const BearerRequest& req, bool withPdcp)
{
    MacNodeId sourceId = flow.sourceId;
    MacNodeId groupId = flow.d2dGroupId;

    // Remember the flow so that nodes joining this group later still get an RX leg; the
    // loop below can only reach the members that already exist. See multicastGroupJoined().
    d2dBinder()->rememberMulticastFlow(groupId, sourceId, flow, req, withPdcp);

    // Multicast bearers stay unidirectional: TX at the sender, RX at the members
    for (auto& [nodeId,_] : binder_->getNodeInfoMap())  //TODO use lte ones if LTE in DC setup, and NR ones if NR in DC setup
        if (nodeId != sourceId && binder_->isInMulticastGroup(nodeId, groupId))
            createIncomingConnectionOnNode(nodeId, flow, req, getNodeTypeById(nodeId)==UE || withPdcp);
}

void ConnectionControlEnbD2D::multicastGroupJoined(MacNodeId nodeId, MacNodeId groupId)
{
    Enter_Method("multicastGroupJoined(%hu, %hu)", (unsigned short)nodeId, (unsigned short)groupId);

    for (auto& [key, mf] : d2dBinder()->getMulticastFlows()) {
        auto& [flowGroupId, senderId] = key;
        if (flowGroupId != groupId || senderId == nodeId)
            continue;
        createIncomingConnectionOnNode(nodeId, mf.flow, mf.req,
                getNodeTypeById(nodeId) == UE || mf.withPdcp);
    }
}

} //namespace
