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
import simu5g.nodes.mec.MECPlatform.MECPlatform;
import simu5g.nodes.mec.MECPlatformManager.MecPlatformManager;
import simu5g.nodes.mec.VirtualisationInfrastructureManager.VirtualisationInfrastructureManager;
import simu5g.nodes.mec.VirtualisationInfrastructureManager.BackgroundApp;
import simu5g.nodes.Upf;
import inet.node.ethernet.Eth10G;
import inet.applications.contract.IApp;

// 
//# MEHost module
//
module MECHost
{
    parameters:
        @display("i=device/server2;bgb=1000,500");

        //# Node specs
        string nodeType = "MECHost";						// DO NOT CHANGE
        string gateway = default("upf");

        //#	ME Host Resources
        int maxMECApps = default(0);
        double maxRam @unit("B") = default(32GB);
        double maxDisk @unit("B") = default(100TB);
        double maxCpuSpeed = default(300000); 	 // CPU's clock speed in Million Instruction per Seconds (MIPS)

        int numBGMecApp = default(0);
        int numIndependentMecApp = default(0);
        
        //# List of Base Stations associated to the MEC host
        // This is a string of comma-separated values
        string bsList = default("");

    gates:

		//to local radio networks
        inout ppp[];

        //to external networks (internet)
        inout pppIf[];

    submodules:

        virtualisationInfrastructure: VirtualisationInfrastructure {
            parameters:
                @display("p=500,400");
        }

        mecPlatform: MECPlatform {
            parameters:
                @display("p=573.535,158.12999");
        }

        mecPlatformManager: MecPlatformManager {
            parameters:
                @display("p=865.94995,116.715;is=m");
        }

        vim: VirtualisationInfrastructureManager {
            parameters:
                @display("p=865.94995,224.64499;is=m");
        }

        // UPF module used to connect the MEC host to the 5G Core Network (either a BS or a UPF)
        upf_mec: Upf {
            parameters:
                @display("p=150,400");
                nodeType = "UPF_MEC";
        }

        bgApp[numBGMecApp]: BackgroundApp {
            @display("p=865.94995,326.3,row,140");
        }
        
        independentMecApp[numIndependentMecApp]: <> like IApp {
            @display("p=371.47998,156.875,row,140");
        }

    connections allowunconnected:

        for i=0..sizeof(ppp)-1 {
            ppp++ <--> upf_mec.pppg++;
        }

        for i=0..sizeof(pppIf)-1 {
            //#
            //# external connection to external networks
            //#
            virtualisationInfrastructure.ppp++ <--> pppIf++;
        }

        //#
        //# internal connection between GtpEndpoint and VirtualisationInfrastrcuture
        //#
        virtualisationInfrastructure.ppp++ <--> Eth10G <--> upf_mec.filterGate;

        for i=0..numIndependentMecApp-1 {
            independentMecApp[i].socketIn <-- virtualisationInfrastructure.independentMecAppOut[i];
            independentMecApp[i].socketOut --> virtualisationInfrastructure.independentMecAppIn[i];
        }

        for i=0..mecPlatform.numMecServices {
            virtualisationInfrastructure.mecPlatform++ <--> mecPlatform.virtInfr++;
        }
}
