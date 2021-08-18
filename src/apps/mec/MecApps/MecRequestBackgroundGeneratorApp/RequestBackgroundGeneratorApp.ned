//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//


package lte.apps.mec.MecApps.MecRequestBackgroundGeneratorApp;

import inet.applications.contract.IApp;
import lte.nodes.mec.MECPlatform.IMECApp;

//
// 
// 

simple MecRequestBackgroundGeneratorApp like IMECApp, IApp
{
    parameters:
        int numberOfApplications;
        bool allocateHostResources = default(false);
        
        int packetSize = default(10);
        string interfaceTableModule = default("");
        
        int mecAppId = default(0); // it will be override by getId() in initialize
        int localUePort = default(0); // not used in MecRequestBackgroundGeneratorApp
        
        string mp1Address = default("");
        int mp1Port = default(10021);
        
		 //resource required info
        double requiredRam @unit("B") = default(10MB);
        double requiredDisk @unit("B") = default(100MB);
        double requiredCpu = default(15000);

		// IApp parameters
		int timeToLive = default(-1); // if not -1, set the TTL (IPv4) or Hop Limit (IPv6) field of sent packets to this value
        int dscp = default(-1); // if not -1, set the DSCP (IPv4/IPv6) field of sent packets to this value
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
		
		bool logger = default(false);        
    gates:
        input socketIn @labels(TcpCommand/up);
        output socketOut @labels(TcpCommand/down);
        
	  	input mePlatformIn;
	 	output mePlatformOut;        
}


