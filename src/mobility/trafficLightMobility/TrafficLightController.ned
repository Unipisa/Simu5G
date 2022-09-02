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

simple TrafficLightController
{
    @class(TrafficLightController);
            
    double greenPeriod @unit(s) = default(1s);
    double yellowPeriod @unit(s) = default(1s);
    double redPeriod @unit(s) = default(1s);
    double startTime @unit(s) = default(1s);
    int startState @enum(OFF, GREEN, YELLOW, RED) = default(3); // do not use 0 or 2, for now!
    double meanCarLength @unit(m) = default(4m);
    double areaWidth @unit(m) = default(1m);
}