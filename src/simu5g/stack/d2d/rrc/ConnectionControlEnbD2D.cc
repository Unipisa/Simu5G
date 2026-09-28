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

namespace simu5g {

Define_Module(ConnectionControlEnbD2D);

using namespace omnetpp;

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
    return establishBearer(flow, BearerRequest{UM, Lcg(3), key});
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
