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

module BackgroundUe
{
    parameters:
        @display("i=device/pocketpc");
        mobility.typename = default("StationaryMobility");
        string trafficGeneratorType = default("TrafficGeneratorBase");
    submodules:
        mobility: <default("")> like IMobility if typename != "" {
            @display("p=57,61;is=s");
        }
        generator: <trafficGeneratorType> like ITrafficGenerator {
            @display("p=142,61;is=s");
        }
}
