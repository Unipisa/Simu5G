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
        string nodeType @enum(PGW,UPF,UPF_MEC);  // The type of the containing node, which sets it
        string gateway = default("");  // A MEC host UPF's core network gateway, the anchor UPF whose sessions it serves; empty at an anchor
}
