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
// This module implements a lightweight model for a 4G or 5G User Equipment (UE).
// It is used to simulate the generation of background traffic in both uplink and
// downlink bandwidth, according to the logic specified by its ~ITrafficGenerator 
// submodule. Generated traffic is not actually transmitted/received by this module,
// making it more efficient from the simulation complexity perspective.
// Instead, such traffic modifies the buffer status information that is used by the
// ~BackgroundScheduler to allocate resources.
// Each background UE has a mobility submodule to configure its position in the 
// simulation floorplan. This way, realistic channel status information can be 
// computed for the background UE and used by the ~BackgroundScheduler.
// Since this module does not send/receive actual traffic, application-level 
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
