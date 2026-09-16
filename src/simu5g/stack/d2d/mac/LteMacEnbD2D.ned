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


package simu5g.stack.d2d.mac;

import simu5g.stack.mac.LteMacEnb;

//
// Extends LTE MAC functionalities at the eNodeB side to support device-to-device (D2D)
// communications. Specifically, it enhances resource allocation procedures to enable direct
// communication between User Equipments (UEs), i.e., it allocates resources for D2D transmissions.
// Network-assisted D2D is only implemented (out-of-coverage D2D is not supported).
//
module LteMacEnbD2D extends LteMacEnb
{
    parameters:
        @class("LteMacEnbD2D");

        // Use the D2D-capable AMC leaf
        amcType = default("LteAmcD2D");

        // Use the D2D-capable uplink scheduler leaf
        ulSchedulerType = default("LteSchedulerEnbUlD2D");

        // Feedback historical base capacity in D2D (number of stored feedback samples per UE)
        int fbhbCapacityD2D = default(5);

        bool usePreconfiguredTxParams = default(false);
        int d2dCqi = default(7);

        // Frequency reuse parameters
        bool reuseD2D = default(false);
        bool reuseD2DMulti = default(false);
        double conflictGraphUpdatePeriod @unit(s) = default(1s);

        // CG thresholds can be defined in either dBm or meters
        // if distances are set to -1.0, then dBm thresholds are used,
        // otherwise distances have priority on dBm thresholds
        double conflictGraphThreshold = default(90);                       // dBm
        double conflictGraphD2DInterferenceRadius @unit(m) = default(-1.0m);         // Meters
        double conflictGraphD2DMultiTxRadius @unit(m) = default(-1.0m);              // Meters
        double conflictGraphD2DMultiInterferenceRadius @unit(m) = default(-1.0m);    // Meters

        // Handling of D2D mode switch
        bool msHarqInterrupt = default(true);
        bool msClearRlcBuffer = default(true);

        @signal[macCellPacketLossD2D];
        @statistic[macCellPacketLossD2D](title="Mac Cell Packet Loss D2D"; unit=""; source="macCellPacketLossD2D"; record=mean);
        @signal[macBufferOverFlowD2D];
        @statistic[macBufferOverFlowD2D](title="Mac buffer overflow as function of time"; unit="Byte/s"; source="macBufferOverFlowD2D"; record=mean);
}
