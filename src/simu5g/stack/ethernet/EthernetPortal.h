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

#ifndef _ETHERNETPORTAL_H_
#define _ETHERNETPORTAL_H_

#include <inet/common/packet/Packet.h>
#include <inet/networklayer/common/NetworkInterface.h>
#include <inet/queueing/contract/IPassivePacketSink.h>

namespace simu5g {

/**
 * The top of a UE's cellular NIC in Ethernet mode; see EthernetPortal.ned. The NIC's
 * NetworkInterface pushes the frames of the link layer (whose Ethernet layer pushes
 * them) into it, so it is a passive packet sink.
 */
class EthernetPortal : public omnetpp::cSimpleModule, public virtual inet::queueing::IPassivePacketSink
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
    void pushPacketStart(inet::Packet *packet, const omnetpp::cGate *gate, inet::bps datarate) override { throw omnetpp::cRuntimeError("EthernetPortal: packet streaming is not supported"); }
    void pushPacketEnd(inet::Packet *packet, const omnetpp::cGate *gate) override { throw omnetpp::cRuntimeError("EthernetPortal: packet streaming is not supported"); }
    void pushPacketProgress(inet::Packet *packet, const omnetpp::cGate *gate, inet::bps datarate, inet::b position, inet::b extraProcessableLength = inet::b(0)) override { throw omnetpp::cRuntimeError("EthernetPortal: packet streaming is not supported"); }
};

} //namespace

#endif
