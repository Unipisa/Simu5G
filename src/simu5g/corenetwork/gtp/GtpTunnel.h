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

#ifndef _GTP_TUNNEL_H_
#define _GTP_TUNNEL_H_

#include <map>
#include <ostream>

#include <inet/common/packet/Packet.h>
#include <inet/networklayer/common/L3Address.h>

#include "simu5g/common/LteTypes.h"
#include "simu5g/common/SessionTag_m.h"
#include "simu5g/corenetwork/gtp/GtpUserMsg_m.h"

namespace simu5g {

/**
 * A fully qualified TEID (F-TEID, TS 29.244): the transport address of the
 * receiving end of a GTP-U tunnel, and the TEID that end allocated. The sending
 * end addresses its G-PDUs with it.
 */
struct FTeid
{
    inet::L3Address address;
    Teid teid = TEID_NONE;

    bool isSet() const { return teid != TEID_NONE; }
};

inline std::ostream& operator<<(std::ostream& os, const FTeid& fteid)
{
    return os << fteid.address.str() << "/teid=" << fteid.teid;
}

/**
 * The session a GTP-U tunnel belongs to -- the UE's PDN connection under an EPC, its
 * PDU session under a 5G core -- as the ends of the tunnel know it: the session's UE,
 * by its node id on each stack, and the session id.
 */
struct SessionRef
{
    MacNodeId lteNodeId = NODEID_NONE;
    MacNodeId nrNodeId = NODEID_NONE;
    SessionId id = SessionId(0);
};

inline std::ostream& operator<<(std::ostream& os, const SessionRef& session)
{
    return os << "session " << session.id << " of UE " << session.lteNodeId << "/" << session.nrNodeId;
}

/**
 * Tells the modules after a tunnel end which session, and so which UE, a datagram
 * that arrived on the tunnel belongs to (see SessionTag).
 */
inline void attachSessionTag(inet::Packet *datagram, const SessionRef& session)
{
    auto tag = datagram->addTag<SessionTag>();
    tag->setSessionId(session.id);
    tag->setLteNodeId(session.lteNodeId);
    tag->setNrNodeId(session.nrNodeId);
}

/**
 * The uplink tunnels of a session, for a base station to send the UE's traffic
 * into the core network through: to the anchor UPF/PGW, and to each MEC host UPF of
 * the anchor's core network, by the address of that UPF.
 */
struct UplinkTunnels
{
    FTeid anchor;
    std::map<inet::L3Address, Teid> mecHosts;
    bool toUpf = false;   // the anchor is a UPF (5GC): the G-PDUs carry a PDU Session Container
};

/**
 * A session's resources at a base station: the session, its uplink tunnels into
 * the core network, and the base station's downlink F-TEID, under which the session's
 * downlink -- from the core network, or forwarded by a handover source over X2-U --
 * enters the node (see ConnectionControlEnb::sessionTunnelSetup()).
 */
struct SessionResource
{
    SessionRef ref;
    UplinkTunnels uplink;
    FTeid dl;
};

/**
 * The GTP-U header of a G-PDU (TS 29.281) that carries a T-PDU of the given length on
 * the tunnel with the given TEID; with a PDU Session Container that carries the QFI
 * (TS 38.415), unless container is PDU_SESSION_CONTAINER_NONE.
 */
inline inet::Ptr<GtpUserMsg> makeGtpUserHeader(Teid teid, Qfi qfi, PduSessionContainerType container, inet::B tpduLength)
{
    auto header = inet::makeShared<GtpUserMsg>();
    header->setTeid(teid);
    header->setQfi(qfi);
    header->setPduSessionContainer(container);
    inet::B headerLength = gtpUserHeaderLength(container);
    header->setChunkLength(headerLength);
    header->setLengthField((headerLength - inet::B(8) + tpduLength).get());
    return header;
}

} //namespace

#endif
