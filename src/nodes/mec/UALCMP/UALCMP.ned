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

package simu5g.nodes.mec.UALCMP;
import simu5g.nodes.mec.UALCMP.UALCMPApp;
import inet.linklayer.ppp.PppInterface;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.ipv4.Ipv4RoutingTable;
import inet.networklayer.contract.IRoutingTable;
import inet.networklayer.contract.INetworkLayer;
import inet.applications.contract.IApp;
import inet.transportlayer.udp.Udp;
import inet.transportlayer.tcp.Tcp;
import inet.common.MessageDispatcher;
import inet.linklayer.contract.IEthernetInterface;
import inet.applications.contract.IApp;
import inet.node.ethernet.Eth10G;
import inet.networklayer.common.InterfaceTable;


module UALCMP
{
    parameters:
        @networkNode();
        @display("i=device/mainframe;bgb=1006.50995,692.76");

        //# Node specs
        string nodeType = "MECOrchestrator";						// DO NOT CHANGE
        string gateway = default("upf");
        
        // hostname of the reference orchestrator                
        string mecOrchestratorHostname = default("mecOrchestrator");
        
        bool hasIpv4 = default(true);
        bool hasIpv6 = default(false);
        *.interfaceTableModule = default(absPath(".interfaceTable"));
//        *.routingTableModule = default("^.ipv4.routingTable");

        int numExtEthInterfaces = default(0); //

    gates:

        //to external networks (internet)
        inout ppp[];

        output toMecOrchestrator;
        input fromMecOrchestrator;			// connection to the ME App output gate

    submodules:

        interfaceTable: InterfaceTable {
            @display("p=61.344,74.976;is=s");
        }

        //#
        //# ppp modules
        //#
        //        pppIf[sizeof(ppp)]: PppInterface {
        //            @display("p=550,508");
        //        }
        pppIf: PppInterface {
            @display("p=446.832,506.688");
        }

        // ethernet interface used for communicating with external applications in emulation mode
        extEth[numExtEthInterfaces]: <default("ExtLowerEthernetInterface")> like IEthernetInterface {
            parameters:
                @display("p=641.712,506.688,row,150;q=txQueue");
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
        ualcmpApp: UALCMPApp {
            @display("p=549.69,74.045,row");
        }

        udp: Udp {
            @display("p=459,217");
        }

        tcp: Tcp {
            @display("p=627,217");
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
        //# UALCMP to transport layer connections
        //#        
        ualcmpApp.socketOut --> at.in++;
        ualcmpApp.socketIn <-- at.out++;

        ualcmpApp.toMecOrchestrator --> toMecOrchestrator;
        ualcmpApp.fromMecOrchestrator <-- fromMecOrchestrator;

        at.out++ --> udp.appIn;
        at.in++ <-- udp.appOut;

        at.out++ --> tcp.appIn;
        at.in++ <-- tcp.appOut;


        //# 
        //# Transport layer to network layer connections
        //#
        udp.ipOut --> tn.in++;
        udp.ipIn <-- tn.out++;

        tcp.ipOut --> tn.in++;
        tcp.ipIn <-- tn.out++;


        ipv4.ifIn <-- nl.out++ if hasIpv4;
        ipv4.ifOut --> nl.in++ if hasIpv4;

        ipv4.transportIn <-- tn.out++ if hasIpv4;
        ipv4.transportOut --> tn.in++ if hasIpv4;

        ipv6.ifIn <-- nl.out++ if hasIpv6;
        ipv6.ifOut --> nl.in++ if hasIpv6;

        ipv6.transportIn <-- tn.out++ if hasIpv6;
        ipv6.transportOut --> tn.in++ if hasIpv6;
        //
        //        for i=0..sizeof(ppp)-1 {
        //            pppIf[i].upperLayerOut --> nl.in++;
        //            pppIf[i].upperLayerIn <-- nl.out++;
        //            pppIf[i].phys <--> gtpEndpoint.pppg++;
        //        }
        
        pppIf.upperLayerOut --> nl.in++;
        pppIf.upperLayerIn <-- nl.out++;
        pppIf.phys <--> ppp++;

        for i=0..numExtEthInterfaces-1 {
            extEth[i].upperLayerOut --> nl.in++;
            extEth[i].upperLayerIn <-- nl.out++;
        }




}
