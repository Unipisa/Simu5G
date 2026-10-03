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

package simu5g.corenetwork.gtp;

//
// Implements a simplified version of the user plane of the GPRS Tunneling
// Protocol (GTP). On one hand, this module encapsulates incoming IP datagrams into GTP
// packets and sends them to the tunnel destination endpoint (GTP operates on top of
// UDP). The destination endpoint is identified by utilizing information from the
// ~TrafficFlowFilter module. On the other hand, when this module receives GTP packets
// from a tunnel source endpoint, it decapsulates the included IP datagram and sends it
// out on lowerLayerOut: at a base station into the cellular NIC (a base station does not
// IP-route the UEs' traffic), at a ~Upf or ~PgwStandard onto the data network, unless
// the destination is a UE of the same core network, whose base station the datagram
// is then tunneled on to. At a ~Upf or ~PgwStandard, lowerLayerOut leads to the node's
// message dispatcher, and every packet carries the service it is for (DispatchProtocolReq):
// ipv4/ipv6 for the data network, ethernetmac for the ~EthernetSessionBridge, and icmpv6
// for the ~Ipv6NdResponder, which gets the link-local traffic of the UEs' IPv6 sessions,
// each packet tagged with its session (the IP link of a session ends at its anchor, and
// its Neighbor Discovery must not leak onto the data network).
// Resides, with the ~TrafficFlowFilter, in the ~UserPlaneRelay of every node that ends
// GTP-U tunnels: ~eNodeB, ~gNodeB, ~PgwStandard and ~Upf (also as the UPF of a ~MecHost).
//
// The tunnel a G-PDU arrives on names its session, and so what the T-PDU carries:
// the payload goes out as the protocol the session's type implies (an IP datagram, or
// the "unstructured" protocol), and a base station attaches the session, with its
// type, as a ~SessionTag. At an anchor UPF the uplink of an Unstructured session
// leaves on the session's N6 tunnel, in a UDP/IP datagram this module builds from
// the session's N6 address to the server (see ~UserPlaneNodeControl). The uplink of an
// Ethernet session goes to the UPF's ~EthernetSessionBridge, its FCS rebuilt (the 5GS does not
// carry it), tagged with its session; the downlink frames the traffic flow filter classified
// lose their FCS here, before they enter the session's tunnel.
//
simple GtpUser
{
    parameters:
        string binderModule = default("binder");
        int localPort = default(31);
        int tunnelPeerPort = default(31);
        string gateway = default("");			// Optional

        @display("i=block/tunnel");

    gates:
        output socketOut;
        input socketIn;

        input trafficFlowFilterGate;
        output lowerLayerOut;  // To the cellular NIC at a base station, to the node's message dispatcher at a UPF or PGW
}

