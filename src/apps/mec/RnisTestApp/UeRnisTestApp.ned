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
// This UE app asks the Device App to instantiate a MecRnisTestApp.
// After the MEC app has been initialized, the UE app requests the MEC app to
// periodically query the RNI Service.
// When the UE app received the requested info from the MEC app, it just outputs
// that in the Qtenv log (set the logger parameter to dump the output to a file).
// If the period is set to 0, then only one query is requested.
//
simple UeRnisTestApp like IApp
{
    parameters:
        @display("i=block/source");

        //connection infos
        int localPort = default(4000);
        int deviceAppPort = default(4000); // ipAddress of the DeviceApp
        string deviceAppAddress;           // port of the DeviceApp

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

