
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

package simu5g.apps.mec.MecRequestResponseApp;

import simu5g.apps.mec.MecApps.MecAppBase;


//
// This module implements a Mobile Edge Computing (MEC) application that processes
// incoming requests from User Equipment (UE) applications. It establishes connection
// with a service registry to retrieve the necessary service endpoints and communicate
// through TCP and UDP sockets to handle requests, send responses, and process service messages.
//
simple MECResponseApp extends MecAppBase
{
    parameters:
        @class(MecResponseApp);
        @display("i=block/app;is=s");

        int responsePacketSize @unit(B) = default(50B);

        int minInstructions = default(9000000);
        int maxInstructions = default(11000000);
}
