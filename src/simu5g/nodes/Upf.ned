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
import inet.linklayer.contract.IEthernetInterface;
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
// The N6 Ethernet interface (dnEth, on the dnEthg gate, with hasEthernetBridge) is a port of
// the ~EthernetSessionBridge (ethBridge), registered in the bridge's own interface table.
//
// The relay reaches the N6 interface, the ~EthernetSessionBridge (ethBridge) and the
// ~Ipv6NdResponder (ndResponder) through the rd message dispatcher, by service (see ~GtpUser).
//
module Upf extends ApplicationLayerNodeBase
{
    parameters:
        @display("i=abstract/switch;bgb=1700,1125");
        @figure[userPlane](type=rectangle; pos=1275,5; size=400,437; fillColor=#ff8000; lineColor=#808080; cornerRadius=5; fillOpacity=0.1);
        @figure[userPlane.title](type=text; pos=1670,10; anchor=ne; text="user plane");
        @figure[n6Interfaces](type=rectangle; pos=1275,458; size=400,660; fillColor=#00ffff; lineColor=#808080; cornerRadius=5; fillOpacity=0.1);
        @figure[n6Interfaces.title](type=text; pos=1670,463; anchor=ne; text="N6 interfaces");
        @figure[submodules];

        string nodeType = default("UPF");   // Do NOT change!
        string gateway = default("");
        bool hasNdResponder = default(false);  // Answer the IPv6 Neighbor Discovery of the UEs' sessions (needed when UEs run IPv6), see ~Ipv6NdResponder
        bool hasEthernetBridge = default(false);  // Anchor Ethernet PDU sessions: the node's Ethernet bridge and N6 Ethernet link (dnEthg), see ~EthernetSessionBridge
        forwarding = default(true);
        multicastForwarding = default(false);
        dnEth.interfaceTableModule = absPath(".ethBridge.interfaceTable");  // A port of the Ethernet session bridge, not an interface of the node's network layer

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
                displayStringTextFormat = "";
                @display("p=1375,975");
        }
        relay: UserPlaneRelay {
            parameters:
                @display("p=1125,75");
                ownerType = parent.nodeType;
                gateway = parent.gateway;
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
                gateway = parent.gateway;
        }
        ndResponder: Ipv6NdResponder if hasNdResponder {
            parameters:
                @display("p=1550,225");
        }
        ethBridge: EthernetSessionBridge if hasEthernetBridge {
            parameters:
                @display("p=1575,700");
        }
        dnEth: <default("LayeredEthernetInterface")> like IEthernetInterface if hasEthernetBridge {
            parameters:
                @display("p=1575,975");
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

        rd.out++ --> ethBridge.sessionIn if hasEthernetBridge;
        ethBridge.sessionOut --> rd.in++ if hasEthernetBridge;
        ethBridge.lowerLayerOut --> dnEth.upperLayerIn if hasEthernetBridge;
        dnEth.upperLayerOut --> ethBridge.lowerLayerIn if hasEthernetBridge;
        dnEth.phys <--> { @display("m=s"); } <--> dnEthg if hasEthernetBridge;

        dnPpp.phys <--> { @display("m=s"); } <--> dnPppg;
}
