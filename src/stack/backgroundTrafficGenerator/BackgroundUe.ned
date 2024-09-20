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


package simu5g.stack.backgroundTrafficGenerator;

import inet.mobility.contract.IMobility;
import simu5g.stack.backgroundTrafficGenerator.generators.ITrafficGenerator;

//
// This module implements a lightweight model for 4G or 5G User Equipment (UE).
// It is used to simulate the generation of background traffic in both uplink and
// downlink bandwidths, according to the logic specified by its ~ITrafficGenerator
// submodule. The generated traffic is not actually transmitted or received by this module,
// making it more efficient from a simulation complexity perspective.
// Instead, such traffic modifies the buffer status information that is used by the
// ~BackgroundScheduler to allocate resources.
// Each background UE has a mobility submodule to configure its position in the
// simulation floor plan. This allows for realistic channel status information to be
// computed for the background UE and used by the ~BackgroundScheduler.
// Since this module does not send or receive actual traffic, application-level
// statistics are not produced.
//
module BackgroundUe
{
    parameters:
        @display("i=device/pocketpc");
        bool computeAvgInterference;
        bool enablePeriodicCqiUpdate;
        bool useProbabilisticCqi;
        *.computeAvgInterference = this.computeAvgInterference;
        *.enablePeriodicCqiUpdate = this.enablePeriodicCqiUpdate;
        *.useProbabilisticCqi = this.useProbabilisticCqi;
    submodules:
        mobility: <default("StationaryMobility")> like IMobility if typename != "" {
            @display("p=57,61;is=s");
        }
        generator: <default("TrafficGeneratorBase")> like ITrafficGenerator {
            @display("p=142,61;is=s");
        }
}
