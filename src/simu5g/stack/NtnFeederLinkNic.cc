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

#include "simu5g/stack/NtnFeederLinkNic.h"

#include <inet/common/packet/Packet.h>

#include "simu5g/stack/phy/packet/AirFrame_m.h"

namespace simu5g {

using namespace omnetpp;
using namespace inet;

Define_Module(NtnFeederLinkNic);

void NtnFeederLinkNic::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        binder_.reference(this, "binderModule", true);
        cModule *node = getParentModule();
        nodeId_ = MacNodeId(node->par("macNodeId").intValue());
        nodeType_ = aToNodeType(node->par("nodeType").stdstringValue());
    }
}

void NtnFeederLinkNic::handleMessage(cMessage *msg)
{
    cGate *arrivalGate = msg->getArrivalGate();
    if (arrivalGate->isName("upperLayerIn")) {
        auto *pkt = check_and_cast<Packet *>(msg);
        auto *frame = check_and_cast<AirFrame *>(pkt->decapsulate());
        delete pkt;

        cGate *peerGate = resolvePeerGate();
        sendDirect(frame, 0, frame->getDuration(), peerGate);
        return;
    }

    if (arrivalGate->isName("radioIn")) {
        auto *frame = check_and_cast<AirFrame *>(msg);
        auto *pkt = new Packet(frame->getName());
        pkt->encapsulate(frame);
        send(pkt, "upperLayerOut");
        return;
    }

    if (arrivalGate->isName("phys"))
        throw cRuntimeError("NtnFeederLinkNic::handleMessage - feeder link uses sendDirect(), unexpected packet on phys gate");

    throw cRuntimeError("NtnFeederLinkNic::handleMessage - unexpected gate %s", arrivalGate->getName());
}

cModule *NtnFeederLinkNic::resolvePeerNode() const
{
    if (nodeType_ == NTN_GATEWAY_NODE) {
        MacNodeId satelliteId = binder_->getAssociatedSatelliteForGateway(nodeId_);
        SatelliteInfo *info = binder_->getSatelliteInfo(satelliteId);
        if (info == nullptr || info->satelliteModule == nullptr)
            throw cRuntimeError("NtnFeederLinkNic::resolvePeerNode - satellite %hu is not registered", num(satelliteId));
        return info->satelliteModule.get();
    }

    if (nodeType_ == SATELLITE_NODE) {
        MacNodeId gatewayId = binder_->getAssociatedGatewayForSatellite(nodeId_);
        NtnGatewayInfo *info = binder_->getNtnGatewayInfo(gatewayId);
        if (info == nullptr || info->gatewayModule == nullptr)
            throw cRuntimeError("NtnFeederLinkNic::resolvePeerNode - NTN gateway %hu is not registered", num(gatewayId));
        return info->gatewayModule.get();
    }

    throw cRuntimeError("NtnFeederLinkNic::resolvePeerNode - unsupported node type %s", nodeTypeToA(nodeType_));
}

cGate *NtnFeederLinkNic::resolvePeerGate() const
{
    cModule *peerNode = resolvePeerNode();
    cModule *peerNic = peerNode->getSubmodule("feederNic");
    if (peerNic == nullptr)
        throw cRuntimeError("NtnFeederLinkNic::resolvePeerGate - node %s has no feederNic submodule", peerNode->getFullPath().c_str());

    cGate *peerGate = peerNic->gate("radioIn");
    if (peerGate == nullptr)
        throw cRuntimeError("NtnFeederLinkNic::resolvePeerGate - feederNic of %s has no radioIn gate", peerNode->getFullPath().c_str());
    return peerGate;
}

} // namespace simu5g
