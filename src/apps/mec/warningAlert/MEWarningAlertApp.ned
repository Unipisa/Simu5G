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

package simu5g.apps.mec.warningAlert;

import simu5g.nodes.mec.MEPlatform.IMEApp;

// MEWarningAlertApp simple module in charge to:
//
// 1) handling WarningAlertPacket of type INFO_UEAPP by forwarding to the MEWarningAlertService
// 2) handling WarningAlertPacket of type INFO_MEAPP by adding source/destination info and forwarding to the VirtualisationManager

simple MEWarningAlertApp like IMEApp
{
    parameters:
        @display("i=block/app;is=s");
        
        string ueSimbolicAddress; 		// i.e. UE[x] or car[x]
        string meHostSimbolicAddress;	// i.e meHost.virtualisationInfrastructure
        
        int packetSize = default(10);
        
        string interfaceTableModule = default("");
           
    gates:
        input virtualisationInfrastructureIn;
        output virtualisationInfrastructureOut;
        
        input mePlatformIn;
        output mePlatformOut;
}

