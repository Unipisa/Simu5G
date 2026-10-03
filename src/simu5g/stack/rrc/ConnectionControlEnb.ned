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
// the control plane of other nodes -- the ~CoreControl, the other base stations, and
// the UEs' ~ConnectionControlUe -- talks to, and that the node's own data path turns
// to when it needs a bearer.
//
// Tunnels. It owns the node's GTP-U tunnel endpoint id (TEID) space, and programs the
// node's tunnel endpoints, ~GtpUser (the N3/S1-U end) and ~GtpUserX2 (the X2-U end),
// as the calls of the ~CoreControl and of the other base stations say:
//  - sessionTunnelSetup(): a session's tunnels at this base station: the downlink
//    tunnel, under a TEID allocated here, which also receives the downlink a handover
//    source forwards over X2-U, and the uplink tunnels into the core network; set up
//    once per session, whichever side asks first
//  - setForwardingTeid(): the session's downlink TEID at another base station, to
//    forward the downlink to it with during a handover
//  - sessionRelease(): the session's tunnels here are forgotten
//  - addDcTunnel(), setDcTunnelTeid(): the X2-U tunnels of a dual connectivity
//    bearer, one per direction, each allocated at its receiving end
//
// Bearers. It establishes the data radio bearers of the UEs it serves, from the bearer
// definitions the ~BearerConfigurator holds (which it reads and never writes): a flow
// the data path could not place gets the bearer of the definition it matches, an
// unmapped QFI the bearer of the definition that maps it, and each bearer's entities
// are installed at this node, at the UE, and at a dual connectivity secondary, through
// their control-plane entry points -- the RRC signaling the model does not have. It
// also owns the DRB identities of its UEs (one pool per UE-and-node pair), which a
// torn-down bearer's returns to.
//
// Attach. A UE's leg connects here with connectionSetupRequest() (at initialization, at its
// configured serving cell; during the run, when a detached leg finds a cell), stating
// the type of the session the UE requests; the base station takes it on at its AMC and
// registers the UE with the ~CoreControl (the INITIAL UE MESSAGE of S1AP/NGAP), which
// establishes the UE's session and asks this base station to set up its resources
// (sessionResourceSetup(): the tunnels, which keep the session's type, the UE's uplink
// QoS rules, and the static bearers of the leg that carries them). A
// leg that lost its cell, or whose UE leaves, says so with connectionLost(): the base
// station releases its state for the leg and tells the core network.
//
// Handover. The serving base station decides on a leg's measurementReport() and
// prepares the handover with the target over Xn: handoverRequest() carries the leg's
// context to the target, which sets the UE's session tunnels up at the base station
// the downlink will enter the RAN at, installs the leg's bearer configuration, and
// starts holding the leg's downlink;
// handoverRequestAck() brings the source the target's downlink TEIDs, on which the
// source commands the leg (the UE's handoverCommand()) and forwards its downlink over
// X2-U from then on. When the leg arrives (reconfigurationComplete()), the target
// takes it on, has the ~CoreControl switch the session's downlink path (PATH SWITCH
// REQUEST, reporting the tunnels the preparation set up; the base station the
// downlink enters at learns of the switch with downlinkPathSwitched()), and releases
// the leg at the source (ueContextRelease(), the UE CONTEXT RELEASE of Xn), which
// stops forwarding and drops its state for the leg. A source that loses the leg
// meanwhile cancels the handover at the target (handoverCancel()).
//
// Radio link failure. Either end's RLC may detect it; the detecting node releases its
// end of the link and tells the other through its control-plane entry point
// (radioLinkFailure() here, from a UE leg; the UE's from this base station), which
// releases its end the same way -- and starts RRC re-establishment if configured
// (see ~BearerManagement).
//
// Nothing outside the node reaches its ~GtpUser, ~GtpUserX2 or ~BearerManagement
// directly; how the node is composed is its own business. Other nodes find this
// module through the ~Binder's node directory. The D2D-capable variant,
// ~ConnectionControlEnbD2D, adds the sidelink and multicast bearers.
//
simple ConnectionControlEnb like IConnectionControl
{
    parameters:
        @display("i=block/control");
        string binderModule = default("binder");
        string bearerConfiguratorModule = default("bearerConfigurator");
        string coreControlModule = default("coreControl");
        string bearerManagementModule = default("^.bearerManagement");
        string macModule = default("^.^.mac");
        string handoverPacketHolderModule = default("^.^.handoverPacketHolder");
        string gtpUserModule = default("^.^.^.relay.gtpUser");
        string gtpUserX2Module = default("^.^.^.gtpUserX2");
        int macNodeId;  // The base station's node id; the NIC sets it from the node's
        string gateway = default("");  // The node's core network gateway, as its gateway parameter names it; ignored at a node not connected to a core network (a secondary node)
}
