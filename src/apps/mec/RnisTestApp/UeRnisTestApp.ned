
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

import inet.applications.contract.IApp;

//
// This module defines the UeRnisTestApp, a UE application that interacts with 
// a DeviceApp to instantiate and communicate with a MEC (Mobile Edge Computing) app. 
// The UeRnisTestApp requests the MEC app to query the RNI (Radio Network Information) 
// service periodically and outputs the received information to the Qtenv log. 
// If the querying period is set to 0, a single query is made.
//
simple UeRnisTestApp like IApp
{
    parameters:
        @display("i=block/source");

        //connection infos
        int localPort = default(4000);
        int deviceAppPort = default(4000); // port of the DeviceApp
        string deviceAppAddress;           // ipAddress of the DeviceApp

        double queryingPeriod @unit("s") = default(1s);   // RNIS querying period

        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value

        //autoscheduling infos
        double period @unit("s") = default(1s);
        double startTime @unit("s") = default(0s);
        double stopTime @unit("s") = default(120s);

        string mecAppName = default("MecRnisTestApp");

        bool logger = default(false);

    gates:
        output socketOut;
        input socketIn;
}

