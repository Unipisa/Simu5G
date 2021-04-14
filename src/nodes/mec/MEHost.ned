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

package simu5g.nodes.mec;

import simu5g.nodes.mec.VirtualisationInfrastructure.VirtualisationInfrastructure;
import simu5g.nodes.mec.MEPlatform.MEPlatform;

//lte tunneling endpoint to connect externally with an eNodeB
import simu5g.nodes.mec.GtpEndpoint.GtpEndpoint;
import inet.node.ethernet.Eth10G;

// 
//# MEHost module
//
module MEHost
{
    parameters:
        @display("i=device/mainframe;bgb=1000,500");

        //# Node specs
        string nodeType = "MEHost";						// DO NOT CHANGE
        string gateway = default("upf");
        
        //#	ME Host Resources
        int maxMEApps = default(0);
        double maxRam @unit("B") = default(32GB);
        double maxDisk @unit("B") = default(100TB);
        double maxCpu = default(1);						// percentage of available CPU

        //#connections
        string eNBList = default("");

    gates:

		//to local radio networks
        inout pppENB[];

        //to external networks (internet)
        inout ppp[];

    submodules:

        virtualisationInfrastructure: VirtualisationInfrastructure {
            parameters:
                @display("p=500,400");
        }

        mePlatform: MEPlatform {
            parameters:
                @display("p=500,50;is=m");
        }

        //gtp endpoint to connect directly with a e/gNodeB
        gtpEndpoint: GtpEndpoint {
            parameters:
                @display("p=150,400;is=l");
        }

    connections allowunconnected:

        //ppp++ <--> virtualisationInfrastructure.ppp++;

        //#
        //# internal connection between GtpEndpoint and VirtualisationInfrastrcuture
        //#
        virtualisationInfrastructure.ppp++ <--> Eth10G <--> gtpEndpoint.filterGate;

        for i=0..sizeof(pppENB)-1 
        {
            pppENB++ <--> gtpEndpoint.pppg++;
        }

        for i=0..sizeof(ppp)-1 
        {
            //#
            //# external connection to external networks
            //#
            virtualisationInfrastructure.ppp++ <--> ppp++;
        }
}
