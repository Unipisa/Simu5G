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

import inet.mobility.single.LinearMobility;

//
// This module extends INET's Linear Mobility model to simulate the movement of
// a vehicle navigating roads with intersections regulated by traffic lights.
// The node implementing this mobility model moves at a constant speed and, if it
// encounters a ~TrafficLight module with a "red" color, it stops until the
// traffic light turns "green". Upon resuming its movement, the node may
// change its direction if the "enableTurns" parameter is enabled. This module
// requires the presence of ~TrafficLight modules in the same network scenario.
//
simple TrafficLightMobility extends LinearMobility
{
    parameters:
        @class(TrafficLightMobility);

        int initialRoadIndex = default(0);
        object trafficLightsList = default([]);
        bool enableTurns = default(false);
}
