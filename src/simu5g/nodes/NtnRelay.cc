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

#include "simu5g/nodes/NtnRelay.h"

#include <cstring>

#include <inet/common/ModuleAccess.h>

namespace simu5g {

using namespace omnetpp;

Define_Module(NtnRelay);

void NtnRelay::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        binder_.reference(this, "binderModule", true);
        relayDelay_ = par("relayDelay");
        cModule *node = inet::getContainingNode(this);
        nodeId_ = MacNodeId(node->par("macNodeId").intValue());
        nodeType_ = aToNodeType(par("nodeType").stdstringValue());
        node->getDisplayString().setTagArg("t", 0, opp_stringf("nodeId=%hu", num(nodeId_)).c_str());
    }
    else if (stage == INITSTAGE_SIMU5G_REGISTRATIONS) {
        cModule *node = inet::getContainingNode(this);
        if (nodeType_ == SATELLITE_NODE)
            binder_->registerSatelliteNode(nodeId_, node);
        else if (nodeType_ == NTN_GATEWAY_NODE)
            binder_->registerNtnGatewayNode(nodeId_, node);
        else
            throw cRuntimeError("NtnRelay::initialize - unsupported NTN relay node type %s", nodeTypeToA(nodeType_));
    }
}

void NtnRelay::handleMessage(cMessage *msg)
{
    const char *arrivalGate = msg->getArrivalGate()->getName();
    if (!strcmp(arrivalGate, "leftNicIn"))
        sendDelayed(msg, relayDelay_, "rightNicOut");
    else if (!strcmp(arrivalGate, "rightNicIn"))
        sendDelayed(msg, relayDelay_, "leftNicOut");
    else
        throw cRuntimeError("NtnRelay::handleMessage - unexpected gate %s", arrivalGate);
}

void NtnRelay::finish()
{
    if (nodeId_ == NODEID_NONE || !binder_->nodeExists(nodeId_))
        return;

    if (nodeType_ == SATELLITE_NODE)
        binder_->unregisterSatelliteNode(nodeId_);
    else if (nodeType_ == NTN_GATEWAY_NODE)
        binder_->unregisterNtnGatewayNode(nodeId_);
}

} // namespace simu5g
