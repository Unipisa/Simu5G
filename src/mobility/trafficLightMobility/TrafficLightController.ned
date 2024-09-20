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

//
// This module defines the behavior of traffic light controllers in the simulation.
// It manages the states of the traffic lights (green, yellow, red) and handles the
// interactions with vehicles, thus influencing the behavior of a node implementing the
// ~TrafficLightMobility mobility model when such a node reaches the position of the
// traffic light. This module allows for the configuration of the duration of
// green-yellow-red cycles.
//
simple TrafficLightController
{
    @class(TrafficLightController);

    string mobilityModule;
    double greenPeriod @unit(s) = default(1s);
    double yellowPeriod @unit(s) = default(1s);
    double redPeriod @unit(s) = default(1s);
    double startTime @unit(s) = default(1s);
    int startState @enum(OFF,GREEN,YELLOW,RED) = default(3); // do not use 0 or 2, for now!
    double meanCarLength @unit(m) = default(4m);
    double areaWidth @unit(m) = default(1m);
    bool bidirectional = default(false);
}
