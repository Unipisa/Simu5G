//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
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
// Implements the 5G Core User Plane Function (UPF).
//
// The UPF is responsible for packet routing and forwarding, packet inspection, and (de)tunneling according to GTP.
// The core network can contain multiple UPFs. Intermediate UPFs, which are not connected to an external Data Network,
// act like IP routers in the current version.
// Allows the connection of mobile terminals to external data networks or other 5G network elements through GTP tunneling.
//
module Upf extends ApplicationLayerNodeBase
{
    parameters:
        @display("i=abstract/switch");
        @figure[submodules];

        string nodeType = default("UPF");   // do NOT change!
        string gateway = default("");
        forwarding = default(true);
        multicastForwarding = default(false);

    gates:
        // this gate is used for the UPF connected to the external data network
        // it must be left unconnected for intermediate UPFs
        inout dnPppg @loose @labels(PppFrame-conn);

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
                gateway = parent.gateway;
        }

    connections:

        gtp_user.socketOut --> at.in++;
        gtp_user.socketIn <-- at.out++;

        pppIf.upperLayerOut --> trafficFlowFilter.internetFilterGateIn;
        pppIf.upperLayerIn <-- gtp_user.pppGate;

        trafficFlowFilter.gtpUserGateOut --> gtp_user.trafficFlowFilterGate;

        pppIf.phys <--> dnPppg;
}
