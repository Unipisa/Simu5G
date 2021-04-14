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

package simu5g.nodes.mec.MEPlatform;

//
// Template for ME Services.
//

moduleinterface IMEApp
{
    parameters:
        @display("i=block/app");
        
        string ueSimbolicAddress; 		// i.e. UE[x] or car[x]
        string meHostSimbolicAddress;	// i.e meHost.virtualisationInfrastructure
        
    gates:
        
        input virtualisationInfrastructureIn;
        output virtualisationInfrastructureOut;
        
        input mePlatformIn;
        output mePlatformOut;
}



