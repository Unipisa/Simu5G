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

#include "simu5g/corenetwork/ethernetSessionBridge/EthernetSessionMux.h"

#include <inet/common/IProtocolRegistrationListener.h>
#include <inet/common/ModuleAccess.h>
#include <inet/common/ProtocolTag_m.h>
#include <inet/linklayer/common/InterfaceTag_m.h>
#include <inet/networklayer/ipv4/Ipv4InterfaceData.h>
#include <inet/networklayer/ipv6/Ipv6InterfaceData.h>

#include "simu5g/common/SessionTag_m.h"

namespace simu5g {

using namespace omnetpp;
using namespace inet;

Define_Module(EthernetSessionPortal);
Define_Module(EthernetSessionMux);

void EthernetSessionPortal::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        // an Ethernet port of the bridge: its own MAC address, broadcast; not
        // multicast, which keeps the node's IGMP off it (see EthernetSessionMux::addSessionPort())
        networkInterface_ = getContainingNicModule(this);
        networkInterface_->setProtocol(&Protocol::ethernetMac);
        networkInterface_->setMacAddress(MacAddress::generateAutoAddress());
        networkInterface_->setBroadcast(true);
        networkInterface_->setMulticast(false);
        networkInterface_->setPointToPoint(false);
        networkInterface_->setMtu(1500);
    }
}

void EthernetSessionPortal::pushPacket(Packet *packet, const cGate *gate)
{
    Enter_Method("pushPacket");
    take(packet);
    // down from the bridge: the frame goes into the session
    send(packet, "lowerLayerOut");
}

void EthernetSessionPortal::handleMessage(cMessage *msg)
{
    auto frame = check_and_cast<Packet *>(msg);
    if (!msg->arrivedOn("lowerLayerIn"))
        throw cRuntimeError("EthernetSessionPortal: message arrived on unexpected gate %s", msg->getArrivalGate()->getFullName());
    // up from the session: the frame reaches the bridge as an Ethernet interface delivers it
    frame->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ethernetMac);
    frame->addTagIfAbsent<InterfaceInd>()->setInterfaceId(networkInterface_->getInterfaceId());
    auto dispatch = frame->addTagIfAbsent<DispatchProtocolReq>();
    dispatch->setProtocol(&Protocol::ethernetMac);
    dispatch->setServicePrimitive(SP_INDICATION);
    send(frame, "upperLayerOut");
}

void EthernetSessionMux::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        interfaceTable_.reference(this, "interfaceTableModule", true);
        macTable_.reference(this, "macTableModule", true);
        // the uplink frames of the sessions reach the bridge as the ethernetmac service
        registerService(Protocol::ethernetMac, gate("sessionIn"), SP_REQUEST);
        WATCH_MAP(portOfGate_);
    }
}

void EthernetSessionMux::addSessionPort(const SessionRef& session)
{
    Enter_Method("addSessionPort");
    if (ports_.count(session.lteNodeId) != 0)
        throw cRuntimeError("EthernetSessionMux: %s has a port already", (std::ostringstream() << session).str().c_str());

    // created at run time the way Ipv6RoutingTable::createTunnelNetworkInterface()
    // creates a tunnel interface, and wired into the bridge's li dispatcher and to a gate
    // pair of this module
    cModule *bridge = getParentModule();
    cModuleType *type = cModuleType::get(par("portModuleType").stringValue());
    std::string name = "sessionPort" + std::to_string(portCounter_++);
    cModule *module = type->create(name.c_str(), bridge);
    module->par("interfaceTableModule") = check_and_cast<cModule *>(interfaceTable_.get())->getFullPath().c_str();
    module->finalizeParameters();
    module->buildInside();
    module->getDisplayString().updateWith(par("portDisplayString").stringValue());
    cModule *li = bridge->getSubmodule("li");
    li->getOrCreateFirstUnconnectedGate("out", 0, false, true)->connectTo(module->gate("upperLayerIn"));
    module->gate("upperLayerOut")->connectTo(li->getOrCreateFirstUnconnectedGate("in", 0, false, true));
    cGate *portOut = getOrCreateFirstUnconnectedGate("portOut", 0, false, true);
    if (gateSize("portIn") <= portOut->getIndex())
        setGateSize("portIn", portOut->getIndex() + 1);
    cGate *portIn = gate("portIn", portOut->getIndex());   // the pair shares its index
    portOut->connectTo(module->gate("lowerLayerIn"));
    module->gate("lowerLayerOut")->connectTo(portIn);
    module->callInitialize();
    auto *networkInterface = check_and_cast<NetworkInterface *>(module);

    // The node's IP configurators prepare every interface created in the node, also one
    // of this bridge's own interface table (INET: they subscribe to
    // interfaceCreatedSignal on the node); a bridge port has no IP configuration
    delete networkInterface->removeProtocolDataIfPresent<Ipv4InterfaceData>();
    delete networkInterface->removeProtocolDataIfPresent<Ipv6InterfaceData>();

    ports_[session.lteNodeId] = Port{TunnelSession{session, ETHERNET}, networkInterface, portOut->getIndex()};
    portOfGate_[portOut->getIndex()] = session.lteNodeId;
    EV_INFO << "EthernetSessionMux: " << session << " has port " << networkInterface->getInterfaceName()
            << " (" << networkInterface->getMacAddress() << ")" << endl;
}

void EthernetSessionMux::removeSessionPort(const SessionRef& session)
{
    Enter_Method("removeSessionPort");
    auto it = ports_.find(session.lteNodeId);
    if (it == ports_.end())
        return;
    NetworkInterface *networkInterface = it->second.networkInterface;
    EV_INFO << "EthernetSessionMux: " << session << " is released, its port " << networkInterface->getInterfaceName() << " goes" << endl;
    // the MAC table does not forget a deleted interface by itself (INET)
    macTable_->removeForwardingInterface(networkInterface->getInterfaceId());
    portOfGate_.erase(it->second.gateIndex);
    ports_.erase(it);
    interfaceTable_->deleteInterface(networkInterface);   // deletes the module, which disconnects its gates
}

void EthernetSessionMux::handleMessage(cMessage *msg)
{
    auto frame = check_and_cast<Packet *>(msg);
    if (msg->arrivedOn("sessionIn")) {
        // an uplink frame of a session: into the bridge on the session's port
        auto session = frame->getTag<SessionTag>();
        auto it = ports_.find(session->getLteNodeId());
        if (it == ports_.end())
            throw cRuntimeError("EthernetSessionMux: an uplink frame of the session of UE %d, which has no port here", (int)num(session->getLteNodeId()));
        frame->clearTags();   // the GTP-U side's, incl. the SessionTag; the port sets the receive tags
        send(frame, "portOut", it->second.gateIndex);
    }
    else if (msg->arrivedOn("portIn")) {
        // a frame the bridge sends into a session: downlink, tagged with the session
        auto it = portOfGate_.find(msg->getArrivalGate()->getIndex());
        ASSERT(it != portOfGate_.end());
        frame->clearTags();
        frame->addTag<PacketProtocolTag>()->setProtocol(&Protocol::ethernetMac);
        auto dispatch = frame->addTag<DispatchProtocolReq>();
        dispatch->setProtocol(&Protocol::ethernetMac);
        dispatch->setServicePrimitive(SP_INDICATION);
        attachSessionTag(frame, ports_.at(it->second).session);
        send(frame, "sessionOut");
    }
    else
        throw cRuntimeError("EthernetSessionMux: message arrived on unexpected gate %s", msg->getArrivalGate()->getFullName());
}

} //namespace
