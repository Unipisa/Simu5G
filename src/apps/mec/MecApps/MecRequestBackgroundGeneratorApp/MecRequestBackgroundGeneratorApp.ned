
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
// This module defines a MEC (Multi-access Edge Computing) application that
// functions as a background generator of requests. It is designed to simulate
// multiple instances of applications sending requests to MEC services. The
// primary purpose of this app is to create contention and allow the evaluation
// of various performance metrics, such as response time and system load,
// without relying on end-user devices. It includes parameters for resource
// allocation and detailed control over network-level configurations.
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
