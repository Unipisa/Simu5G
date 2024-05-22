
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

package simu5g.apps.mec.MecApps.MecRequestBackgroundApp;

import simu5g.apps.mec.MecApps.MecAppBase;

//
// This module defines a background MEC (Multi-access Edge Computing) application
// that sends HTTP requests to MEC services at specified intervals. The application
// uses TCP sockets for communication and supports lifecycle management. The
// module parameters allow configuring the number of applications, resource
// allocation, packet size, server connection details, and traffic properties.
// The `lambda` parameter controls the mean interval between requests.
//
// This background application can generate synthetic load on MEC services
// to simulate various network conditions and assess service performance
// under different configurations.
//
simple MecRequestBackgroundApp extends MecAppBase
{
    parameters:
        @class(MecRequestBackgroundApp);
        int numberOfApplications = default(1);
        bool allocateHostResources = default(false);

        int packetSize @unit(B) = default(10B);

        mecAppId = default(0); // it will be override by getId() in initialize
        mecAppIndex = default(0);

        double lambda @unit(s) = default(10ms);

        //resource required info
        requiredRam = default(10MB);
        requiredDisk = default(100MB);
        requiredCpu = default(15000);

        string connectAddress = default("");  // server address (may be symbolic)
        int connectPort = default(1000); // port number to connect to
        double startTime @unit(s) = default(1s); // time first session begins
        double stopTime @unit(s) = default(-1s);  // time of finishing sending, negative values mean forever

        @display("i=block/app");
}
