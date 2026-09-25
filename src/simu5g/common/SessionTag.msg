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
// The PDU session a downlink datagram belongs to at a base station, and so the UE it
// is for. ~GtpUser attaches it where the datagram enters the base station: from the
// GTP-U tunnel it arrived on, or, on the shortcut between two UEs of the same base
// station (TrafficFlowFilter's fastForwarding), from the destination UE's session. A
// datagram the handover source forwarded over X2 carries no tunnel identity yet, and
// ~HandoverPacketHolderEnb derives the tag from its destination address, leaving
// sessionId unset. The modules after the entry read the UE from this tag, and each
// applies its own policy for picking one of the UE's node ids (see
// ~HandoverPacketHolderEnb, ~Ip2Nic).
//
// The tag is node-local: removeAllSimu5GTags() leaves it in place, and every node
// attaches its own.
//
class SessionTag extends inet::TagBase {
    SessionId sessionId;
    MacNodeId lteNodeId = NODEID_NONE;   // The UE's node id on its LTE stack
    MacNodeId nrNodeId = NODEID_NONE;    // The UE's node id on its NR stack; NODEID_NONE if it has none
}
