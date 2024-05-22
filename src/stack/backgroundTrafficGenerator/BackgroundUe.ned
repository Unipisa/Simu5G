
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
// This module represents a background UE (User Equipment) in the network,
// responsible for generating background traffic. It includes configurable
// parameters for mobility and traffic generation.
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
