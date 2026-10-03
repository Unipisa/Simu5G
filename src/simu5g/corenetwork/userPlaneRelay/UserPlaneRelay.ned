//
//                  Simu5G
//
// Copyright (C) 2026 Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.corenetwork.userPlaneRelay;

import simu5g.corenetwork.gtp.GtpUser;
import simu5g.corenetwork.trafficFlowFilter.TrafficFlowFilter;

//
// The user-plane relay of a node that ends GTP-U tunnels: the "relay" between the
// two protocol stacks in the user-plane stack diagrams of 3GPP (TS 23.401, 23.501,
// 38.300). At a base station it relays between the radio interface and the N3/S1-U
// tunnels, at a ~Upf or ~PgwStandard between the data network (N6/SGi) and the
// tunnels toward the base stations or other UPFs.
//
// Packets arriving on lowerLayerIn are classified by the ~TrafficFlowFilter, which picks
// the tunnel they go into, and are tunneled by the ~GtpUser over UDP (socketOut/socketIn);
// packets arriving in a tunnel leave on lowerLayerOut, or are tunneled on (see ~GtpUser).
//
// The ND and Ethernet bridge gates lead to the node's ~Ipv6NdResponder and
// ~EthernetSessionBridge, if it has them; they are left unconnected otherwise.
//
module UserPlaneRelay
{
    parameters:
        @display("i=block/routing;bgb=300,250");
        string ownerType @enum(ENODEB,GNODEB,PGW,UPF,UPF_MEC);  // The type of the containing node, which sets it
        string gateway = default("");  // The core network gateway of a base station or a MEC host's UPF, a module path relative to the network; unused at an anchor (PGW, UPF)
        string mecHost = default("");  // At a base station: the MEC host the base station steers traffic to; empty = none

    gates:
        output socketOut;  // To the UDP of the node: the tunnels
        input socketIn;
        input lowerLayerIn;  // From the interface whose traffic is relayed: the cellular NIC at a base station, the N6/SGi interface at an anchor
        output lowerLayerOut;
        output ndOut @loose;  // To the Neighbor Discovery responder of a UPF or PGW, see ~Ipv6NdResponder
        input ndIn @loose;
        output ethernetBridgeOut @loose;  // At a UPF: the uplink frames of the Ethernet sessions, to the ~EthernetSessionBridge
        input ethernetBridgeIn @loose;  // At a UPF: the downlink frames of the Ethernet sessions, from the ~EthernetSessionBridge

    submodules:
        trafficFlowFilter: TrafficFlowFilter {
            parameters:
                @display("p=200,175");
                ownerType = parent.ownerType;
                gateway = parent.gateway;
                mecHost = parent.mecHost;
        }
        gtpUser: GtpUser {
            parameters:
                @display("p=100,75");
                gateway = parent.gateway;
        }

    connections:
        gtpUser.socketOut --> socketOut;
        gtpUser.socketIn <-- socketIn;

        lowerLayerIn --> trafficFlowFilter.internetFilterGateIn;
        trafficFlowFilter.gtpUserGateOut --> gtpUser.trafficFlowFilterGate;
        gtpUser.pppGate --> lowerLayerOut;

        gtpUser.ndOut --> ndOut;
        gtpUser.ndIn <-- ndIn;

        gtpUser.ethernetBridgeOut --> ethernetBridgeOut;
        ethernetBridgeIn --> trafficFlowFilter.ethernetBridgeIn;
}
