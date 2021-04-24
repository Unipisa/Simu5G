//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

package lte.apps.mec.RNIDelay;

import lte.nodes.mec.MEPlatform.IMEApp;

// MEWarningAlertApp simple module in charge to:
//
// TODO

simple MECRniDelayApp like IMEApp
{
    parameters:
        @display("i=block/app;is=s");
        
        string ueSimbolicAddress; 		// i.e. UE[x] or car[x]
        int uePort;
        string localAddress;	// i.e meHost.virtualisationInfrastructure
		int localPort;					// i.e port where the MeApp is binded
        int packetSize = default(10);
        
        double getPeriod @unit("s") = default(100ms);
        double threshold @unit("s") = default(6ms);
        int thresholdCount = default(2);
        
         string uri = default("/example/rni/v2/queries/layer2_meas");
        
        string interfaceTableModule = default("");
              
       	//resource required infos
        int requiredRam @unit("B") = default(10MB);
        int requiredDisk @unit("B") = default(100MB);
        double requiredCpu = default(0.01);
        
        
        
        
           
    gates:
        input socketIn; 
        output socketOut; 
        
        input mePlatformIn;
	    output mePlatformOut;
        
}

