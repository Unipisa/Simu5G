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

package simu5g.corenetwork.coreControl;

//
// The control plane of the core network, one per cellular network alongside its
// ~Binder and ~BearerConfigurator: the MME and the control plane of the gateways of
// an EPC, or the AMF and the SMF of a 5G core, in one module.
//
// As the MME/AMF, it takes the UEs' registrations: a UE's leg that connects at a
// base station is announced here by that base station (initialUeMessage(), the
// INITIAL UE MESSAGE of S1AP/NGAP; at initialization, every leg attached to its
// configured cell). As the gateways' control plane/SMF, it keeps a session per
// UE (session id 1), of the type the UE requests with its registration (the
// sessionType parameter of ~LteUe), established at the UE's first registration and
// released when the UE leaves the simulation. A type the anchor's core network cannot
// carry is an error: an EPC (a ~PgwStandard anchor) carries the IP types only. The
// module programs the nodes that end the session's
// tunnels, and tells them the session's type, through their control-plane entry
// points: the user plane nodes' ~UserPlaneNodeControl and the base stations'
// connection controls (~ConnectionControlEnb, whose sessionResourceSetup() also
// installs the UE's uplink QoS rules and static bearers). A session has an uplink
// tunnel at the anchor UPF (or PGW) and at each MEC host UPF of the anchor's core
// network, and a downlink tunnel at each base station the session's downlink has
// entered the RAN at; the tunnel endpoint ids (TEIDs) are allocated by the receiving
// node itself. The anchor is the core network gateway of the base station the UE's
// downlink enters the RAN at when the session is established, and it stays for the
// lifetime of the session (SSC mode 1): a handover only moves the downlink end of the
// tunnel (the path switch, TS 23.502 4.9.1.2.2), on the target base station's PATH
// SWITCH REQUEST, and the anchor closes the old path with an End Marker. The target's
// tunnels are set up at handover preparation, between the base stations over Xn, and
// reported with the request.
//
// The module also delivers the QFI classification rules authored in the
// ~BearerConfigurator (its dlQfiRules and ulQfiRules parameters) to their evaluation
// sites: the downlink rules to each user plane node, the uplink rules to each
// SDAP UE's classifier (the QoS rules NAS signaling installs into a UE at PDU session
// establishment, TS 23.501 5.7.1.4).
//
// Like the ~Binder, the module is reached through module paths supplied in
// parameters (coreControlModule), so several cellular networks -- each with its own
// Binder, BearerConfigurator and CoreControl -- can coexist in one simulation. The
// signaling toward the user plane nodes (GTP-C/PFCP) and the base stations (S1AP/NGAP)
// is modeled as calls; a separate S-GW, intermediate UPFs (I-UPF) and SSC modes 2 and
// 3 are not modeled.
//
simple CoreControl
{
    parameters:
        @display("i=block/cogwheel");
        string binderModule = default("binder");
        string bearerConfiguratorModule = default("bearerConfigurator");
}
