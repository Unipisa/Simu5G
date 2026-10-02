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

#include "simu5g/nodes/CellularNicNdSink.h"

#include <inet/common/ProtocolTag_m.h>
#include <inet/common/packet/Packet.h>
#include <inet/networklayer/icmpv6/Icmpv6Header_m.h>
#include <inet/networklayer/ipv6/Ipv6Header.h>

namespace simu5g {

using namespace inet;

Define_Module(CellularNicNdSink);

static bool isNeighborDiscovery(Packet *packet)
{
    if (packet->getTag<PacketProtocolTag>()->getProtocol() != &Protocol::ipv6)
        return false;
    const auto& ipv6Header = packet->peekAtFront<Ipv6Header>();
    if (ipv6Header->getProtocolId() != IP_PROT_IPv6_ICMP)
        return false;
    const auto& icmpv6Header = packet->peekDataAt<Icmpv6Header>(ipv6Header->getChunkLength());
    switch (icmpv6Header->getType()) {
        case ICMPv6_ROUTER_SOL:
        case ICMPv6_ROUTER_AD:
        case ICMPv6_NEIGHBOUR_SOL:
        case ICMPv6_NEIGHBOUR_AD:
        case ICMPv6_REDIRECT:
            return true;
        default:
            return false;
    }
}

void CellularNicNdSink::handleMessage(cMessage *msg)
{
    auto packet = check_and_cast<Packet *>(msg);
    if (!isNeighborDiscovery(packet))
        throw cRuntimeError("The network layer of the base station sent %s to the cellular NIC, which is no interface of the network layer: only IPv6 Neighbor Discovery is expected there, and discarded",
                packet->getName());
    EV_DETAIL << "Discarding " << packet->getName() << ": IPv6 Neighbor Discovery over the cellular NIC" << endl;
    delete packet;
}

} // namespace simu5g
