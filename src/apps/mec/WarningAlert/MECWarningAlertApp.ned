
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
// This is a simple MEC app that receives the coordinates and the radius from the UE app
// and subscribes to the CircleNotificationSubscription of the Location Service. The latter
// periodically checks if the UE is inside/outside the area and sends a notification to the
// MEC App. It then notifies the UE.
//
//  The event behavior flow of the app is:
//  1) receive coordinates from the UE app
//  2) subscribe to the circleNotificationSubscription
//  3) receive the notification
//  4) send the alert event to the UE app
//  5) (optional) receive stop from the UE app
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
