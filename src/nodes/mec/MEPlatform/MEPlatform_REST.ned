//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

//
//	@author	Alessandro Noferi
//
package lte.nodes.mec.MEPlatform;
import inet.common.MessageDispatcher;

import inet.linklayer.contract.ILoopbackInterface;


import inet.transportlayer.contract.ITcp;
import inet.transportlayer.contract.IUdp;

import inet.linklayer.contract.IEthernetInterface;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.ipv4.Ipv4RoutingTable;
import inet.networklayer.contract.IRoutingTable;
import inet.networklayer.contract.INetworkLayer;
import inet.applications.contract.IApp;
//import inet.applications.contract.ITCPApp;
import inet.transportlayer.udp.Udp;
import inet.transportlayer.tcp.Tcp;
import lte.nodes.mec.MEPlatform.IMEService;

// 
//# VirtualisationInfrastructure
//

module MEPlatform_REST
{
    parameters:
        @networkNode();

        //# Network Layer specs
        string networkLayerType = default("IPv4NetworkLayer");
        string routingTableType = default("IPv4RoutingTable");
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        *.routingTableModule = default(routingTableType != "" ? absPath(".routingTable") : "");
        //# Num Services
        int numServices = default(0);

        @display("bgb=1122.99,610.08");

         //# Network Layer specs
        bool hasIpv4 = default(true);
        bool hasIpv6 = default(false);

        //# External Ethernet interfaces
        int numExtEthInterfaces = default(1); //
//        string extHostAddress = default("");

        //# Loopback interfaces
        int numLoInterfaces = default(1);

    gates:
        inout virtInfr; // connection to the virtualization infrastructure to communicate with the MEapps

    submodules:

        interfaceTable: InterfaceTable {
            @display("p=61.344,74.976;is=s");
        }

        // TODO: should not include a routing table
        // routing table
        routingTable: <routingTableType> like IRoutingTable if routingTableType != "" {
            parameters:
                @display("p=61.344,124.96;is=s");
        }

		//# app layer 
        app[numServices]: <> like IApp {
            @display("p=551.04,73.8,row,140");
        }

        //# transport layer 
        udp: <default(firstAvailableOrEmpty("Udp"))> like IUdp if hasUdp {
            parameters:
                @display("p=375,226");
        }
        tcp: <default(firstAvailableOrEmpty("Tcp", "TcpLwip", "TcpNsc"))> like ITcp if hasTcp {
            parameters:
                @display("p=676.5,225.09");
        }

        //# network layer
        ipv4: <default("Ipv4NetworkLayer")> like INetworkLayer if hasIpv4 {
            parameters:
                @display("p=389.91,375.15;q=queue");
        }
        ipv6: <default("Ipv6NetworkLayer")> like INetworkLayer if hasIpv6 {
            parameters:
                @display("p=709.71,375.15;q=queue");
        }

        //# link layer 
        lo[numLoInterfaces]: <default("LoopbackInterface")> like ILoopbackInterface {
            parameters:
                @display("p=373.92,521.52,row,150");
        }

        // ethernet interface used for communicating with external applications in emulation mode
        extEth[numExtEthInterfaces]: <default("ExtLowerEthernetInterface")> like IEthernetInterface {
            parameters:
                @display("p=549.81,530.13,row,150;q=txQueue");
        }

         // ethernet interface used for communicating with interal MEC applications in the virtualization infrastructure
        eth: <default("EthernetInterface")> like IEthernetInterface {
            parameters:
                @display("p=739.23,521.52,row,150;q=txQueue");
        }


        //# message dispatcher for SAP between application and transport layer
        at: MessageDispatcher {
            parameters:
                @display("p=550,146;b=600,5,,,,1");
        }
        //# message dispatcher for SAP between transport and network layer
        tn: MessageDispatcher {
            parameters:
                @display("p=550,296;b=600,5,,,,1");
        }
        //# message dispatcher for SAP to link layer
        nl: MessageDispatcher {
            parameters:
                @display("p=550,446;b=600,5,,,,1");
        }

    connections allowunconnected:

	  //# 
        //# Internal Tcp/Udp applications connections with transport layer
        //#

        for i=0..numApps-1 {
            app[i].socketOut --> at.in++;
            app[i].socketIn <-- at.out++;
        }

        at.out++ --> udp.appIn if hasUdp;
        at.in++ <-- udp.appOut if hasUdp;

        at.out++ --> tcp.appIn if hasTcp;
        at.in++ <-- tcp.appOut if hasTcp;

        //# 
		//# Internal transport layer connections to network layer
		//# 

        udp.ipOut --> tn.in++ if hasUdp;
        udp.ipIn <-- tn.out++ if hasUdp;

        tcp.ipOut --> tn.in++ if hasTcp;
        tcp.ipIn <-- tn.out++ if hasTcp;

        ipv4.ifIn <-- nl.out++ if hasIpv4;
        ipv4.ifOut --> nl.in++ if hasIpv4;

        ipv4.transportIn <-- tn.out++ if hasIpv4;
        ipv4.transportOut --> tn.in++ if hasIpv4;

        ipv6.ifIn <-- nl.out++ if hasIpv6;
        ipv6.ifOut --> nl.in++ if hasIpv6;

        ipv6.transportIn <-- tn.out++ if hasIpv6;
        ipv6.transportOut --> tn.in++ if hasIpv6;

        tn.out++ --> nl.in++;
        tn.in++ <-- nl.out++;

        //#
        //# Internal link layer connections to network layer  
        //#

        for i=0..numExtEthInterfaces-1 {
            extEth[i].upperLayerOut --> nl.in++;
            extEth[i].upperLayerIn <-- nl.out++;
        }

        for i=0..numLoInterfaces-1 {
            nl.out++ --> lo[i].upperLayerIn;
            lo[i].upperLayerOut --> nl.in++;
        }

        eth.upperLayerOut --> nl.in++;
        eth.upperLayerIn <-- nl.out++;
        eth.phys <--> virtInfr;
       
}
