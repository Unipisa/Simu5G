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

#include "simu5g/corenetwork/ndResponder/Ipv6NdResponder.h"

#include <inet/common/IProtocolRegistrationListener.h>
#include <inet/common/ProtocolTag_m.h>
#include <inet/networklayer/icmpv6/Icmpv6.h>
#include <inet/networklayer/icmpv6/Ipv6NdMessage_m.h>
#include <inet/networklayer/ipv6/Ipv6Header.h>

namespace simu5g {

using namespace inet;

Define_Module(Ipv6NdResponder);

void Ipv6NdResponder::initialize()
{
    binder_.reference(this, "binderModule", true);
    linkLocalAddress_ = Ipv6Address(par("linkLocalAddress").stringValue());
    if (!linkLocalAddress_.isLinkLocal())
        throw cRuntimeError("linkLocalAddress: %s is not a link-local address", linkLocalAddress_.str().c_str());
    routerLifetime_ = par("routerLifetime");
    if (routerLifetime_ < 0 || routerLifetime_ > 9000)
        throw cRuntimeError("routerLifetime: %s is outside the 0..9000s range", routerLifetime_.str().c_str());
    curHopLimit_ = par("curHopLimit");
    if (curHopLimit_ < 0 || curHopLimit_ > 255)
        throw cRuntimeError("curHopLimit: %d is outside the 0..255 range", curHopLimit_);
    checksumMode_ = parseChecksumMode(par("checksumMode"), false);

    // the Neighbor Discovery of the sessions' links reaches this module as the icmpv6 service
    registerService(Protocol::icmpv6, gate("in"), SP_REQUEST);

    WATCH(numRouterAdvertisementsSent_);
    WATCH(numNeighbourAdvertisementsSent_);
    WATCH(numDiscarded_);
}

void Ipv6NdResponder::handleMessage(cMessage *msg)
{
    auto pkt = check_and_cast<Packet *>(msg);
    const auto& session = pkt->getTag<SessionTag>();   // the session whose link the packet came on
    const auto& ipv6Header = pkt->peekAtFront<Ipv6Header>();
    const Ipv6Address& srcAddress = ipv6Header->getSrcAddress();

    if (ipv6Header->getProtocolId() != IP_PROT_IPv6_ICMP) {
        discard(pkt, "link-local traffic other than Neighbor Discovery");
        return;
    }

    const auto& icmpMessage = pkt->peekDataAt<Icmpv6Header>(ipv6Header->getChunkLength());
    if (dynamicPtrCast<const Ipv6RouterSolicitation>(icmpMessage)) {
        // a UE sends its solicitation from its link-local address once it has one
        if (srcAddress.isUnspecified())
            discard(pkt, "Router Solicitation without a source address");
        else {
            EV_INFO << "Ipv6NdResponder: Router Solicitation from " << srcAddress << ", answering it" << endl;
            answerRouterSolicitation(srcAddress, *session);
            delete pkt;
        }
    }
    else if (auto ns = dynamicPtrCast<const Ipv6NeighbourSolicitation>(icmpMessage)) {
        if (srcAddress.isUnspecified())
            consume(pkt, "duplicate address detection probe, no conflict to report");
        else if (ns->getTargetAddress() != linkLocalAddress_)
            discard(pkt, "Neighbor Solicitation for an address other than the router's");
        else {
            EV_INFO << "Ipv6NdResponder: Neighbor Solicitation from " << srcAddress << ", answering it" << endl;
            answerNeighbourSolicitation(srcAddress, *session);
            delete pkt;
        }
    }
    else
        discard(pkt, "link-local ICMPv6 message other than a Router or Neighbor Solicitation");
}

void Ipv6NdResponder::answerRouterSolicitation(const Ipv6Address& ueAddress, const SessionTag& session)
{
    auto ra = makeShared<Ipv6RouterAdvertisement>();
    ra->setCurHopLimit(curHopLimit_);
    ra->setManagedAddrConfFlag(false);
    ra->setOtherStatefulConfFlag(false);
    ra->setRouterLifetime(routerLifetime_.inUnit(SIMTIME_S));
    ra->setReachableTime(0);   // unspecified
    ra->setRetransTimer(0);    // unspecified

    // The session's /64, i.e. that of the UE's global address. The address is configured
    // statically, so the prefix is advertised for information only: the UE is not to
    // autoconfigure an address from it, nor to treat the prefix as on-link (the UPF is
    // the only neighbor on the session's link). (INET's hosts do not accept a Router
    // Advertisement without a prefix.)
    Ipv6Address globalAddress;
    for (const auto& address : binder_->getAddresses(binder_->getMacNodeId(ueAddress)))
        if (address.getType() == L3Address::IPv6 && address.toIpv6().getScope() == Ipv6Address::GLOBAL) {
            globalAddress = address.toIpv6();
            break;
        }
    if (globalAddress.isUnspecified())
        EV_WARN << "Ipv6NdResponder: " << ueAddress << " belongs to no UE with a global address, no prefix to advertise" << endl;
    else {
        auto prefixInfo = new Ipv6NdPrefixInformation();
        prefixInfo->setPrefix(globalAddress.getPrefix(64));
        prefixInfo->setPrefixLength(64);
        prefixInfo->setOnlinkFlag(false);
        prefixInfo->setAutoAddressConfFlag(false);
        prefixInfo->setValidLifetime(0xffffffff);      // infinity
        prefixInfo->setPreferredLifetime(0xffffffff);  // infinity
        ra->getOptionsForUpdate().appendOption(prefixInfo);
        ra->addChunkLength(IPv6ND_PREFIX_INFORMATION_OPTION_LENGTH);
    }
    sendToUe("RouterAdvertisement", ra, ueAddress, session);
    numRouterAdvertisementsSent_++;
}

void Ipv6NdResponder::answerNeighbourSolicitation(const Ipv6Address& ueAddress, const SessionTag& session)
{
    auto na = makeShared<Ipv6NeighbourAdvertisement>();
    na->setRouterFlag(true);
    na->setSolicitedFlag(true);
    na->setOverrideFlag(false);   // there is no link-layer address to override
    na->setTargetAddress(linkLocalAddress_);
    sendToUe("NeighbourAdvertisement", na, ueAddress, session);
    numNeighbourAdvertisementsSent_++;
}

void Ipv6NdResponder::sendToUe(const char *name, const Ptr<Icmpv6Header>& icmpMessage, const Ipv6Address& ueAddress, const SessionTag& session)
{
    auto packet = new Packet(name);
    Icmpv6::insertChecksum(checksumMode_, icmpMessage, packet);
    packet->insertAtFront(icmpMessage);

    auto ipv6Header = makeShared<Ipv6Header>();
    ipv6Header->setSrcAddress(linkLocalAddress_);
    ipv6Header->setDestAddress(ueAddress);
    ipv6Header->setHopLimit(255);   // Neighbor Discovery messages are accepted only with 255 (RFC 4861)
    ipv6Header->setProtocolId(IP_PROT_IPv6_ICMP);
    ipv6Header->setPayloadLength(packet->getDataLength());
    packet->insertAtFront(ipv6Header);
    packet->addTag<PacketProtocolTag>()->setProtocol(&Protocol::ipv6);
    auto dispatchProtocolReq = packet->addTag<DispatchProtocolReq>();
    dispatchProtocolReq->setProtocol(&Protocol::ipv6);
    dispatchProtocolReq->setServicePrimitive(SP_INDICATION);
    *packet->addTag<SessionTag>() = session;   // the reply goes back on the session's link
    send(packet, "out");
}

void Ipv6NdResponder::consume(Packet *pkt, const char *what)
{
    EV_INFO << "Ipv6NdResponder: " << pkt->getName() << " consumed: " << what << endl;
    delete pkt;
}

void Ipv6NdResponder::discard(Packet *pkt, const char *reason)
{
    EV_WARN << "Ipv6NdResponder: " << pkt->getName() << " discarded: " << reason << endl;
    numDiscarded_++;
    delete pkt;
}

} // namespace simu5g
