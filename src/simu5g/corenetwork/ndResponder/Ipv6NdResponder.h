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

#ifndef _IPV6_ND_RESPONDER_H_
#define _IPV6_ND_RESPONDER_H_

#include <inet/common/ModuleRefByPar.h>
#include <inet/common/checksum/ChecksumMode_m.h>
#include <inet/common/packet/Packet.h>
#include <inet/networklayer/contract/ipv6/Ipv6Address.h>
#include <inet/networklayer/icmpv6/Icmpv6Header_m.h>

#include "simu5g/common/SessionTag_m.h"
#include "simu5g/common/binder/Binder.h"

namespace simu5g {

using namespace omnetpp;

/**
 * Answers the IPv6 Neighbor Discovery traffic of the UEs' sessions at the
 * session anchor. See the NED file for details.
 */
class Ipv6NdResponder : public cSimpleModule
{
  protected:
    inet::ModuleRefByPar<Binder> binder_;
    inet::Ipv6Address linkLocalAddress_;
    simtime_t routerLifetime_;
    int curHopLimit_ = 0;
    inet::ChecksumMode checksumMode_ = inet::CHECKSUM_MODE_UNDEFINED;

    int numRouterAdvertisementsSent_ = 0;
    int numNeighbourAdvertisementsSent_ = 0;
    int numDiscarded_ = 0;

  protected:
    void initialize() override;
    void handleMessage(cMessage *msg) override;

    virtual void answerRouterSolicitation(const inet::Ipv6Address& ueAddress, const SessionTag& session);
    virtual void answerNeighbourSolicitation(const inet::Ipv6Address& ueAddress, const SessionTag& session);
    virtual void sendToUe(const char *name, const inet::Ptr<inet::Icmpv6Header>& icmpMessage, const inet::Ipv6Address& ueAddress, const SessionTag& session);
    virtual void consume(inet::Packet *pkt, const char *what);   // the protocol wants no answer
    virtual void discard(inet::Packet *pkt, const char *reason); // this module cannot handle it
};

} // namespace simu5g

#endif
