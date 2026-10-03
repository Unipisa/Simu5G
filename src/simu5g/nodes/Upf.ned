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
import simu5g.corenetwork.ethernetSessionBridge.EthernetSessionBridge;
import simu5g.corenetwork.ndResponder.Ipv6NdResponder;
import simu5g.corenetwork.userPlaneNodeControl.UserPlaneNodeControl;
import simu5g.corenetwork.userPlaneRelay.UserPlaneRelay;

//
// Implements the 5G Core User Plane Function (UPF).
//
// The UPF is responsible for packet routing and forwarding, packet inspection, and (de)tunneling according to GTP.
// The core network can contain multiple UPFs. Intermediate UPFs, which are not connected to an external Data Network,
// act like IP routers in the current version.
// Allows the connection of mobile terminals to external data networks or other 5G network elements through GTP tunneling.
//
// The N6 interface toward the data network (dnPpp, on the dnPppg gate) belongs to this user
// plane: it is not in the node's interface table, so the node's own network layer (the stack
// of the core network) neither sees nor uses it, and the address configurator gives it no
// address.
//
module Upf extends ApplicationLayerNodeBase
{
    parameters:
        @display("i=abstract/switch");
        @figure[submodules];

        string nodeType = default("UPF");   // Do NOT change!
        string gateway = default("");
        bool hasNdResponder = default(false);  // Answer the IPv6 Neighbor Discovery of the UEs' sessions (needed when UEs run IPv6), see ~Ipv6NdResponder
        bool hasEthernetBridge = default(false);  // Anchor Ethernet PDU sessions: the node's Ethernet bridge and N6 Ethernet link (dnEthg), see ~EthernetSessionBridge
        forwarding = default(true);
        multicastForwarding = default(false);

    gates:
        // This gate is used for the UPF connected to the external data network
        // it must be left unconnected for intermediate UPFs
        inout dnPppg @loose @labels(PppFrame-conn);
        // The N6 Ethernet link of the Ethernet PDU sessions (with hasEthernetBridge)
        inout dnEthg @loose @labels(EtherFrame-conn);

    submodules:
        dnPpp: PppInterface {
            parameters:
                interfaceTableModule = "";  // Not an interface of the node's network layer: the user plane's N6 end
                @display("p=1100,975");
        }
        relay: UserPlaneRelay {
            parameters:
                @display("p=1200,75");
                ownerType = parent.nodeType;
                gateway = parent.gateway;
        }
        nodeControl: UserPlaneNodeControl {
            parameters:
                @display("p=1200,175");
                nodeType = parent.nodeType;
                gateway = parent.gateway;
        }
        ndResponder: Ipv6NdResponder if hasNdResponder {
            parameters:
                @display("p=1000,175");
        }
        ethBridge: EthernetSessionBridge if hasEthernetBridge {
            parameters:
                @display("p=1100,275");
        }

    connections:

        relay.socketOut --> at.in++;
        relay.socketIn <-- at.out++;

        dnPpp.upperLayerOut --> relay.lowerLayerIn;
        dnPpp.upperLayerIn <-- relay.lowerLayerOut;

        relay.ndOut --> ndResponder.in if hasNdResponder;
        relay.ndIn <-- ndResponder.out if hasNdResponder;

        relay.ethernetBridgeOut --> ethBridge.sessionIn if hasEthernetBridge;
        ethBridge.sessionOut --> relay.ethernetBridgeIn if hasEthernetBridge;
        ethBridge.dnEthg <--> dnEthg if hasEthernetBridge;

        dnPpp.phys <--> dnPppg;
}
