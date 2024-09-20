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


package simu5g.nodes;

import inet.applications.contract.IApp;
import inet.common.MessageDispatcher;
import inet.common.packet.recorder.PcapRecorder;
import inet.linklayer.contract.IEthernetInterface;
import inet.linklayer.contract.ILoopbackInterface;
import inet.linklayer.ethernet.contract.IEthernetLayer;
import inet.mobility.contract.IMobility;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.contract.INetworkLayer;
import inet.transportlayer.contract.ISctp;
import inet.transportlayer.contract.ITcp;
import inet.transportlayer.contract.IUdp;
import simu5g.corenetwork.statsCollector.UeStatsCollector;
import simu5g.stack.ICellularNic;


//
// This module models a User Equipment (UE) in a 4G LTE network. It is a host equipped with
// an LTE Network Interface Card (NIC), allowing the UE to connect to an ~eNodeB module via the
// Radio Access Network (RAN) of the LTE network and communicate with any other host in the system.
// In addition to a NIC submodule, this compound module includes all the higher layers of the protocol stack,
// from IP to application modules. It supports a configurable number of UDP and TCP applications. It
// also features an Ethernet interface, which is utilized in real-time emulation mode to connect the UE
// to the real network (outside the simulator) via a virtual Ethernet interface.
//
module Ue
{
    parameters:
        @networkNode();
        @display("i=device/pocketpc;bgb=858,659");
        @figure[applicationLayer](type=rectangle; pos=250,6; size=600,130; lineColor=#808080; cornerRadius=5; fillColor=#ffff00; fillOpacity=0.1);
        @figure[applicationLayer.title](type=text; pos=845,11; anchor=ne; text="application layer");
        @figure[transportLayer](type=rectangle; pos=250,156; size=600,130; fillColor=#ff0000; lineColor=#808080; cornerRadius=5; fillOpacity=0.1);
        @figure[transportLayer.title](type=text; pos=845,161; anchor=ne; text="transport layer");
        @figure[networkLayer](type=rectangle; pos=250,306; size=600,130; fillColor=#00ff00; lineColor=#808080; cornerRadius=5; fillOpacity=0.1);
        @figure[networkLayer.title](type=text; pos=845,311; anchor=ne; text="network layer");
        @figure[linkLayer](type=rectangle; pos=250,456; size=600,130; fillColor=#0000ff; lineColor=#808080; cornerRadius=5; fillOpacity=0.1);
        @figure[linkLayer.title](type=text; pos=845,461; anchor=ne; text="link layer");
        @figure[submodules];

        //# Apps
        int numApps = default(0);   // no of apps.
                                    // Specify the app types in INI file with app[0..1].typename="UdpVideoStreamClient" syntax

        //# Transport layer
        bool hasUdp = default(firstAvailableOrEmpty("Udp") != "");
        bool hasTcp = default(firstAvailableOrEmpty("Tcp", "TcpLwip", "TcpNsc") != "");
        bool hasSctp = default(false);

        //# Node specs
        string nodeType = "UE";  // DO NOT CHANGE
        int masterId @mutable;
        int macNodeId @mutable = default(0); // TODO: this is not a real parameter
        int macCellId @mutable = default(0); // TODO: this is not a real parameter

        //# Network Layer specs
        bool hasIpv4 = default(true);
        bool hasIpv6 = default(false);
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        *.routingTableModule = default("^.ipv4.routingTable");
        *.mobilityModule = default(absPath(".mobility"));

        //# External Ethernet interfaces
        int numEthInterfaces = default(0);
        string extHostAddress = default("");

        //# Loopback interfaces
        int numLoInterfaces = default(1);

        //# Num of PcapRecorders
        int numPcapRecorders = default(0); // no of PcapRecorders

        bool hasRNISupport = default(false);
        cellularNic.mac.collectorModule = hasRNISupport ? "^.^.ueCollector" : "";
        cellularNic.phy.hasCollector = hasRNISupport;

    gates:
        input radioIn @directIn;     // connection to master

    submodules:
        interfaceTable: InterfaceTable {
            @display("p=127,257;is=s");
        }
        mobility: <default("StationaryMobility")> like IMobility {
            @display("p=127,172;is=s");
        }

        //# layer submodules (must be ordered from lower to upper layers)

        //# link layer
        lo[numLoInterfaces]: <default("LoopbackInterface")> like ILoopbackInterface {
            parameters:
                @display("p=503,590,row,150");
        }
        cellularNic: <default("LteNicUe")> like ICellularNic {
            nodeType = parent.nodeType;
            @display("p=355,589");
        }

        //# ethernet interface used for communicating with external applications in emulation mode
        eth[numEthInterfaces]: <default("ExtLowerEthernetInterface")> like IEthernetInterface {
            parameters:
                @display("p=674,590,row,150;q=txQueue");
        }
        encap: <default("EthernetEncapsulation")> like IEthernetLayer if typename != "" {
            parameters:
                registerProtocol = true;
                @display("p=674,500");
        }

        //# network layer
        ipv4: <default("Ipv4NetworkLayer")> like INetworkLayer if hasIpv4 {
            parameters:
                @display("p=375,376;q=queue");
        }
        ipv6: <default("Ipv6NetworkLayer")> like INetworkLayer if hasIpv6 {
            parameters:
                @display("p=525,376;q=queue");
        }

        //# transport layer
        udp: <default(firstAvailableOrEmpty("Udp"))> like IUdp if hasUdp {
            parameters:
                @display("p=375,226");
        }
        tcp: <default(firstAvailableOrEmpty("Tcp", "TcpLwip", "TcpNsc"))> like ITcp if hasTcp {
            parameters:
                @display("p=525,226");
        }
        sctp: <default(firstAvailableOrEmpty("Sctp"))> like ISctp if hasSctp {
            parameters:
                @display("p=675,226");
        }

        //# app layer
        app[numApps]: <> like IApp {
            @display("p=374,72,row,140");
        }

        //# message dispatcher for SAP between application and transport layer
        at: MessageDispatcher {
            parameters:
                @display("p=550,146;b=600,5,,,,1");
        }
        //# message dispatcher for SAP between transport and network layer
        tn: MessageDispatcher {
            parameters:
                @display("p=550,300;b=600,5,,,,1");
        }
        //# message dispatcher for SAP to link layer
        nl: MessageDispatcher {
            parameters:
                @display("p=550,446;b=600,5,,,,1");
        }
        //# PcapRecorders
        pcapRecorder[numPcapRecorders]: PcapRecorder {
            @display("p=126,346,r,10");
        }

        //# UeStatsCollector - for MEC
        ueCollector: UeStatsCollector if hasRNISupport {
            @display("p=126,588;is=s");
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

        at.out++ --> sctp.appIn if hasSctp;
        at.in++ <-- sctp.appOut if hasSctp;

        //#
		//# Internal transport layer connections to network layer
		//#

        udp.ipOut --> tn.in++ if hasUdp;
        udp.ipIn <-- tn.out++ if hasUdp;

        tcp.ipOut --> tn.in++ if hasTcp;
        tcp.ipIn <-- tn.out++ if hasTcp;

        sctp.ipOut --> tn.in++ if hasSctp;
        tn.out++ --> sctp.ipIn if hasSctp;

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

        cellularNic.upperLayerOut --> nl.in++;
        cellularNic.upperLayerIn <-- nl.out++;

        for i=0..numEthInterfaces-1 {
            encap.lowerLayerOut --> eth[i].upperLayerIn;
            encap.lowerLayerIn <-- eth[i].upperLayerOut;
        }
        encap.upperLayerOut --> nl.in++ if numEthInterfaces != 0;
        encap.upperLayerIn <-- nl.out++ if numEthInterfaces != 0;

        for i=0..numLoInterfaces-1 {
            nl.out++ --> lo[i].upperLayerIn;
            lo[i].upperLayerOut --> nl.in++;
        }

        //#
        //# Connections between LTE/NR NIC and the radio medium
        //#
        cellularNic.radioIn <-- radioIn;
}
