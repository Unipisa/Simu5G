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
// host's UPF): the node's end of the N4 reference point (TS 23.501 4.2.3), over
// which the SMF programs the user plane with PFCP (TS 29.244). The signaling is not
// modeled as packets; it is modeled as the calls this module answers, on behalf of
// the SMF, which the ~BearerConfigurator stands in for:
//  - setDownlinkClassifierRules(): the node's downlink QFI classification rules (the packet
//    detection and QoS enforcement rules, PDR/QER), installed into the node's
//    ~TrafficFlowFilter
//  - establishUserPlaneSession(): a PDU session's uplink tunnel at this node, installed into
//    the node's ~GtpUser under a tunnel endpoint id (TEID) allocated from the node's
//    own TEID space
//  - updateDownlinkTunnel(): the session's downlink tunnel, i.e. the F-TEID at the base
//    station the downlink enters the RAN at, and the End Marker that closes the old
//    path after a path switch (TS 23.502 4.9.1.2.2)
//  - releaseUserPlaneSession(): the session's tunnels are forgotten
//
// Nothing outside the node reaches its ~GtpUser or ~TrafficFlowFilter directly; how
// the node is composed is its own business. The module registers with the ~Binder
// at initialization as the node's N4 endpoint, which is how the SMF finds the user
// plane nodes of the network and the core network gateway a MEC host's UPF sits
// behind.
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
