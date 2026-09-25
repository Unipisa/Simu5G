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
import simu5g.common.LteCommonEnum;

namespace simu5g;

//
// At a dual connectivity node: the bearer a PDCP PDU relayed over X2 belongs to, as the
// X2-U tunnel it arrived on names it (see ~GtpUserX2). Each dual connectivity bearer has
// one tunnel per direction, allocated by its receiving end: to the secondary for the
// downlink the master relays, to the master for the uplink the secondary relays back.
// Attached by ~GtpUserX2, read by ~DcMux. The tag is node-local.
//
class X2DcTunnelInd extends inet::TagBase {
    MacNodeId ueNodeId = NODEID_NONE;   // The UE, by the id this node keys the bearer by
    DrbId drbId;
    Direction direction = DL;           // DL: relayed by the master; UL: relayed by the secondary
}
