
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
// This module represents a foreground MEC application that sends GET requests to the 
// Location Service every 500ms. This app does not require UEs to be instantiated 
// through the MEC orchestrator, making it useful for evaluating MEC host related 
// statistics, such as the load generator model of the MEC service to see the 
// execution time differences.
//
simple MecRequestForegroundApp extends MecAppBase
{
    parameters:
        @class(MecRequestForegroundApp);
        bool allocateHostResources = default(false);

        int packetSize @unit(B) = default(10B);
        double lambda @unit(s) = default(10ms);
}
