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
// This module represents an application that is dynamically instantiated on a MEC host upon request
// from a ~UERequestApp, via a ~DeviceApp.
// Upon receiving a request from the UE, it queries the ~LocationService to obtain some
// information (which is not utilized; it is intended only to "consume" some time), and sends a
// response message to the UE after a computation time that depends on the current resource utilization at the
// MEC host.
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
