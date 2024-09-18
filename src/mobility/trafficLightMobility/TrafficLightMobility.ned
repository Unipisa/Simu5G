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
// This module extends the INET's Linear Mobility model to mimic the movement of
// a vehicle moving across roads with interserctions regulated by traffic lights.
// The node implementing this mobility model moves at constant speed and, if it 
// encounters a ~TrafficLight module whose color is "red", then it stops until the
// traffic light's color becomes "green". When resuming its movement, the node may
// change its direction if the "enableTurns" parameter is enabled. It requires the
// presence of ~TrafficLight modules in the same network scenario.
//
simple TrafficLightMobility extends LinearMobility
{
    parameters:
        @class(TrafficLightMobility);

        int initialRoadIndex = default(0);
        object trafficLightsList = default([]);
        bool enableTurns = default(false);
}
