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

package simu5g.apps.mec.MecApps.MecRequestForegroundApp;

import simu5g.apps.mec.MecApps.MecAppBase;

//
// This module implements a MEC application that issues periodic GET requests to the
// ~LocationService. This application can be utilized as an "independent MEC app" (see
// ~MecHost module), hence it does not require instantiation via the MEC orchestrator
// upon request from a User Equipment. Essentially, this application provides a quick
// way to generate configurable load to the Location Service and simulate different
// (and potentially more realistic) response times from the latter.
//
// For now, this application does not consume resources of the MEC Host (i.e., resource
// parameters are not used). 
// 
// TODO consider resources used by this app in the VIM
//
simple MecRequestForegroundApp extends MecAppBase
{
    parameters:
        @class(MecRequestForegroundApp);

        mecAppId = default(0); // it will be overridden by module ID in initialize()
        
        // resource usage (not used so far)
        requiredRam = default(10MB);
        requiredDisk = default(100MB);
        requiredCpu = default(15000);
        
        double meanRequestTime @unit(s) = default(0.5s);    // mean time between consecutive requests to the LocationService
        bool exponentialDist = default(false);              // if true, requests are generated according to the exponential distribution with meanRequestTime as mean. If false, requests are periodic, every meanRequestTime
}
