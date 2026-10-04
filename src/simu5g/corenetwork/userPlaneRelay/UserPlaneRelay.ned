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
// The relay has two sides: the user data (the T-PDUs of TS 29.281: IP datagrams, Ethernet
// frames, Unstructured payloads) on userDataIn/userDataOut, and the tunnels (G-PDUs, i.e.
// GTP-U over UDP) on socketOut/socketIn. User data arriving on userDataIn is classified by
// the ~TrafficFlowFilter, which picks the tunnel it goes into, and tunneled by the
// ~GtpUser; user data arriving in a tunnel leaves on userDataOut, or is tunneled on (see
// ~GtpUser). At a ~Upf or ~PgwStandard, the user data side is a message dispatcher,
// through which the relay reaches the data-network interface, the ~EthernetSessionBridge
// and the ~Ipv6NdResponder (see ~GtpUser).
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
        input userDataIn;  // The user data to tunnel: from the cellular NIC at a base station, from the node's message dispatcher at a UPF or PGW
        output userDataOut;  // The user data out of the tunnels

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

        userDataIn --> trafficFlowFilter.in;
        trafficFlowFilter.out --> gtpUser.userDataIn;
        gtpUser.userDataOut --> userDataOut;
}
