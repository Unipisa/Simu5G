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

#include "simu5g/apps/unstructured/UnstructuredIo.h"

#include <inet/common/IProtocolRegistrationListener.h>
#include <inet/common/ModuleAccess.h>
#include <inet/common/ProtocolTag_m.h>
#include <inet/common/Simsignals.h>
#include <inet/linklayer/common/InterfaceTag_m.h>
#include <inet/networklayer/common/NetworkInterface.h>
#include <inet/networklayer/contract/IInterfaceTable.h>

#include "simu5g/common/LteCommon.h"

namespace simu5g {

Define_Module(UnstructuredIo);

using namespace omnetpp;
using namespace inet;

void UnstructuredIo::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        WATCH(numSent_);
        WATCH(numReceived_);
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        // the payload the NIC delivers up as the unstructured protocol comes here
        registerProtocol(LteProtocol::unstructured, gate("socketOut"), gate("socketIn"));
    }
}

int UnstructuredIo::getInterfaceId()
{
    if (interfaceId_ == -1) {
        auto *interfaceTable = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        NetworkInterface *networkInterface = interfaceTable->findInterfaceByName(par("interface").stringValue());
        if (networkInterface == nullptr)
            throw cRuntimeError("UnstructuredIo: the node has no network interface named '%s'", par("interface").stringValue());
        interfaceId_ = networkInterface->getInterfaceId();
    }
    return interfaceId_;
}

void UnstructuredIo::handleMessage(cMessage *msg)
{
    auto packet = check_and_cast<Packet *>(msg);
    if (msg->arrivedOn("trafficIn")) {
        // down to the cellular NIC, as the payload of the UE's Unstructured session
        packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&LteProtocol::unstructured);
        packet->addTagIfAbsent<InterfaceReq>()->setInterfaceId(getInterfaceId());
        numSent_++;
        emit(packetSentSignal, packet);
        send(packet, "socketOut");
    }
    else if (msg->arrivedOn("socketIn")) {
        EV_INFO << "UnstructuredIo: received " << packet << endl;
        numReceived_++;
        emit(packetReceivedSignal, packet);
        packet->removeTagIfPresent<DispatchProtocolReq>();
        send(packet, "trafficOut");
    }
    else
        throw cRuntimeError("UnstructuredIo: message arrived on unexpected gate %s", msg->getArrivalGate()->getFullName());
}

void UnstructuredIo::finish()
{
    recordScalar("packets sent", numSent_);
    recordScalar("packets received", numReceived_);
}

} //namespace
