//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//
// 

package lte.nodes.mec.VirtualisationInfrastructureManager;


//
// VirtualisationInfrastructureManager
//	
//	Simple Module for handling the availability of virtualised resources on the MEC host.
//	From  ETSI GS MEC 003, it is responsable for allocating, managing and releasing virtualised 
//  resources of the virtualisation infrastructure;
// 

simple VirtualisationInfrastructureManager
{
    parameters:
        @display("i=block/process");
        
        int mp1Port = default(10021);
        string scheduling = default("segregation");
        
    gates:
        input virtualisationManagerIn;
        output virtualisationManagerOut;
}



