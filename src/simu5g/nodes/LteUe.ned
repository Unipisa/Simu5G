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
import inet.linklayer.contract.IMacForwardingTable;
import inet.node.inet.StandardHost;
import simu5g.corenetwork.statsCollector.UeStatsCollector;
import simu5g.stack.ICellularNic;


//
// Models a User Equipment (UE) in a 4G LTE network. It is a host equipped with
// an LTE Network Interface Card (NIC), allowing the UE to connect to an ~eNodeB module via the
// Radio Access Network (RAN) of the LTE network and communicate with any other host in the system.
// The UE is an INET ~StandardHost with the cellular NIC as an additional network
// interface of its link layer, so it has all the higher layers of the protocol stack,
// from the link layer to the application modules, and the configuration options of
// ~StandardHost (numApps, hasUdp, hasTcp, hasIpv4, hasIpv6, numEthInterfaces, etc.).
// Its Ethernet interfaces (eth[]) are utilized in real-time emulation mode to connect
// the UE to the real network (outside the simulator) via a virtual Ethernet interface.
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
// (TS 23.501 5.6.10.2). The UE's link layer is then that of an INET Ethernet node
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
module LteUe extends StandardHost
{
    parameters:
        @display("i=device/pocketpc");

        //# Apps
        int numUnstructuredApps = default(0);  // Number of apps attached to the cellular NIC directly (0 or 1), for a UE whose session is of type Unstructured

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

        //# External Ethernet interfaces
        string extHostAddress = default("");

        bool hasRniSupport = default(false);
        cellularNic.mac.collectorModule = hasRniSupport ? "^.^.ueCollector" : "";
        cellularNic.rrc.connectionControl.hasCollector = hasRniSupport;

        mobility.typename = default("StationaryMobility");

        //# the link layer of an Ethernet session; otherwise only the eth[] interfaces of
        //# emulation mode have a link-layer protocol (EthernetEncapsulation)
        ethernet.typename = default(hasEthernetSession ? "EthernetLayer" : sizeof(ethg) > 0 ? "EthernetEncapsulation" : "");
        ethernet.hasSocketSupport = default(true);
        ethernet.*.promiscuous = default(ethernetMode == "bridge");
        ieee8021q.typename = default(hasEthernetSession ? "Ieee8021qProtocol" : "");
        bridging.typename = default(hasEthernetSession && ethernetMode == "bridge" ? "BridgingLayer" : "");
        bridging.macTableModule = default(absPath(".macTable"));
        eth[*].typename = default(hasEthernetSession ? "LayeredEthernetInterface" : "ExtLowerEthernetInterface");

    gates:
        input lteRadioIn @directIn;     // Connection to master

    submodules:
        cellularNic: <default(hasD2D ? "LteNicUeD2D" : "LteNicUe")> like ICellularNic {
            nodeType = parent.nodeType;
            sessionType = parent.sessionType;
            @display("p=600,975");
        }
        macTable: <default("MacForwardingTable")> like IMacForwardingTable if hasEthernetSession && ethernetMode == "bridge" {
            @display("p=125,880;is=s");
        }
        unstructuredApp[numUnstructuredApps]: <default("UnstructuredApp")> like IApp {
            @display("p=1125,75,row,150");
        }

        //# UeStatsCollector - for MEC
        ueCollector: UeStatsCollector if hasRniSupport {
            @display("p=125,960;is=s");
        }

    connections allowunconnected:
        // the payload of an Unstructured session goes to and from the NIC with no
        // transport or network layer in between
        for i=0..numUnstructuredApps-1 {
            unstructuredApp[i].socketOut --> nl.in++;
            unstructuredApp[i].socketIn <-- nl.out++;
        }

        cellularNic.upperLayerOut --> li.in++;
        cellularNic.upperLayerIn <-- li.out++;

        //#
        //# Connections between LTE/NR NIC and the radio medium
        //#
        cellularNic.radioIn <-- lteRadioIn;
}
