//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//


package simu5g.nodes;

import inet.applications.contract.IApp;
import inet.common.MessageDispatcher;
import simu5g.stack.ICellularNic;
import inet.common.packet.recorder.PcapRecorder;
import inet.linklayer.contract.IEthernetLayer; // for inet-4.6
import inet.linklayer.ethernet.contract.IEthernetLayer; // for inet-4.5
import inet.linklayer.contract.IEthernetInterface;
import inet.linklayer.contract.IIeee8021qLayer;
import inet.linklayer.contract.IMacForwardingTable;
import inet.protocolelement.contract.IProtocolLayer;
import inet.linklayer.contract.ILoopbackInterface;
import inet.mobility.contract.IMobility;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.contract.INetworkLayer;
import inet.transportlayer.contract.ISctp;
import inet.transportlayer.contract.ITcp;
import inet.transportlayer.contract.IUdp;
import simu5g.corenetwork.statsCollector.UeStatsCollector;


//
// Models a User Equipment (UE) in a 4G LTE network. It is a host equipped with
// an LTE Network Interface Card (NIC), allowing the UE to connect to an ~eNodeB module via the
// Radio Access Network (RAN) of the LTE network and communicate with any other host in the system.
// In addition to a NIC submodule, this compound module includes all the higher layers of the protocol stack,
// from IP to application modules. It supports a configurable number of UDP and TCP applications. It
// also features an Ethernet interface, which is utilized in real-time emulation mode to connect the UE
// to the real network (outside the simulator) via a virtual Ethernet interface.
//
// The UE has one session, whose type it requests when it registers with the core
// network (the PDU session type of the PDU Session Establishment Request in a 5G
// core, the PDN type in an EPC): the sessionType parameter. The network establishes
// the session with that type, or rejects it with an error if its core network cannot
// carry it. The IP types ("IPv4", "IPv6", "IPv4v6") are carried by both core
// networks; "Unstructured" and "Ethernet" need a 5G core.
//
// An Unstructured session carries opaque payload, from an application attached to
// the NIC directly (an ~UnstructuredApp in the unstructuredApp submodule vector, see
// numUnstructuredApps) to the server the anchor UPF's N6 end of the session is
// configured with (see ~UserPlaneNodeControl). The session has one QoS flow, the
// default one, on one bearer, the default DRB; it is not supported on a D2D-capable
// UE (hasD2D). Such a UE normally has no IP layer (hasIpv4 = false).
//
// An Ethernet session carries Ethernet frames between the UE and the anchor UPF's
// Ethernet bridge (see ~EthernetSessionBridge), which switches them among the UPF's Ethernet
// sessions and its N6 Ethernet LAN; the frames cross the session without their FCS
// (TS 23.501 5.6.10.2). The UE then has the link layer of an INET Ethernet node
// (EthernetLayer, Ieee8021qProtocol), whose Ethernet ports are the cellular NIC and the
// eth[] interfaces. With ethernetMode = "host", the UE is an Ethernet host: raw Ethernet
// applications (e.g. INET's EthernetSourceApp and EthernetSinkApp) send and receive with
// the NIC's MAC address. With ethernetMode = "bridge", the UE bridges (BridgingLayer)
// the NIC and its eth[] interfaces, whose ethg[] gates connect the devices behind it
// (numEthInterfaces). The QoS rules classify a frame by its headers (e.g. its 802.1Q
// priority); a dscpAsQfi rule does not apply to a frame. The session is not supported
// on a D2D-capable UE (hasD2D), and not with ROHC. Such a UE normally has no IP layer
// (hasIpv4 = false): nothing configures IP over the session automatically.
//
module LteUe
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
        int numApps = default(0);   // Number of apps
                                    // Set the app types in the ini file, e.g. app[0..1].typename="UdpVideoStreamClient"
        int numUnstructuredApps = default(0);  // Number of apps attached to the cellular NIC directly (0 or 1), for a UE whose session is of type Unstructured

        //# Transport layer
        bool hasUdp = default(firstAvailableOrEmpty("Udp") != "");
        bool hasTcp = default(firstAvailableOrEmpty("Tcp", "TcpLwip", "TcpNsc") != "");
        bool hasSctp = default(false);

        //# Node specs
        string nodeType = "UE";  // DO NOT CHANGE
        int servingNodeId = default(0); // The initial serving eNodeB of the UE; 0 if none
        int macNodeId = default(1025+simu5g_seq("LteUe_macNodeId"));
        string sessionType @enum("IPv4","IPv6","IPv4v6","Ethernet","Unstructured") = default("IPv4");  // The type of the session the UE requests (see the main comment)
        string ethernetMode @enum("host","bridge") = default("host");  // Ethernet sessions only: the UE is an Ethernet host, or a bridge for the devices on its eth[] interfaces (see the main comment)
        bool hasEthernetSession = sessionType == "Ethernet";  // Do not change: whether the UE has the Ethernet link layer (see the main comment)

        //# D2D support: selects the D2D-capable NIC variant (see the cellularNic submodule).
        //# An explicit cellularNic.typename in the ini file still overrides this.
        bool hasD2D = default(false);

        //# Network Layer specs
        bool hasIpv4 = default(true);
        bool hasIpv6 = default(false);
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        bridging.macTableModule = default(absPath(".macTable"));
        ethernet.registerProtocol = default(true);
        ethernet.*.promiscuous = default(ethernetMode == "bridge");
        ethernet.hasSocketSupport = default(true);
        *.routingTableModule = default("^.ipv4.routingTable");
        *.mobilityModule = default(absPath(".mobility"));

        //# External Ethernet interfaces
        int numEthInterfaces = default(0);
        string extHostAddress = default("");

        //# Loopback interfaces
        int numLoInterfaces = default(1);

        //# PcapRecorders
        int numPcapRecorders = default(0); // Number of PcapRecorders

        bool hasRniSupport = default(false);
        cellularNic.mac.collectorModule = hasRniSupport ? "^.^.ueCollector" : "";
        cellularNic.rrc.connectionControl.hasCollector = hasRniSupport;

    gates:
        input radioIn @directIn;     // Connection to master
        inout ethg[hasEthernetSession ? numEthInterfaces : 0] @labels(EtherFrame-conn);   // Ethernet session in bridge mode: the links to the devices behind the UE

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
        cellularNic: <default(hasD2D ? "LteNicUeD2D" : "LteNicUe")> like ICellularNic {
            nodeType = parent.nodeType;
            sessionType = parent.sessionType;
            @display("p=355,589");
        }

        //# ethernet interface used for communicating with external applications in emulation mode;
        //# with an Ethernet session in bridge mode, the links to the devices behind the UE
        eth[numEthInterfaces]: <default(hasEthernetSession ? "LayeredEthernetInterface" : "ExtLowerEthernetInterface")> like IEthernetInterface {
            parameters:
                @display("p=674,590,row,150;q=txQueue");
        }

        //# the link layer of an Ethernet session (as in INET's LinkLayerNodeBase): the
        //# cellular NIC and eth[] are its interfaces
        cb: MessageDispatcher if hasEthernetSession {
            @display("p=550,520;b=600,5,,,,1");
        }
        bridging: <default(ethernetMode == "bridge" ? "BridgingLayer" : "")> like IProtocolLayer if hasEthernetSession && typename != "" {
            @display("p=375,520");
        }
        macTable: <default("MacForwardingTable")> like IMacForwardingTable if hasEthernetSession && ethernetMode == "bridge" {
            @display("p=127,430;is=s");
        }
        bl: MessageDispatcher if hasEthernetSession {
            @display("p=550,545;b=600,5,,,,1");
        }
        ethernet: <default("EthernetLayer")> like IEthernetLayer if hasEthernetSession {
            @display("p=375,560");
        }
        ieee8021q: <default("Ieee8021qProtocol")> like IIeee8021qLayer if hasEthernetSession {
            @display("p=525,560");
        }
        li: MessageDispatcher if hasEthernetSession {
            @display("p=550,575;b=600,5,,,,1");
        }
        encap: <default(hasEthernetSession ? "" : "EthernetEncapsulation")> like IEthernetLayer if typename != "" {
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
        unstructuredApp[numUnstructuredApps]: <default("UnstructuredApp")> like IApp {
            @display("p=726,72,row,140");
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
        ueCollector: UeStatsCollector if hasRniSupport {
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

        // the payload of an Unstructured session goes to and from the NIC with no
        // transport or network layer in between
        for i=0..numUnstructuredApps-1 {
            unstructuredApp[i].socketOut --> nl.in++;
            unstructuredApp[i].socketIn <-- nl.out++;
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

        cellularNic.upperLayerOut --> nl.in++ if !hasEthernetSession;
        cellularNic.upperLayerIn <-- nl.out++ if !hasEthernetSession;

        for i=0..numEthInterfaces-1 {
            encap.lowerLayerOut --> eth[i].upperLayerIn if !hasEthernetSession;
            encap.lowerLayerIn <-- eth[i].upperLayerOut if !hasEthernetSession;
        }
        encap.upperLayerOut --> nl.in++ if numEthInterfaces != 0 && !hasEthernetSession;
        encap.upperLayerIn <-- nl.out++ if numEthInterfaces != 0 && !hasEthernetSession;

        //#
        //# The link layer of an Ethernet session: nl <-> cb <-> (bridging) <-> bl <->
        //# ethernet, ieee8021q <-> li <-> the cellular NIC and eth[]; raw Ethernet
        //# applications reach it through at <-> tn <-> nl
        //#
        if hasEthernetSession {
            at.out++ --> tn.in++;
            at.in++ <-- tn.out++;
            cb.out++ --> nl.in++;
            cb.in++ <-- nl.out++;
            cb.out++ --> bridging.upperLayerIn if exists(bridging);
            bridging.upperLayerOut --> cb.in++ if exists(bridging);
            bridging.lowerLayerOut --> bl.in++ if exists(bridging);
            bl.out++ --> bridging.lowerLayerIn if exists(bridging);
            cb.out++ --> bl.in++ if !exists(bridging);
            bl.out++ --> cb.in++ if !exists(bridging);
            bl.out++ --> ethernet.upperLayerIn;
            ethernet.upperLayerOut --> bl.in++;
            bl.out++ --> ieee8021q.upperLayerIn;
            ieee8021q.upperLayerOut --> bl.in++;
            bl.out++ --> li.in++;
            li.out++ --> bl.in++;
            ethernet.lowerLayerOut --> li.in++;
            li.out++ --> ethernet.lowerLayerIn;
            ieee8021q.lowerLayerOut --> li.in++;
            li.out++ --> ieee8021q.lowerLayerIn;
            cellularNic.upperLayerOut --> li.in++;
            cellularNic.upperLayerIn <-- li.out++;
        }
        for i=0..numEthInterfaces-1 {
            eth[i].upperLayerOut --> li.in++ if hasEthernetSession;
            eth[i].upperLayerIn <-- li.out++ if hasEthernetSession;
            eth[i].phys <--> ethg[i] if hasEthernetSession;
        }

        for i=0..numLoInterfaces-1 {
            nl.out++ --> lo[i].upperLayerIn;
            lo[i].upperLayerOut --> nl.in++;
        }

        //#
        //# Connections between LTE/NR NIC and the radio medium
        //#
        cellularNic.radioIn <-- radioIn;
}
