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
// ~Binder and ~BearerConfigurator: the AMF and the SMF of a 5G core (in EPS terms,
// the MME and the control plane of the gateways) in one module.
//
// As the SMF, it keeps a PDU session per UE (PDU Session ID 1), established when the
// UE first has a serving node and released when the UE leaves the simulation, and
// programs the nodes that end the session's tunnels through their control-plane
// entry points: the user plane nodes' N4 endpoints (~UserPlaneNodeControl) and the base
// stations' connection controls (~ConnectionControlEnb). A session has an uplink
// tunnel at the anchor UPF (or PGW) and at each MEC host UPF of the anchor's core
// network, and a downlink tunnel at each base station the session's downlink has
// entered the RAN at; the tunnel endpoint ids (TEIDs) are allocated by the receiving
// node itself. The anchor is the core network gateway of the base station the UE's
// downlink enters the RAN at when the session is established, and it stays for the
// lifetime of the session (SSC mode 1): a handover only moves the downlink end of the
// tunnel (the path switch, TS 23.502 4.9.1.2.2), at the instant the ~Binder records
// the new serving node, and the anchor closes the old path with an End Marker.
// Before that, at handover preparation, the target base station gets the session's
// tunnels, so the source can forward the downlink to it over X2 (the UEs' handover
// controllers announce the preparation with a signal this module listens to).
//
// The module also delivers the QFI classification rules authored in the
// ~BearerConfigurator (its dlQfiRules and ulQfiRules parameters) to their evaluation
// sites: the downlink rules to each user plane node over N4, the uplink rules to each
// SDAP UE's classifier (the QoS rules NAS signaling installs into a UE at PDU session
// establishment, TS 23.501 5.7.1.4).
//
// Like the ~Binder, the module is reached through module paths supplied in
// parameters (coreControlModule), so several cellular networks -- each with its own
// Binder, BearerConfigurator and CoreControl -- can coexist in one simulation. N4
// signaling is modeled as calls; intermediate UPFs (I-UPF) and SSC modes 2 and 3 are
// not modeled.
//
simple CoreControl
{
    parameters:
        @display("i=block/cogwheel");
        string binderModule = default("binder");
        string bearerConfiguratorModule = default("bearerConfigurator");
}
