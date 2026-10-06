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

import inet.common.MessageDispatcher;
import inet.linklayer.ppp.PppInterface;
import inet.node.base.ApplicationLayerNodeBase;
import inet.transportlayer.udp.Udp;
import simu5g.corenetwork.ndResponder.Ipv6NdResponder;
import simu5g.corenetwork.userPlaneNodeControl.UserPlaneNodeControl;
import simu5g.corenetwork.userPlaneRelay.UserPlaneRelay;

//
// Implements a simplified EPC PDN Gateway (P-GW) of a 4G LTE network.
// It serves as the entry point to the core network of an LTE network, thus bridging it to the rest
// of the Internet. Packets arriving from the Internet are tunneled to the appropriate next hop in
// the core network (e.g., the ~eNodeB that will transmit the packet to the destination User Equipment
// through the radio access network) using the GPRS Tunneling Protocol (GTP). Conversely, packets
// received from the core network are detunneled and sent to the Internet (e.g., towards a remote server).
// To accomplish this, it utilizes a ~UserPlaneRelay, whose ~GtpUser handles GTP (de)tunneling and
// whose ~TrafficFlowFilter classifies incoming packets.
//
// The interface toward the Internet (dnPpp, on the dnPppg gate) belongs to this user plane:
// it is not in the node's interface table, so the node's own network layer (the stack of
// the core network) neither sees nor uses it, and the address configurator gives it no
// address.
//
// The relay reaches the SGi interface and the ~Ipv6NdResponder (ndResponder) through the
// rd message dispatcher, by service (see ~GtpUser).
//
module PgwStandard extends ApplicationLayerNodeBase
{
    parameters:
        @display("i=device/mainframe;bgb=1700,1125");
        @figure[userPlane](type=rectangle; pos=1275,5; size=400,437; fillColor=#ff8000; lineColor=#808080; cornerRadius=5; fillOpacity=0.1);
        @figure[userPlane.title](type=text; pos=1670,10; anchor=ne; text="user plane");
        @figure[sgiInterfaces](type=rectangle; pos=1275,458; size=400,660; fillColor=#00ffff; lineColor=#808080; cornerRadius=5; fillOpacity=0.1);
        @figure[sgiInterfaces.title](type=text; pos=1670,463; anchor=ne; text="SGi interfaces");
        @figure[submodules];

        string nodeType = "PGW";
        bool hasNdResponder = default(false);  // Answer the IPv6 Neighbor Discovery of the UEs' sessions (needed when UEs run IPv6), see ~Ipv6NdResponder

        forwarding = default(true);
        multicastForwarding = default(false);

    gates:
        inout dnPppg @labels(PppFrame-conn);

    submodules:
        dnPpp: PppInterface {
            parameters:
                interfaceTableModule = "";  // Not an interface of the node's network layer: the user plane's SGi end
                displayStringTextFormat = "";
                @display("p=1375,975");
        }
        relay: UserPlaneRelay {
            parameters:
                @display("p=1125,75");
                ownerType = parent.nodeType;
        }
        rd: MessageDispatcher {
            parameters:
                @display("p=1475,450;b=380,5,,,,1");
                serviceMapping = {"ipv4": "dnPpp", "ipv6": "dnPpp"};  // The data network; the optional modules register their services
                protocolMapping = {"ipv4": "relay", "ipv6": "relay", "ethernetmac": "relay"};
                forwardServiceRegistration = false;
                forwardProtocolRegistration = false;
        }
        nodeControl: UserPlaneNodeControl {
            parameters:
                @display("p=125,400;is=s");
                nodeType = parent.nodeType;
        }
        ndResponder: Ipv6NdResponder if hasNdResponder {
            parameters:
                @display("p=1550,225");
        }

    connections:

        relay.socketOut --> at.in++;
        relay.socketIn <-- at.out++;

        relay.userDataOut --> { @display("m=h,v"); } --> rd.in++;
        rd.out++ --> { @display("m=v,h"); } --> relay.userDataIn;

        rd.out++ --> dnPpp.upperLayerIn;
        dnPpp.upperLayerOut --> rd.in++;

        rd.out++ --> ndResponder.in if hasNdResponder;
        ndResponder.out --> rd.in++ if hasNdResponder;

        dnPpp.phys <--> { @display("m=s"); } <--> dnPppg;
}

