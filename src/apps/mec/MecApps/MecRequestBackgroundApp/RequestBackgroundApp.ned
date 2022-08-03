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

package simu5g.apps.mec.MecApps.MecRequestBackgroundApp;

import inet.applications.contract.IApp;
import simu5g.nodes.mec.MECPlatform.IMECApp;

simple MecRequestBackgroundApp like IMECApp, IApp
{
    parameters:

        int numberOfApplications = default(1);
        bool allocateHostResources = default(false);
        
        int packetSize = default(10);
        string interfaceTableModule = default("");
        
        int mecAppIndex = default(0); 
        int mecAppId = default(0); // it will be override by getId() in initialize
        int localUePort = default(0); // not used in MecRequestBackgroundGeneratorApp
        
        string mp1Address = default("");
        int mp1Port = default(10021);
        
        double lambda @unit(s) = default(10ms);
        
		 //resource required info
        double requiredRam @unit("B") = default(10MB);
        double requiredDisk @unit("B") = default(100MB);
        double requiredCpu = default(15000);

		// IApp parameters
		int timeToLive = default(-1); // if not -1, set the TTL (IPv4) or Hop Limit (IPv6) field of sent packets to this value
        int dscp = default(-1); // if not -1, set the DSCP (IPv4/IPv6) field of sent packets to this value
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
		
		bool logger = default(false);    
		
        string localAddress = default(""); // may be left empty ("")
        int localPort = default(-1); // port number to listen on
        string connectAddress = default("");  // server address (may be symbolic)
        int connectPort = default(1000); // port number to connect to
        double startTime @unit(s) = default(1s); // time first session begins
        double stopTime @unit(s) = default(-1s);  // time of finishing sending, negative values mean forever

        @display("i=block/app");
        @lifecycleSupport;
        double stopOperationExtraTime @unit(s) = default(-1s);    // extra time after lifecycle stop operation finished
        double stopOperationTimeout @unit(s) = default(2s);    // timeout value for lifecycle stop operation
       
 
    gates:
        input socketIn @labels(TcpCommand/up);
        output socketOut @labels(TcpCommand/down);
        
        	  input mePlatformIn;
	  output mePlatformOut;
        
}