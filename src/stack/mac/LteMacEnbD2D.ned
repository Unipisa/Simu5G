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
// Medium Access Control (MAC) layer of the evolved NodeB (eNodeB) for Device-to-Device (D2D) communication.
// This module manages the MAC layer functionalities for eNodeBs, including scheduling, resource allocation, 
// and Hybrid Automatic Repeat Request (HARQ) processes. It enhances the 5G network by enabling direct 
// communication between UEs, improving spectral efficiency, and supporting advanced services such as 
// ultra-reliable low latency communications (URLLC).
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
