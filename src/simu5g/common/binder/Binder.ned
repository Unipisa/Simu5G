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


package simu5g.common.binder;

//
// The Binder module functions as a global repository of network-related information
// utilized throughout the simulation for various control-plane functionalities. It provides
// a centralized storage and access point for data such as network configurations and
// operational parameters.
//
// One instance serves one cellular network: the base stations, UEs and core-network
// entities that share a node-id (MacNodeId) space, and with it the cell associations,
// dual-connectivity relations, X2 peerings and multicast groups keyed by those ids.
// The module is always reached through module paths supplied in parameters
// (binderModule), never through hardcoded ones, so one simulation may hold several
// cellular networks side by side, each with its own Binder and ~BearerConfigurator,
// provided every module references those of its own network.
//
simple Binder
{
    parameters:
        int blerShift = default(0);
        double maxDataRatePerRb @unit("Mbps") = default(1.16Mbps);
        bool printTrafficGeneratorConfig = default(false);
        // Notification (not a statistic): carries the id of a node that has just been
        // unregistered, so that holders of per-node state outside this module -- notably the
        // optional D2D layer, which the core must not reference -- can purge it.
        @signal[nodeUnregistered](type=long);
        // Notification (not a statistic): carries the id of a UE whose serving node has just
        // changed, so that state following the UE's attachment -- the downlink tunnels of its
        // PDU sessions, see ~BearerConfigurator -- changes at the same instant.
        @signal[servingNodeChanged](type=long);
        @display("i=block/cogwheel");
}
