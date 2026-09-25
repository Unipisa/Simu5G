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
// At a base station: the UE an uplink datagram was received from over the air, by
// the node id the base station's stack knows it by. Attached where the datagram
// leaves the radio stack (~Ip2Nic) and consumed where it enters the uplink tunnel of
// the UE's PDU session (~GtpUser), so the tunnel is chosen by the UE the datagram
// came from, whatever its payload.
//
// The tag is node-local: removeAllSimu5GTags() leaves it in place.
//
class UplinkUeTag extends inet::TagBase {
    MacNodeId ueNodeId = NODEID_NONE;
}
