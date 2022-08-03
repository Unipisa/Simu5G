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
import simu5g.nodes.mec.MECPlatform.IMECApp;

//
//  This is a simple MEC app that receives the coordinates and the radius from the UE app
//  and subscribes to the CircleNotificationSubscription of the Location Service. The latter
//  periodically checks if the UE is inside/outside the area and sends a notification to the
//  MEC App. It then notifies the UE.

//
//  The event behavior flow of the app is:
//  1) receive coordinates from the UE app
//  2) subscribe to the circleNotificationSubscription
//  3) receive the notification
//  4) send the alert event to the UE app
//  5) (optional) receive stop from the UE app
//


simple MECWarningAlertApp like IMECApp, IApp
{
    parameters:
        @display("i=block/app;is=s");
  			
        int packetSize = default(10);
        string interfaceTableModule = default("");
        
        int mecAppIndex;
        int mecAppId;
        int localUePort;
        
        string mp1Address = default("");
        int mp1Port = default(10021);
        
		 //resource required info
        double requiredRam @unit("B");
        double requiredDisk @unit("B");
        double requiredCpu;

		// IApp parameters
		int timeToLive = default(-1); // if not -1, set the TTL (IPv4) or Hop Limit (IPv6) field of sent packets to this value
        int dscp = default(-1); // if not -1, set the DSCP (IPv4/IPv6) field of sent packets to this value
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
		
		bool logger = default(false);

    gates:
	  input mePlatformIn;
	  output mePlatformOut;
	  
	  input socketIn;
	  output socketOut;
                
}

