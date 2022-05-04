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

package simu5g.apps.mec.PlatooningApp;

import inet.applications.contract.IApp;
import simu5g.nodes.mec.MECPlatform.IMECApp;

//
//  This is a MEC app that ... TBC
//
simple MECPlatooningProducerApp like IMECApp, IApp
{
    parameters:
        @display("i=block/app;is=s");
  			
        int packetSize = default(10);
        string interfaceTableModule = default("");
        
        int mecAppId;
        int producerAppId;
        int localUePort = default(5900);
        int platooningConsumerAppsPort = default(5500);
        int platooningProducerAppsPort = default(8500);
        int platooningControllerAppsPort = default(6500);
        
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
	
		object federatedProducerApps = default([]);
		
		// to create controllers
		string lateralController = "Rajamani";
		string longitudinalController = "Rajamani";
		double lateralControlPeriod @unit(s) = 0.100s; 
		double lateralUpdatePositionPeriod @unit(s) = 0.070s; 
		double longitudinalControlPeriod @unit(s) = 0.100s; 
		double longitudinalUpdatePositionPeriod @unit(s) = 0.070s; 
		
		double heartbeatPeriod @unit(s) = 1s;
		
		bool adjustPosition = false;
		bool sendBulk = true;
		
		@signal[updateDiffs];
        @statistic[updateDiffs](title="difference LS response times"; unit="ms"; source="updateDiffs"; record=mean, stats, vector); 
    gates:
	  input mePlatformIn;
	  output mePlatformOut;
	  
	  input socketIn;
	  output socketOut;
                
}

