// 
//                           Simu5G
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 

package lte.stack.backgroundTrafficGenerator;

import inet.mobility.contract.IMobility;
import lte.stack.backgroundTrafficGenerator.generators.ITrafficGenerator;

module BackgroundUe
{
    parameters:
        @display("i=device/pocketpc");
        string mobilityType = default("StationaryMobility");
        string trafficGenerator = default("TrafficGeneratorBase");
    submodules:
        mobility: <mobilityType> like IMobility {
            @display("p=57,61;is=s");
        }
        generator: <trafficGenerator> like ITrafficGenerator {
            @display("p=142,61;is=s");
        }
}
