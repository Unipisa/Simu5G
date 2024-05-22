
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
// This module represents a simple MEC app that connects to the Radio Network Information
// Service (RNIS) and periodically requests L2 Measurements. The measurements obtained
// from the RNIS are then sent back to the UE application. It can be used to demonstrate
// the interaction between UE and MEC applications for querying RNIS.
//
simple MecRnisTestApp extends MecAppBase
{
    parameters:
        @class(MecRnisTestApp);
        @display("i=block/app;is=s");

        int localUePort;

        // Resource required info
        requiredRam = default(10MB);
        requiredDisk = default(1MB);
        requiredCpu = default(100);

        bool logger = default(false);
}
