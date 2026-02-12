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
import simu5g.corenetwork.gtp.GtpUser;
import simu5g.corenetwork.trafficFlowFilter.TrafficFlowFilter;

//
// Implements a simplified EPC PDN Gateway (P-GW) of a 4G LTE network.
// It serves as the entry point to the core network of an LTE network, thus bridging it to the rest
// of the Internet. Packets arriving from the Internet are tunneled to the appropriate next hop in
// the core network (e.g., the ~eNodeB that will transmit the packet to the destination User Equipment
// through the radio access network) using the GPRS Tunneling Protocol (GTP). Conversely, packets
// received from the core network are detunneled and sent to the Internet (e.g., towards a remote server).
// To accomplish this, it utilizes a ~GtpUser module that handles GTP (de)tunneling, along with the
// ~TrafficFlowFilter module, which classifies incoming packets.
//
module PgwStandard extends ApplicationLayerNodeBase
{
    parameters:
        @display("i=device/mainframe");
        @figure[submodules];

        string nodeType = "PGW";

        forwarding = default(true);
        multicastForwarding = default(false);

    gates:
        inout filterGate @labels(PppFrame-conn);

    submodules:
        pppIf: PppInterface {
            parameters:
                @display("p=1100,975");
        }
        trafficFlowFilter: TrafficFlowFilter {
            parameters:
                @display("p=1200,75");
                ownerType = parent.nodeType;
        }
        gtp_user: GtpUser {
            parameters:
                @display("p=1000,75");
        }

    connections:

        gtp_user.socketOut --> at.in++;
        gtp_user.socketIn <-- at.out++;

        pppIf.upperLayerOut --> trafficFlowFilter.internetFilterGateIn;
        pppIf.upperLayerIn <-- gtp_user.pppGate;

        trafficFlowFilter.gtpUserGateOut --> gtp_user.trafficFlowFilterGate;

        pppIf.phys <--> filterGate;
}

