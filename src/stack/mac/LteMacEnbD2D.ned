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


package simu5g.stack.mac;

//
// This module extends LTE MAC functionalities at the eNodeB side to support device-to-device (D2D)
// communications. Specifically, it enhances resource allocation procedures to enable direct
// communication between User Equipments (UEs), i.e., it allocates resources for D2D transmissions.
// Network-assisted D2D is only implemented (out-of-coverage D2D is not supported).
//
simple LteMacEnbD2D extends LteMacEnb
{
    parameters:
        @class("LteMacEnbD2D");

        bool usePreconfiguredTxParams = default(false);
        int d2dCqi = default(7);

        // frequency reuse parameters
        bool reuseD2D = default(false);
        bool reuseD2DMulti = default(false);
        double conflictGraphUpdatePeriod @unit(s) = default(1s);

        // CG thresholds can be defined in either dBm or meters
        // if distances are set to -1.0, then dBm thresholds are used,
        // otherwise distances have priority on dBm thresholds
        double conflictGraphThreshold = default(90);                       // dBm
        double conflictGraphD2DInterferenceRadius @unit(m) = default(-1.0m);         // meters
        double conflictGraphD2DMultiTxRadius @unit(m) = default(-1.0m);              // meters
        double conflictGraphD2DMultiInterferenceRadius @unit(m) = default(-1.0m);    // meters

        // handling of D2D mode switch
        bool msHarqInterrupt = default(true);
        bool msClearRlcBuffer = default(true);

        @signal[macCellThroughputD2D];
        @statistic[macCellThroughputD2D](title="Cell Throughput at the MAC layer D2D"; unit="Bps"; source="macCellThroughputD2D"; record=mean);
        @signal[macCellPacketLossD2D];
        @statistic[macCellPacketLossD2D](title="Mac Cell Packet Loss D2D"; unit=""; source="macCellPacketLossD2D"; record=mean);
}
