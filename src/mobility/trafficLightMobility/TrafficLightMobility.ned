
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
// This module represents a mobility model that allows for dynamic movement
// of nodes based on traffic light signals. Nodes can change direction at 
// traffic lights and will reflect off boundaries with the same angle.
//
// Parameters:
//   - initialRoadIndex: Index of the initial road where the node starts.
//   - trafficLightsList: List of traffic light modules that influence the node.
//   - enableTurns: Flag to enable or disable random turns at traffic lights.
//
simple TrafficLightMobility extends LinearMobility
{
    parameters:
        @class(TrafficLightMobility);

        int initialRoadIndex = default(0);
        object trafficLightsList = default([]);
        bool enableTurns = default(false);
}
