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

#include <inet/networklayer/common/L3Address.h>

#include "simu5g/common/LteTypes.h"

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
 * The PDU session a GTP-U tunnel belongs to, as the ends of the tunnel know it: the
 * session's UE, by its node id on each stack, and the PDU Session ID.
 */
struct SessionRef
{
    MacNodeId lteNodeId = NODEID_NONE;
    MacNodeId nrNodeId = NODEID_NONE;
    SessionId id = SessionId(0);

    // true if nodeId is one of the UE's node ids
    bool isUe(MacNodeId nodeId) const { return nodeId != NODEID_NONE && (nodeId == lteNodeId || nodeId == nrNodeId); }
};

inline std::ostream& operator<<(std::ostream& os, const SessionRef& session)
{
    return os << "PDU session " << session.id << " of UE " << session.lteNodeId << "/" << session.nrNodeId;
}

/**
 * The uplink tunnels of a PDU session, for a base station to send the UE's traffic
 * into the core network through: to the anchor UPF/PGW, and to each MEC host UPF of
 * the anchor's core network, by the address of that UPF.
 */
struct UplinkTunnels
{
    FTeid anchor;
    std::map<inet::L3Address, Teid> mecHosts;
};

} //namespace

#endif
