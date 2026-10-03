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

package simu5g.corenetwork.trafficFlowFilter;

//
// Maps IP 4-Tuples to Traffic Flow Template (TFT) identifiers, which are used by the ~GtpUser module
// contained in the same node. It interacts with the ~Binder to determine the destination of the packet.
// Resides in any module that also includes a ~GtpUser module, such as ~eNodeB, ~gNodeB, ~PgwStandard,
// ~Upf, and ~MecHost modules. For instance, in a ~Upf module acting as the entry point of the 5G network, it
// identifies the destination endpoint of the GTP tunnel that the packet should traverse, which is the gNodeB
// serving the destination User Equipment (UE). In a base station, it receives the UEs'
// uplink datagrams straight from the cellular NIC.
//
// The module also assigns each packet entering the tunnel its QoS Flow Identifier
// (QFI). At a core-network tunnel entry (~Upf, ~PgwStandard, a MEC host's UPF) the
// QFI-assignment rules are installed by the node's control-plane entry point,
// ~UserPlaneNodeControl, on behalf of the ~CoreControl, the SMF stand-in (the rules are
// authored in its dlQfiRules parameter): they model the packet detection and QoS
// enforcement rules (PDR/QER) that the SMF installs into a UPF over N4 signaling --
// a real UPF holds no classification policy of its own, and neither does this
// module. A packet matching no rule gets QFI 0 (the default flow).
//
// A base station is no rule-enforcement point: no SMF installs classification rules
// there, and none can be authored for it. An uplink packet that SDAP already
// attributed to a QoS flow keeps that QFI on its way into the core network; uplink
// traffic from a stack without SDAP is classified by a built-in DSCP-as-QFI
// residual.
//
// Unstructured sessions (see the sessionType parameter of ~LteUe). At a base station,
// their uplink payload, which SDAP delivers up as the "unstructured" protocol, goes
// to the session's anchor on the QoS flow SDAP attributed it to: no MEC steering and
// no local delivery, which go by an IP destination the payload has none of. At the
// anchor UPF, a datagram of the data network addressed to an Unstructured session's
// N6 address is that session's downlink (see ~UserPlaneNodeControl): the N6 tunnel's
// UDP/IP headers go, and the payload goes into the session's tunnel on the default
// QoS flow, with no rule evaluated.
//
// Ethernet sessions. At a UPF, the downlink frames of the Ethernet sessions come from the
// node's ~EthernetSessionBridge, each tagged with its session: the frame goes into the session's
// tunnel on the QFI of the first rule that matches the full frame (a dscpAsQfi rule does
// not match a frame), else the default QoS flow.
//
simple TrafficFlowFilter
{
    parameters:
        @display("i=block/filter");

        string binderModule = default("binder");
        string ownerType @enum(ENODEB,GNODEB,PGW,UPF,UPF_MEC); // Must be one of ENODEB, GNODEB, PGW, UPF, UPF_MEC
        string gateway = default("");  // The core network gateway of a base station or a MEC host's UPF, a module path relative to the network; unused at an anchor (PGW, UPF), which is the gateway itself
        string mecHost = default("");  // At a base station: the MEC host the base station steers traffic to (see the mecHost parameter of ~eNodeB); empty = none
        bool fastForwarding = default(true);
    gates:
        input internetFilterGateIn;
        input ethernetBridgeIn @loose;  // At a UPF: the downlink frames of the Ethernet sessions, from the ~EthernetSessionBridge
        output gtpUserGateOut;
}
