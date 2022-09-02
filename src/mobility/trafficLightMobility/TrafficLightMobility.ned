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
// This is a linear mobility model with speed and angle parameters.
// Angle changes when the mobile node reaches a traffic light.
// When the mobile node hits a wall, it reflects off the wall at the same angle.
//
simple TrafficLightMobility extends LinearMobility
{
    parameters:
        
        @class(TrafficLightMobility);

		int initialRoadIndex = default(0);
        string trafficLightsList = default("");
        bool enableTurns = default(false);
}