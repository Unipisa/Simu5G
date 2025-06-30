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

package simu5g.mobility.trafficLightMobility;

import inet.mobility.static.StationaryMobility;

//
// This module simulates a traffic light in an urban environment.
// It incorporates a stationary mobility submodule to represent the fixed position
// of the traffic light, and a ~TrafficLightController submodule to model
// the traffic light's states (green, yellow, red) and its interactions with vehicles.
//
module TrafficLight
{
    parameters:
        @display("i=misc/node2_s;bgb=200,100");
        bool bidirectional = default(false);

    submodules:
        mobility: StationaryMobility {
            parameters:
                @display("p=142,52;is=s");
        }
        trafficLightController: TrafficLightController {
            parameters:
                @display("p=48,52;is=s");
                mobilityModule = default("^.mobility");
                bidirectional = parent.bidirectional;
        }
}
