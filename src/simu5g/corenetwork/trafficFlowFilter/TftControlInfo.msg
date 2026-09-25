//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

import inet.common.INETDefs;
import simu5g.common.LteCommon;
import inet.common.TagBase;

namespace simu5g;

//
// Where a traffic flow filter sends a datagram entering the user plane of its node
// (see ~TrafficFlowFilter, ~GtpUser)
//
enum TftOutcome
{
    TFT_REMOVED_DESTINATION = 0;    // Dropped: the destination left the simulation, or is attached nowhere
    TFT_EXTERNAL_DESTINATION = 1;   // To the core network gateway
    TFT_LOCAL_DELIVERY = 2;         // Back into this base station's radio network
    TFT_MEC_HOST = 3;               // To the UPF of the destination MEC host
    TFT_PDU_SESSION = 4;            // Into the downlink tunnel of the destination UE's PDU session
}

class TftControlInfo extends inet::TagBase
{
    TftOutcome tft;
    MacNodeId ueNodeId = NODEID_NONE;   // TFT_PDU_SESSION: the destination UE, by one of its node ids
    Qfi qfi;  // QFI derived from IP DSCP (simplified PDR matching)
};
