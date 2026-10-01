//
//                  Simu5G
//
// Authors: Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.corenetwork.userPlaneNodeControl;

//
// The control-plane entry point of a user plane node (~Upf, ~PgwStandard, a MEC
// host's UPF): the node's end of the interface over which the core network's control
// plane (~CoreControl) programs the user plane -- N4 with PFCP (TS 29.244) in a 5G
// core; in an EPC, GTP-C over S11/S5 (TS 29.274), or Sx with PFCP under CUPS. The
// signaling is not modeled as packets; it is modeled as the calls this module
// answers:
//  - setDownlinkClassifierRules(): the node's downlink QFI classification rules (in
//    5GC terms the packet detection and QoS enforcement rules, PDR/QER), installed
//    into the node's ~TrafficFlowFilter
//  - establishUserPlaneSession(): a session's uplink tunnel at this node (PFCP Session
//    Establishment; GTP-C Create Session), installed into the node's ~GtpUser under a
//    tunnel endpoint id (TEID) allocated from the node's own TEID space, with the
//    session's type (the PDU session type / PDN type these messages carry), which the
//    tunnel end attaches to every datagram of the session (see ~SessionTag)
//  - updateDownlinkTunnel(): the session's downlink tunnel, i.e. the F-TEID at the
//    base station the downlink enters the RAN at, and the End Marker that closes the
//    old path after a path switch (PFCP Session Modification; GTP-C Modify Bearer;
//    TS 23.502 4.9.1.2.2, TS 23.401 5.5.1.1.2)
//  - releaseUserPlaneSession(): the session's tunnels are forgotten (PFCP Session
//    Deletion; GTP-C Delete Session)
//
// Unstructured sessions. A UPF anchors a UE's Unstructured session (see the
// sessionType parameter of ~LteUe) with an N6 point-to-point tunnel based on UDP/IP to
// one application server (TS 29.561 9.2): the session's uplink payload leaves the node
// in a UDP/IP datagram from the session's N6 address and the unstructuredPort to the
// unstructuredServer at the unstructuredServerPort, and a datagram of the data network
// to the session's N6 address and the unstructuredPort is the session's downlink,
// whose payload goes into the session's tunnel without the UDP/IP headers. The node
// originates and ends those datagrams itself, as a forwarding node, on its N6
// interface. The ~CoreControl (the SMF) allocates each session's N6 address from the
// unstructuredPrefix, one address per session, and hands it over with the session's
// establishment; the data network routes the prefix to this node. TS 29.561 defines
// no port numbers: the UPF and the server use ports configured alike at both ends,
// which is why the two port parameters have no default. The prefix, the server and the
// ports are the node's, not per data network: a UPF serves one data network here.
// Both address families work; the server is resolved in the prefix's family.
//
// Ethernet sessions. A UPF with an ~EthernetSessionBridge anchors a UE's Ethernet session: its
// establishment adds the session's port to the anchor's bridge, its release removes it
// (flushing the addresses learned on it). A UPF without one rejects the session.
//
// Nothing outside the node reaches its ~GtpUser or ~TrafficFlowFilter directly; how
// the node is composed is its own business. The module registers with the ~Binder
// at initialization as the node's control-plane entry point, which is how the
// ~CoreControl finds the user plane nodes of the network and the core network
// gateway a MEC host's UPF sits behind.
//
simple UserPlaneNodeControl
{
    parameters:
        @display("i=block/cogwheel");
        string binderModule = default("binder");
        string gtpUserModule = default("^.gtp_user");
        string trafficFlowFilterModule = default("^.trafficFlowFilter");
        string ethernetBridgeModule = default("^.ethBridge.sessionMux");  // The node's Ethernet session bridge (see ~EthernetSessionBridge), if it has one
        string nodeType @enum(PGW,UPF,UPF_MEC);  // The type of the containing node, which sets it
        string gateway = default("");  // A MEC host UPF's core network gateway, the anchor UPF whose sessions it serves; empty at an anchor
        string unstructuredPrefix = default("");  // The prefix the N6 addresses of Unstructured sessions are allocated from, e.g. "2001:db8:5::/64" or "10.5.0.0/16"; empty = the node anchors no Unstructured session (see the main comment)
        string unstructuredServer = default("");  // The application server the Unstructured sessions' uplink goes to: a module path relative to the network, resolved in the family of unstructuredPrefix
        int unstructuredServerPort = default(-1);  // The UDP port of unstructuredServer the uplink goes to; must be set for Unstructured sessions
        int unstructuredPort = default(-1);  // The UDP port the node receives the Unstructured sessions' downlink on, and sends their uplink from; must be set for Unstructured sessions
}
