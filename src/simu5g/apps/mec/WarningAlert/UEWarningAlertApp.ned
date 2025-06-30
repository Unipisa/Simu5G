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


package simu5g.apps.mec.WarningAlert;

import inet.applications.contract.IApp;


//
// This application is to be instantiated on the UE side. It requires that a ~DeviceApp is
// also instantiated on the UE. This module requests the ~DeviceApp to instantiate a
// ~MECWarningAlertApp in the MEC system.
// Once the peer MEC application is up and running, this module sends a message to the MEC
// application, requesting that it be notified when the UE running this module enters
// a circular area defined by its (configurable) center and radius.
//
simple UEWarningAlertApp like IApp
{
    parameters:
        @display("i=block/source");

        string mobilityModule;

        //connection infos
        int localPort = default(4000);
        int deviceAppPort = default(4000); // ipAddress of the DeviceApp
        string deviceAppAddress; // port of the DeviceApp

        int packetSize @unit(B) = default(10B);

        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value

        //autoscheduling infos
        double period @unit("s") = default(1s);
        double startTime @unit("s") = default(0s);
        double stopTime @unit("s") = default(120s);

        string mecAppName = default("MECWarningAlertApp"); // but could be MEWarningAlertApp_rest_External

        double positionX @unit(m) = default(210m);
        double positionY @unit(m) = default(260m);
        double positionZ @unit(m) = default(0m);
        double radius @unit(m) = default(60m);

        bool logger = default(false);

    gates:
        output socketOut;
        input socketIn;
}

