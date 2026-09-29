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

#ifndef _L3_UTILS_H_
#define _L3_UTILS_H_

#include <inet/common/Protocol.h>
#include <inet/common/packet/Packet.h>
#include <inet/networklayer/contract/NetworkHeaderBase_m.h>
#include <inet/networklayer/contract/ipv6/Ipv6Address.h>

#include "simu5g/common/IpHeaderFieldsTag_m.h"
#include "simu5g/common/LteCommonEnum_m.h"

namespace simu5g {

// The IP version of the datagram at the front of the packet, Protocol::ipv4 or
// Protocol::ipv6, taken from the datagram itself (3GPP does not signal it per packet,
// a receiver of an IPv4v6 session reads the version field). Throws if the packet
// does not start with an IP datagram.
const inet::Protocol& ipProtocolOf(const inet::Packet *pkt);

// The protocol of the payload of a session of the given type, which the packet is:
// the IP version of the datagram on an IP session (see ipProtocolOf()), the
// unstructured protocol on an Unstructured one. Throws for an Ethernet session,
// which is not supported yet.
const inet::Protocol& sessionPayloadProtocol(SessionType type, const inet::Packet *pkt);

// The IP header at the front of the packet, of either family
inet::Ptr<const inet::NetworkHeaderBase> peekIpHeader(const inet::Packet *pkt);

// Parses the IP header at the front of a user-plane packet and attaches its fields
// as an IpHeaderFieldsTag, replacing any tag already there. Called where the packet
// enters a node's user plane; the node's later modules read the tag.
inet::Ptr<const IpHeaderFieldsTag> attachIpHeaderFields(inet::Packet *pkt);

// Whether an IPv6 destination has link-local scope: link-local unicast, or a
// multicast group of interface-local or link-local scope
bool isLinkLocalScope(const inet::Ipv6Address& address);

// Whether the host an L3AddressResolver address spec names exists yet, looked up
// relative to the given module: the "host" part of "host", "host%interface",
// "host(ipv6)" or "host>peer". A literal address counts as existing.
bool addressSpecHostExists(omnetpp::cModule *context, const char *addressSpec);

// The DSCP field of a DSCP and ECN octet (see IpHeaderFieldsTag::tos)
inline uint8_t dscpOf(uint8_t tos) { return (tos & 0xfc) >> 2; }

} // namespace simu5g

#endif
