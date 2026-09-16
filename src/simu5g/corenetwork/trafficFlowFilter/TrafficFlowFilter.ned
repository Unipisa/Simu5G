//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.corenetwork.trafficFlowFilter;

//
// Maps IP 4-Tuples to Traffic Flow Template (TFT) identifiers, which are used by the ~GtpUser module
// contained in the same node. It interacts with the ~Binder to determine the destination of the packet.
// Resides in any module that also includes a ~GtpUser module, such as ~eNodeB, ~gNodeB, ~PgwStandard,
// ~Upf, and ~MecHost modules. For instance, in a ~Upf module acting as the entry point of the 5G network, it
// identifies the destination endpoint of the GTP tunnel that the packet should traverse, which is the gNodeB
// serving the destination User Equipment (UE).
//
// The module also assigns each packet entering the tunnel its QoS Flow Identifier
// (QFI). At a core-network tunnel entry (~Upf, ~PgwStandard, a MEC host's UPF) the
// QFI-assignment rules are delivered by the ~BearerConfigurator (see its dlQfiRules
// parameter): they model the packet detection and QoS enforcement rules (PDR/QER)
// that the SMF installs into a UPF over N4 signaling at session setup -- a real UPF
// holds no classification policy of its own, and neither does this module. A packet
// matching no rule gets QFI 0 (the default flow).
//
// A base station is no rule-enforcement point: no SMF installs classification rules
// there, and none can be authored for it. An uplink packet that SDAP already
// attributed to a QoS flow keeps that QFI on its way into the core network; uplink
// traffic from a stack without SDAP is classified by a built-in DSCP-as-QFI
// residual.
//
simple TrafficFlowFilter
{
    parameters:
        @display("i=block/filter");

        string binderModule = default("binder");
        string bearerConfiguratorModule = default("bearerConfigurator");  // Where a core-network instance registers for QFI-rule delivery
        string ownerType @enum(ENODEB,GNODEB,PGW,UPF,UPF_MEC); // Must be one of ENODEB, GNODEB, PGW, UPF, UPF_MEC
        bool fastForwarding = default(true);
    gates:
        input internetFilterGateIn;
        output gtpUserGateOut;
}
