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

import inet.common.geometry.Geometry; // for Coord
import simu5g.common.LteControlInfo;

cplusplus {{
#include "simu5g/common/LteControlInfo.h"
}}

namespace simu5g;

//
// Represents a physical layer transmission frame that encapsulates various types of
// messages/packets for transmission over the air interface, such as:
// - MAC PDUs (user data): transmitted as Transport Blocks (TB) on PDSCH/PUSCH
// - HARQ feedback: transmitted on PUCCH (uplink control channel)
// - Scheduling grants (DCI): transmitted on PDCCH (downlink control channel)
// - Random access and D2D packets: transmitted on respective control/data channels
// In the simulation, all these messages types are abstracted as AirFrame transmissions.
//
packet AirFrame extends cPacket {
    UserControlInfo additionalInfo @nopack;
    inet::Coord arrivalPosition = inet::Coord(NaN, NaN, NaN) @nopack; // where the receiving radio was when the frame started to arrive; set by the receiving CellularRadio, unspecified until then
    omnetpp::simtime_t receptionStartTime = -1 @nopack; // when the frame started to arrive at the receiving radio; set by the receiving CellularRadio, -1 until then
}
