//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//


package simu5g.stack.d2d.phy;

import simu5g.stack.phy.PhyUe;

//
// Extends the PHY functionalities at the User Equipment side to support device-to-device (D2D)
// communications. Specifically, it customizes the ~PhyUe functionalities to support the transmission and reception
// of frames to another UE directly, without going through the serving eNodeB. It also supports one-to-many
// D2D transmission to UEs belonging to a multicast group. It is assumed that one-to-one D2D communication is only possible
// when both endpoints are served by the same eNodeB. When a handover is triggered, this module is responsible
// for initiating a mode switch from direct (D2D) mode to infrastructure mode.
//
simple PhyUeD2D extends PhyUe
{
    parameters:
        @class("PhyUeD2D");
        double d2dTxPower @unit(dBm) = default(26dBm);
        bool d2dMulticastCaptureEffect = default(true);
        string d2dMulticastCaptureEffectFactor = default("RSRP");  // "RSRP", otherwise distance-based
        bool enableMulticastD2DRangeCheck = default(false);
        double multicastD2DRange @unit(m) = default(1000m);

        //# D2D CQI statistic
        @signal[averageCqiD2D];
        @statistic[averageCqiD2D](title="Average Cqi reported in D2D"; unit="cqi"; source="averageCqiD2D"; record=mean,vector);
}

