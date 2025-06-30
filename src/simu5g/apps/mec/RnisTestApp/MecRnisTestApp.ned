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
// This module is an application that is dynamically instantiated on a MEC host upon request
// from a ~UeRnisTestApp via a ~DeviceApp. It is used to test and showcase the functionalities
// of the ~RNIService API.
// Upon receiving a request from the UE, it queries the ~RNIService periodically to obtain
// L2 measurements related to the UE that sent the request (refer to the sendQuery() function). The
// results obtained from the RNIS are sent back to the ~UeRnisTestApp in a response message.
// Please note that the response time of the RNIS depends on the current load at the RNIS.
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
