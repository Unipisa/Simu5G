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

import inet.common.MessageDispatcher;
import inet.linklayer.ppp.PppInterface;
import inet.transportlayer.udp.Udp;
import inet.node.base.NetworkLayerNodeBase;
import simu5g.corenetwork.gtp.GtpUser;
import simu5g.corenetwork.trafficFlowFilter.TrafficFlowFilter;

//
// This module implements a simplified model of the EPC PDN Gateway (P-GW) of a 4G LTE network.
// It serves as the entry point to the core network of an LTE network, thus bridging it to the rest
// of the Internet. Packets arriving from the Internet are tunneled to the appropriate next hop in
// the core network (e.g., the ~eNodeB that will transmit the packet to the destination User Equipment
// through the radio access network) using the GPRS Tunneling Protocol (GTP). Conversely, packets
// received from the core network are detunneled and sent to the Internet (e.g., towards a remote server).
// To accomplish this, it utilizes a ~GtpUser module that handles GTP (de)tunneling, along with the
// ~TrafficFlowFilter module, which classifies incoming packets.
//
module PgwStandard extends NetworkLayerNodeBase
{
    parameters:
        @display("bgb=1350,750;i=device/mainframe");
        @figure[applicationLayer](type=rectangle; pos=250,6; size=1000,130; lineColor=#808080; cornerRadius=5; fillColor=#ffff00; fillOpacity=0.1);
        @figure[applicationLayer.title](type=text; pos=1245,11; anchor=ne; text="application layer");
        @figure[transportLayer](type=rectangle; pos=250,156; size=1000,130; fillColor=#ff0000; lineColor=#808080; cornerRadius=5; fillOpacity=0.1);
        @figure[transportLayer.title](type=text; pos=1245,161; anchor=ne; text="transport layer");
        @figure[submodules];

        string nodeType = "PGW";

        forwarding = default(true);
        multicastForwarding = default(false);
        // numPppInterfaces = 1;

    gates:
        inout filterGate @labels(PppFrame-conn);

    submodules:
        pppIf: PppInterface {
            parameters:
                @display("p=1150,525");
        }
        udp: Udp {
            parameters:
                @display("p=330,206");
        }
        trafficFlowFilter: TrafficFlowFilter {
            parameters:
                @display("p=675,80");
                ownerType = parent.nodeType;
        }
        gtp_user: GtpUser {
            parameters:
                @display("p=525,80");
        }
        tn: MessageDispatcher {
            parameters:
                @display("p=750,300;b=1000,5,,,,1");
        }
        at: MessageDispatcher {
            parameters:
                @display("p=750,146;b=1000,5,,,,1");
        }

    connections:

        gtp_user.socketOut --> at.in++;
        gtp_user.socketIn <-- at.out++;

        pppIf.upperLayerOut --> trafficFlowFilter.internetFilterGateIn;
        pppIf.upperLayerIn <-- gtp_user.pppGate;

        trafficFlowFilter.gtpUserGateOut --> gtp_user.trafficFlowFilterGate;

        // at dispatcher to transport layer connections
        at.out++ --> udp.appIn;
        at.in++ <-- udp.appOut;

        // transport layer to tn dispatcher connections
        udp.ipOut --> tn.in++;
        udp.ipIn <-- tn.out++;

        // tn dispatcher to network layer connections
        tn.out++ --> ipv4.transportIn if hasIpv4;
        tn.in++ <-- ipv4.transportOut if hasIpv4;

        tn.out++ --> ipv6.transportIn if hasIpv6;
        tn.in++ <-- ipv6.transportOut if hasIpv6;

        tn.out++ --> generic.transportIn if hasGn;
        tn.in++ <-- generic.transportOut if hasGn;

        tn.out++ --> nl.in++;
        tn.in++ <-- nl.out++;

        pppIf.phys <--> filterGate;
}

