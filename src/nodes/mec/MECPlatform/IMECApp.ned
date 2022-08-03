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

package simu5g.nodes.mec.MECPlatform;


moduleinterface IMECApp
{
    parameters:
        @display("i=block/app");
        int mecAppId;
        int mecAppIndex;
        
        string mp1Address;
        int mp1Port;
        
        int localUePort;
        
	
		//resource required infos
        double requiredRam @unit("B");
        double requiredDisk @unit("B");
        double requiredCpu;
}



