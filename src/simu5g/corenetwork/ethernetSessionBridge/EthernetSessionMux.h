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

#ifndef _ETHERNETSESSIONMUX_H_
#define _ETHERNETSESSIONMUX_H_

#include <map>

#include <inet/common/ModuleRefByPar.h>
#include <inet/common/packet/Packet.h>
#include <inet/linklayer/ethernet/contract/IMacForwardingTable.h>
#include <inet/networklayer/common/NetworkInterface.h>
#include <inet/networklayer/contract/IInterfaceTable.h>
#include <inet/queueing/contract/IPassivePacketSink.h>

#include "simu5g/corenetwork/gtp/GtpTunnel.h"

namespace simu5g {

/**
 * The inside of an EthernetSessionPort (see EthernetSessionBridge.ned). The port's
 * NetworkInterface pushes the frames the bridge sends down into it, so it is a
 * passive packet sink.
 */
class EthernetSessionPortal : public omnetpp::cSimpleModule, public virtual inet::queueing::IPassivePacketSink
{
  protected:
    inet::NetworkInterface *networkInterface_ = nullptr;

  protected:
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(omnetpp::cMessage *msg) override;

  public:
    bool canPushSomePacket(const omnetpp::cGate *gate) const override { return true; }
    bool canPushPacket(inet::Packet *packet, const omnetpp::cGate *gate) const override { return true; }
    void pushPacket(inet::Packet *packet, const omnetpp::cGate *gate) override;
    void pushPacketStart(inet::Packet *packet, const omnetpp::cGate *gate, inet::bps datarate) override { throw omnetpp::cRuntimeError("EthernetSessionPortal: packet streaming is not supported"); }
    void pushPacketEnd(inet::Packet *packet, const omnetpp::cGate *gate) override { throw omnetpp::cRuntimeError("EthernetSessionPortal: packet streaming is not supported"); }
    void pushPacketProgress(inet::Packet *packet, const omnetpp::cGate *gate, inet::bps datarate, inet::b position, inet::b extraProcessableLength = inet::b(0)) override { throw omnetpp::cRuntimeError("EthernetSessionPortal: packet streaming is not supported"); }
};

/**
 * The entry point and multiplexer of the EthernetSessionBridge: one session port per Ethernet
 * PDU session. See EthernetSessionBridge.ned.
 */
class EthernetSessionMux : public omnetpp::cSimpleModule
{
  protected:
    inet::ModuleRefByPar<inet::IInterfaceTable> interfaceTable_;
    inet::ModuleRefByPar<inet::IMacForwardingTable> macTable_;

    // A session's port, and the index of the mux gates its lower side is connected to
    struct Port {
        TunnelSession session;
        inet::NetworkInterface *networkInterface = nullptr;
        int gateIndex = -1;
    };
    std::map<MacNodeId, Port> ports_;          // by the UE's LTE node id
    std::map<int, MacNodeId> portOfGate_;      // portIn gate index -> the UE's LTE node id
    int portCounter_ = 0;                       // for the ports' module names

  protected:
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(omnetpp::cMessage *msg) override;

  public:
    // The Ethernet PDU session is established at this UPF: its port is created
    virtual void addSessionPort(const SessionRef& session);

    // The session is released: its port's MAC table entries are flushed and the port is
    // deleted; nothing happens if the session has no port here
    virtual void removeSessionPort(const SessionRef& session);
};

} //namespace

#endif
