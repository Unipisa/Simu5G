//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

import simu5g.stack.phy.packet.AirFrame;

cplusplus {{
#include "simu5g/common/LteCommon.h"  // for RemoteUnitPhyDataVector
}}

namespace simu5g;

//
// Represents a physical layer transmission frame that encapsulates various types of
// messages/packets for transmission over the air interface, such as:
// - MAC PDUs (user data): transmitted as Transport Blocks (TB) on PDSCH/PUSCH
// - HARQ feedback: transmitted on PUCCH (uplink control channel)
// - Scheduling grants (DCI): transmitted on PDCCH (downlink control channel)
// - Random access and D2D packets: transmitted on respective control/data channels
// In the simulation, all these messages types are abstracted as LteAirFrame transmissions.
//
packet LteAirFrame extends AirFrame {
    @customize(true);
    RemoteUnitPhyData remoteUnitPhyDataVector[] @nopack;
}

cplusplus(LteAirFrame) {{
  public:
    void addRemoteUnitPhyDataVector(const RemoteUnitPhyData& data) { appendRemoteUnitPhyDataVector(data); }
}}
