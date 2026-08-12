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


package simu5g.common.binder;

//
// Binder module
//
// The Binder module functions as a global repository of network-related information
// utilized throughout the simulation for various control-plane functionalities. It provides
// a centralized storage and access point for data such as network configurations and
// operational parameters.
// Only one instance of this module is permitted within the entire network, ensuring
// the consistency and integrity of the shared information.
//
simple Binder
{
    parameters:
        int blerShift = default(0);
        double maxDataRatePerRb @unit("Mbps") = default(1.16Mbps);
        bool printTrafficGeneratorConfig = default(false);

        // Lowest elevation at which a cell is willing to use its satellite. It sets the longest
        // service and feeder links a cell can have, and therefore the worst-case round-trip delay
        // that getNtnCellRoundTripDelay() reports and the NTN protocol timers are dimensioned
        // against. TR 38.821 clause 6.1.1 uses 10 degrees, which is what makes the reported delays
        // reproduce the 541.46ms (GEO) and 25.77ms (LEO-600) of its clause 7.2.
        // Only read on the NTN path; a network with no satellites never evaluates it.
        double ntnMinElevation @unit(deg) = default(10deg);

        // Added to every cell round-trip delay. The timers derived from that delay must be an
        // upper bound rather than an estimate -- overestimating costs latency, underestimating
        // breaks the protocol -- so this is the slack knob for a scenario whose real geometry sits
        // close to a timer's next legal value. Zero reproduces TR 38.821 exactly.
        double ntnRoundTripDelayMargin @unit(s) = default(0s);

        // Sanity guard on the resolved satellite position. Below this altitude the geometry is
        // rejected rather than used, which catches a round-trip-delay query issued before
        // inet::INITSTAGE_SINGLE_MOBILITY -- at that point every radio still reports (0,0,0),
        // which converts to a plausible point on the geoid and would otherwise pass unnoticed.
        double ntnMinSatelliteAltitude @unit(m) = default(100km);
        @display("i=block/cogwheel");
}
