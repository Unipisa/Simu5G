//
//                  Simu5G
//
// Copyright (C) 2026 Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/ethernet/EthernetPortal.h"

#include <inet/common/ModuleAccess.h>
#include <inet/common/ProtocolTag_m.h>
#include <inet/linklayer/common/InterfaceTag_m.h>

namespace simu5g {

using namespace omnetpp;
using namespace inet;

Define_Module(EthernetPortal);

void EthernetPortal::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL)
        networkInterface_ = getContainingNicModule(this);
}

void EthernetPortal::pushPacket(Packet *packet, const cGate *gate)
{
    Enter_Method("pushPacket");
    take(packet);
    // a frame of the UE's link layer, into the NIC's stack
    send(packet, "lowerLayerOut");
}

void EthernetPortal::handleMessage(cMessage *msg)
{
    auto frame = check_and_cast<Packet *>(msg);
    if (msg->arrivedOn("upperLayerIn"))
        send(frame, "lowerLayerOut");
    else if (msg->arrivedOn("lowerLayerIn")) {
        // a frame of the session, up to the link layer as an Ethernet interface delivers it
        frame->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ethernetMac);
        frame->addTagIfAbsent<InterfaceInd>()->setInterfaceId(networkInterface_->getInterfaceId());
        auto dispatch = frame->addTagIfAbsent<DispatchProtocolReq>();
        dispatch->setProtocol(&Protocol::ethernetMac);
        dispatch->setServicePrimitive(SP_INDICATION);
        send(frame, "upperLayerOut");
    }
    else
        throw cRuntimeError("EthernetPortal: message arrived on unexpected gate %s", msg->getArrivalGate()->getFullName());
}

} //namespace
