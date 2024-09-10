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


package simu5g.apps.mec.MecApps.MecRequestBackgroundGeneratorApp;

import simu5g.apps.mec.MecApps.MecAppBase;

//
// This module implements a generator of requests to sent to the ~LocationService.
// This application can be instantiated as an "independent MEC app" (see ~MecHost 
// module), hence it does not require be instantiated via the MEC orchestrator upon 
// request from a User Equipment. In particular, this module produces bulks of 
// GET requests, as they are produced by a configurable number of MEC applications
// running on the MEC host. The effect of such requests is to delay the service 
// of real requests (i.e., coming from complete MEC applications)
//
simple MecRequestBackgroundGeneratorApp extends MecAppBase
{
    parameters:
        @class(MecRequestBackgroundGeneratorApp);
        int numberOfApplications;
        bool allocateHostResources = default(false);

        int packetSize @unit(B) = default(10B);

        //resource required info
        requiredRam = default(10MB);
        requiredDisk = default(100MB);
        requiredCpu = default(15000);
}
