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
// This application is to be instantiated on the UE side. It requires that a ~DeviceApp is
// also instantiated on the UE. This module requests the ~DeviceApp to instantiate a
// ~MecRnisTestApp in the MEC system. The application is used to test and showcase the
// functionalities of the ~RNIService API.
// Once the peer MEC application is up and running, this module sends a message to the MEC
// application, requesting it to start querying the RNIS API periodically. When the
// response from the  ~MecRnisTestApp is received, the result of the query to the RNIS API
// is printed to the QtEnv log
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

