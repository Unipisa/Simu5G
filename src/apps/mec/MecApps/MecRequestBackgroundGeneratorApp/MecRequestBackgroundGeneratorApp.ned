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
//
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

        bool logger = default(false);
}
