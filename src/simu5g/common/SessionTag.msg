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

import inet.common.INETDefs;
import inet.common.TagBase;
import simu5g.common.LteCommon;

namespace simu5g;

//
// The session a downlink datagram belongs to at a base station, and so the UE it
// is for. It is attached where the datagram enters the base station: from the GTP-U
// tunnel it arrived on, from the core network (~GtpUser) or from a handover source
// (~GtpUserX2), or, on the shortcut between two UEs of the same base station
// (TrafficFlowFilter's fastForwarding), from the destination UE's session (~GtpUser).
// The modules after the entry read the UE from this tag, and each applies its own
// policy for picking one of the UE's node ids (see ~HandoverPacketHolderEnb, ~Ip2Nic).
// A datagram forwarded to a handover target keeps it until ~GtpUserX2 sends it on the
// session's tunnel there.
//
// The tag is node-local: removeAllSimu5GTags() leaves it in place, and every node
// attaches its own.
//
class SessionTag extends inet::TagBase {
    SessionId sessionId;
    MacNodeId lteNodeId = NODEID_NONE;   // The UE's node id on its LTE stack
    MacNodeId nrNodeId = NODEID_NONE;    // The UE's node id on its NR stack; NODEID_NONE if it has none
}

//
// Marks a packet that stands for a GTP-U End Marker (TS 29.281 7.3.2): the end of a PDU
// session's downlink on the tunnel it arrived on, which the anchor sends at the path
// switch. It carries no data. The old base station relays it over X2 to the base
// station the UE moved to, which releases the downlink it has held back from the new
// path until then (see ~HandoverPacketHolderEnb). Attached by ~GtpUser and ~GtpUserX2
// where the End Marker arrives; node-local, like SessionTag.
//
class GtpEndMarkerInd extends inet::TagBase {
}
