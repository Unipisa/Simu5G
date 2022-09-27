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

package simu5g.apps.mec.Plaas_framework.mobility;

import inet.mobility.single.LinearMobility;

//
// This is a linear mobility model with speed, angle and acceleration parameters.
// Angle only changes when the mobile node hits a wall: then it reflects
// off the wall at the same angle.
//
simple LinearAccelerationMobility extends LinearMobility
{
    parameters:

        double initialAcceleration = default(0); // initial acceleration of the host
        @class(LinearAccelerationMobility);
}

