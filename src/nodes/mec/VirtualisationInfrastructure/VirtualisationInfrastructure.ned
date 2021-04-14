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

import inet.linklayer.ppp.PppInterface;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.ipv4.Ipv4RoutingTable;
import inet.networklayer.contract.IRoutingTable;
import inet.networklayer.contract.INetworkLayer;
import inet.applications.contract.IApp;
import inet.transportlayer.udp.Udp;
import inet.common.MessageDispatcher;
import simu5g.nodes.mec.VirtualisationInfrastructure.VirtualisationManager;


// 
//# VirtualisationInfrastructure
//

module VirtualisationInfrastructure
{
    parameters:
        @networkNode();
        @figure[applicationLayer](type=rectangle; pos=250,6; size=600,130; lineColor=#808080; cornerRadius=5; fillColor=#ffff00; fillOpacity=0.1);
        @figure[applicationLayer.title](type=text; pos=845,11; anchor=ne; text="application layer");
        @figure[transportLayer](type=rectangle; pos=250,156; size=600,130; fillColor=#ff0000; lineColor=#808080; cornerRadius=5; fillOpacity=0.1);
        @figure[transportLayer.title](type=text; pos=845,161; anchor=ne; text="transport layer");
        @figure[networkLayer](type=rectangle; pos=250,306; size=600,130; fillColor=#00ff00; lineColor=#808080; cornerRadius=5; fillOpacity=0.1);
        @figure[networkLayer.title](type=text; pos=845,311; anchor=ne; text="network layer");
        @figure[linkLayer](type=rectangle; pos=250,456; size=600,130; fillColor=#0000ff; lineColor=#808080; cornerRadius=5; fillOpacity=0.1);
        @figure[linkLayer.title](type=text; pos=845,461; anchor=ne; text="link layer");
        @figure[submodules];

        //# Network Layer specs
        bool hasIpv4 = default(true);
        bool hasIpv6 = default(false);
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        *.routingTableModule = default("^.ipv4.routingTable");

    gates:

        inout ppp[];				// connection with GtpEndpoint and local/externa networks
        output meAppOut[];			// connection to the ME App input gate
        input meAppIn[];			// connection to the ME App output gate

    submodules:

        interfaceTable: InterfaceTable {
            @display("p=61.344,74.976;is=s");
        }

//        // TODO: should not include a routing table
//        // routing table
//        routingTable: <routingTableType> like IRoutingTable if routingTableType != "" {
//            parameters:
//                @display("p=61.344,124.96;is=s");
//        }

        //#
        //# ppp modules
        //#
        pppIf[sizeof(ppp)]: PppInterface {
            @display("p=550,508");
        }

        //#
        //# Network layer module
        //#
        ipv4: <default("Ipv4NetworkLayer")> like INetworkLayer if hasIpv4 {
            parameters:
                @display("p=420.28,372.88;q=queue");
        }
        ipv6: <default("Ipv6NetworkLayer")> like INetworkLayer if hasIpv6 {
            parameters:
                @display("p=657.28,374.46;q=queue");
        }

        //#
        //# Transport layer modules and corresponding application modules
        //#
		// =============== UDP ============== //
        virtualisationManager: VirtualisationManager {
            @display("p=550,74,row");
        }

        udp: Udp {
            @display("p=550,217");
        }

        //#
        //# Resource Manager module
        //#
        resourceManager: ResourceManager {
            @display("p=722,74,row");
        }

                // message dispatcher for SAP between application and transport layer
        at: MessageDispatcher {
            parameters:
                @display("p=550,146;b=600,5,,,,1");
        }
        // message dispatcher for SAP between transport and network layer
        tn: MessageDispatcher {
            parameters:
                @display("p=550,296;b=600,5,,,,1");
        }
        // message dispatcher for SAP to link layer
        nl: MessageDispatcher {
            parameters:
                @display("p=550,446;b=600,5,,,,1");
        }

    connections allowunconnected:


        //# 
        //# VirtualisationManager to ResourceManager connections
        //#
        virtualisationManager.resourceManagerIn <-- resourceManager.virtualisationManagerOut;
        virtualisationManager.resourceManagerOut --> resourceManager.virtualisationManagerIn;


        //# 
        //# VirtualisationManager to transport layer connections
        //#        
        virtualisationManager.socketOut --> at.in++;
        virtualisationManager.socketIn <-- at.out++;

        at.out++ --> udp.appIn;
        at.in++ <-- udp.appOut;

        //# 
        //# Transport layer to network layer connections
        //#
        udp.ipOut --> tn.in++;
        udp.ipIn <-- tn.out++;

        ipv4.ifIn <-- nl.out++ if hasIpv4;
        ipv4.ifOut --> nl.in++ if hasIpv4;

        ipv4.transportIn <-- tn.out++ if hasIpv4;
        ipv4.transportOut --> tn.in++ if hasIpv4;

        ipv6.ifIn <-- nl.out++ if hasIpv6;
        ipv6.ifOut --> nl.in++ if hasIpv6;

        ipv6.transportIn <-- tn.out++ if hasIpv6;
        ipv6.transportOut --> tn.in++ if hasIpv6;

        for i=0..sizeof(ppp)-1 {
            pppIf[i].upperLayerOut --> nl.in++;
            pppIf[i].upperLayerIn <-- nl.out++;
            pppIf[i].phys <--> ppp++;
        }

}
