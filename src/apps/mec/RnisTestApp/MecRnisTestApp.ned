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

package simu5g.apps.mec.RnisTestApp;

import simu5g.apps.mec.MecApps.MecAppBase;


//
// This is a simple MEC app that connects to the Radio Network Information
// Service and periodically requests L2 Measurements from it.
// The response obtained from the RNIS is just sent back to the UE app.
//
simple MecRnisTestApp extends MecAppBase
{
    parameters:
        @class(MecRnisTestApp);
        @display("i=block/app;is=s");

        // Resource required info
        requiredRam = default(10MB);
        requiredDisk = default(1MB);
        requiredCpu = default(100);

        bool logger = default(false);
}
