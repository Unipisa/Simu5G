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

#include "simu5g/common/L3Utils.h"
#include "simu5g/common/LteCommon.h"

#include <inet/common/packet/chunk/BytesChunk.h>
#include <inet/linklayer/ethernet/common/Ethernet.h>
#include <inet/linklayer/ethernet/common/EthernetMacHeader_m.h>
#include <inet/networklayer/common/L3Tools.h>
#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/networklayer/ipv6/Ipv6Header.h>

namespace simu5g {

using namespace inet;
using namespace omnetpp;

const Protocol& ipProtocolOf(const Packet *pkt)
{
    // A header object's type stands for the version field; a datagram in raw bytes
    // (e.g. from an emulation interface) has the field itself.
    const auto& front = pkt->peekAtFront();
    if (dynamicPtrCast<const Ipv4Header>(front))
        return Protocol::ipv4;
    if (dynamicPtrCast<const Ipv6Header>(front))
        return Protocol::ipv6;
    int version = pkt->peekDataAt<BytesChunk>(b(0), B(1))->getByte(0) >> 4;
    if (version == 4)
        return Protocol::ipv4;
    if (version == 6)
        return Protocol::ipv6;
    throw cRuntimeError("Packet '%s' does not start with an IP datagram (IP version field: %d)", pkt->getName(), version);
}

const Protocol& sessionPayloadProtocol(SessionType type, const Packet *pkt)
{
    switch (type) {
        case IP_V4:
        case IP_V6:
        case IP_V4V6:
            return ipProtocolOf(pkt);
        case UNSTRUCTURED:
            return LteProtocol::unstructured;
        case ETHERNET:
            return Protocol::ethernetMac;
        default:
            throw cRuntimeError("Packet '%s' belongs to a session of unknown type %d", pkt->getName(), (int)type);
    }
}

bool isNonIpSessionPayload(const Protocol *protocol)
{
    return protocol == &LteProtocol::unstructured || protocol == &Protocol::ethernetMac;
}

void insertDeclaredEthernetFcs(Packet *frame)
{
    auto fcs = makeShared<EthernetFcs>();
    fcs->setFcsMode(FCS_DECLARED_CORRECT);
    fcs->setFcs(computeEthernetFcs(frame, FCS_DECLARED_CORRECT));
    frame->insertAtBack(fcs);
}

Ptr<const NetworkHeaderBase> peekIpHeader(const Packet *pkt)
{
    return peekNetworkProtocolHeader(pkt, ipProtocolOf(pkt));
}

bool isLinkLocalScope(const Ipv6Address& address)
{
    return address.isLinkLocal() || (address.isMulticast() && address.getMulticastScope() <= 2);
}

bool addressSpecHostExists(cModule *context, const char *addressSpec)
{
    L3Address literal;
    if (literal.tryParse(addressSpec))
        return true;
    std::string path = addressSpec;
    path = path.substr(0, path.find_first_of("%(>"));
    return context->findModuleByPath(path.c_str()) != nullptr;
}

Ptr<const IpHeaderFieldsTag> attachIpHeaderFields(Packet *pkt)
{
    const auto& ipHeader = peekIpHeader(pkt);
    auto tag = pkt->addTagIfAbsent<IpHeaderFieldsTag>();
    tag->setSrcAddress(ipHeader->getSourceAddress());
    tag->setDestAddress(ipHeader->getDestinationAddress());
    if (auto ipv4Header = dynamicPtrCast<const Ipv4Header>(ipHeader))
        tag->setTos(ipv4Header->getTypeOfService());
    else
        tag->setTos(staticPtrCast<const Ipv6Header>(ipHeader)->getTrafficClass());
    return tag;
}

} // namespace simu5g
