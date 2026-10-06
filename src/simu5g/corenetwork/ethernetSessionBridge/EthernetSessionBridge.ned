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

package simu5g.corenetwork.ethernetSessionBridge;

import inet.common.MessageDispatcher;
import inet.linklayer.contract.IIeee8021qLayer;
import inet.linklayer.contract.IMacForwardingTable;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.common.NetworkInterface;
import inet.protocolelement.contract.IProtocolLayer;
import inet.linklayer.contract.IEthernetLayer;

//
// The Ethernet bridge of a ~Upf, which forwards the traffic of its Ethernet PDU sessions
// "based on detected MAC addresses" (TS 23.501 5.8.2.5.3): one learning bridge per UPF,
// whose ports are the Ethernet sessions the UPF anchors and one N6 Ethernet interface,
// the node's dnEth (TS 23.501 5.8.2.5.3, NOTE 2), which is connected to lowerLayerIn/Out:
// a submodule of the node, but a port of this bridge, registered in its interface table. A session's uplink frames enter the bridge on the
// session's port; the bridge learns their source addresses, forwards known unicast to
// the port of the destination (another session: switched locally, not via N6; or dnEth),
// floods unknown unicast, broadcast and multicast to every port but the ingress, and
// drops a frame whose destination is on its ingress port. Learning is per VLAN.
//
// The bridge is INET's layered BridgingLayer over its own interface table and
// MAC table, so the session ports and dnEth get no IP configuration and no routes, and
// the bridge floods among them only. A session port (~EthernetSessionPort) is created
// when the session is established at the UPF and deleted when it is released, its MAC
// table entries flushed first; ~EthernetSessionMux does both, on the calls of the node's
// ~UserPlaneNodeControl, and connects the ports' lower sides to the node's GTP-U path:
// uplink frames from ~GtpUser, downlink frames to the ~TrafficFlowFilter, each tagged
// with its session (~SessionTag).
//
// The frames carry their FCS inside the bridge; ~GtpUser rebuilds it where a frame leaves
// the tunnel and strips it where one enters, after the traffic flow filter has classified
// the full frame (TS 23.501 5.6.10.2: the 5GS does not carry the FCS).
//
// Not modeled (TS 23.501 5.8.2.5.3): flooding scoped to a VLAN (frames are flooded across
// VLANs), ARP/ND proxying, allowed VLAN/MAC lists, S-/C-TAG handling, the reporting of
// learned MAC addresses to the SMF, PSA relocation, 5G VN groups.
//
module EthernetSessionBridge
{
    parameters:
        @display("i=block/switch;bgb=700,600");
        **.interfaceTableModule = default(absPath(".interfaceTable"));
        **.macTableModule = default(absPath(".macTable"));
        *.fcsMode = "declared";
        ethernet.*.promiscuous = true;
        ethernet.registerProtocol = true;
    gates:
        input sessionIn;       // uplink frames of the sessions, from ~GtpUser (the ethernetmac service, which the session mux registers)
        output sessionOut;     // downlink frames of the sessions, to the ~TrafficFlowFilter
        output lowerLayerOut;  // to the N6 Ethernet interface (the node's dnEth)
        input lowerLayerIn;    // from the N6 Ethernet interface
    submodules:
        macTable: <default("MacForwardingTable")> like IMacForwardingTable {
            @display("p=100,100;is=s");
        }
        interfaceTable: InterfaceTable {
            @display("p=100,200;is=s");
        }
        sessionMux: EthernetSessionMux {
            portDisplayString = default("p=500,420,row,100;g=sessionPorts");
            @display("p=600,520");
        }
        cb: MessageDispatcher {
            @display("p=400,50;b=700,5,,,,1");
        }
        bridging: <default("BridgingLayer")> like IProtocolLayer {
            @display("p=400,120");
        }
        bl: MessageDispatcher {
            @display("p=400,190;b=700,5,,,,1");
        }
        ethernet: <default("EthernetLayer")> like IEthernetLayer {
            @display("p=300,260");
        }
        ieee8021q: <default("Ieee8021qProtocol")> like IIeee8021qLayer {
            @display("p=500,260");
        }
        li: MessageDispatcher {
            @display("p=400,330;b=700,5,,,,1");
        }
    connections allowunconnected:
        cb.out++ --> bridging.upperLayerIn;
        bridging.upperLayerOut --> cb.in++;
        bridging.lowerLayerOut --> bl.in++;
        bl.out++ --> bridging.lowerLayerIn;
        bl.out++ --> ethernet.upperLayerIn;
        ethernet.upperLayerOut --> bl.in++;
        bl.out++ --> ieee8021q.upperLayerIn;
        ieee8021q.upperLayerOut --> bl.in++;
        bl.out++ --> li.in++;
        li.out++ --> bl.in++;
        ethernet.lowerLayerOut --> li.in++;
        li.out++ --> ethernet.lowerLayerIn;
        ieee8021q.lowerLayerOut --> li.in++;
        li.out++ --> ieee8021q.lowerLayerIn;
        li.out++ --> { @display("m=s"); } --> lowerLayerOut;
        lowerLayerIn --> { @display("m=s"); } --> li.in++;
        sessionIn --> sessionMux.sessionIn;
        sessionMux.sessionOut --> sessionOut;
}

//
// The bridge's side of one Ethernet PDU session: a port of the ~EthernetSessionBridge,
// whose lower side is the session (through ~EthernetSessionMux). Created and deleted by
// the mux as the session is established and released.
//
module EthernetSessionPort extends NetworkInterface
{
    parameters:
        string interfaceTableModule;
        *.interfaceTableModule = default(absPath(this.interfaceTableModule));
        @display("i=block/ifcard");
    gates:
        input upperLayerIn;
        output upperLayerOut;
        input lowerLayerIn;
        output lowerLayerOut;
    submodules:
        portal: EthernetSessionPortal {
            @display("p=100,100");
        }
    connections:
        upperLayerIn --> portal.upperLayerIn;
        portal.upperLayerOut --> upperLayerOut;
        lowerLayerIn --> portal.lowerLayerIn;
        portal.lowerLayerOut --> lowerLayerOut;
}

//
// The inside of an ~EthernetSessionPort: frames the bridge sends down leave toward the
// session; frames of the session go up to the bridge as an Ethernet interface delivers
// them (the receive tags of the port)
//
simple EthernetSessionPortal
{
    parameters:
        @display("i=block/socket");
    gates:
        input upperLayerIn;
        output upperLayerOut;
        input lowerLayerIn;
        output lowerLayerOut;
}

//
// The bridge's entry point and multiplexer: creates an ~EthernetSessionPort per
// Ethernet PDU session and deletes it at the session's release (flushing its MAC table
// entries first), on the calls of the node's ~UserPlaneNodeControl; sends each uplink
// frame of a session (arriving on sessionIn with its ~SessionTag) to the session's port,
// and each frame a port sends down out on sessionOut, tagged with the port's session.
//
simple EthernetSessionMux
{
    parameters:
        string interfaceTableModule;  // The bridge's interface table, which the session ports are registered in
        string macTableModule;  // The bridge's MAC table, flushed of a released session's port
        string portModuleType = default("simu5g.corenetwork.ethernetSessionBridge.EthernetSessionPort");  // The module type of the session ports
        string portDisplayString = default("");  // Display string of the session ports (their place in the bridge's layout)
        @display("i=block/join");
    gates:
        input sessionIn;  // Uplink frames of the sessions, each with its SessionTag
        output sessionOut;  // Downlink frames of the sessions, each with its SessionTag
        input portIn[];  // From the session ports, one per port
        output portOut[];  // To the session ports, one per port
}
