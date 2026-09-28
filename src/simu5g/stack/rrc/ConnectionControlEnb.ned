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

package simu5g.stack.rrc;

//
// The control-plane entry point of a base station: the one module of the node that
// the control plane of other nodes -- today the ~BearerConfigurator, standing in for
// the SMF -- talks to. It owns the node's GTP-U tunnel endpoint id (TEID) space, and
// programs the node's tunnel endpoints, ~GtpUser (the N3/S1-U end) and ~GtpUserX2
// (the X2-U end), as the calls say:
//  - sessionTunnelSetup(): a PDU session's tunnels at this base station: the downlink
//    tunnel, under a TEID allocated here, which also receives the downlink a handover
//    source forwards over X2-U, and the uplink tunnels into the core network
//  - setForwardingTeid(): the session's downlink TEID at another base station, to
//    forward the downlink to it with during a handover
//  - sessionRelease(): the session's tunnels here are forgotten
//  - addDcTunnel(), setDcTunnelTeid(): the X2-U tunnels of a dual connectivity
//    bearer, one per direction, each allocated at its receiving end
//
// Nothing outside the node reaches its ~GtpUser or ~GtpUserX2 directly; how the node
// is composed is its own business. Other nodes find this module through the
// ~Binder's node directory.
//
simple ConnectionControlEnb like IConnectionControl
{
    parameters:
        @display("i=block/control");
        string binderModule = default("binder");
        string gtpUserModule = default("^.^.^.gtpUser");
        string gtpUserX2Module = default("^.^.^.gtpUserX2");
        int macNodeId;  // The base station's node id; the NIC sets it from the node's
        string gateway = default("");  // The node's core network gateway, as its gateway parameter names it; ignored at a node not connected to a core network (a secondary node)
}
