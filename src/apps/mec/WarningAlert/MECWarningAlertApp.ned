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

import simu5g.apps.mec.MecApps.MecAppBase;

//
// This module is an application that is dynamically instantiated on a MEC host upon request
// from a ~UEWarningAlertApp via a ~DeviceApp.
// It receives requests from the UE to be notified when it enters a circular area defined by
// its center and radius. This is accomplished by subscribing to the ~LocationService provided
// by the MEC Host. This module receives a notification from the Location Service when
// the UE enters/exits the area and, in turn, sends a message to the UE.
//
simple MECWarningAlertApp extends MecAppBase
{
    parameters:
        @class(MECWarningAlertApp);
        @display("i=block/app;is=s");

        int packetSize @unit(B) = default(10B);

        int localUePort;

        bool logger = default(false);
}
