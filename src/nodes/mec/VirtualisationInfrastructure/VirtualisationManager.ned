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

package simu5g.nodes.mec.VirtualisationInfrastructure;

import inet.applications.contract.IApp;

//
//# VirtualisationManager
//	
//	Simple Module for handling all defined UE/ME App Packets!
//
//	The task of this module is:
//		1) to Instantiate a MEAppS according to the StartPacket received & passing the UE destAddress
//		2) mantaining the Data Structure about ALL ME Apps Instantiated!
//		3) to Handle Table mapping UE-IPAddresses & v2vAppName with the CORRESPONDED GATE connected with the MEApp
//		4) forwarding UE/ME App Packets to the right MEApp according to the Table
//

simple VirtualisationManager like IApp{

    parameters:
        @display("i=block/source");
        
        int localPort = default(4000);			// ALL UEApps have to send UDP Packets to this Port!
        int destPort = default(4000);
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
        
        string interfaceTableModule;
                
    gates:
        
        output socketOut;
        input socketIn;
        output meAppOut[];
        input meAppIn[];
        output resourceManagerOut;
        input resourceManagerIn;
}

