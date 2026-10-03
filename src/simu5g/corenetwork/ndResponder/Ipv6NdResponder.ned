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

package simu5g.corenetwork.ndResponder;

//
// Answers the IPv6 Neighbor Discovery traffic of the UEs' sessions at the
// session anchor (~Upf, ~PgwStandard), which is the router on the IP link of an
// IPv6 session: the base station is transparent to it. In 5GS the SMF generates
// the Router Advertisements and the UPF relays them into the session's tunnel,
// together with the UE's Router Solicitations the other way (TS 23.502); this
// module stands in for both.
//
// ~GtpUser hands over every uplink packet whose destination has link-local scope
// (fe80::/10, or a multicast group of interface-local or link-local scope), so no
// such packet leaks onto the data network: through the node's message dispatcher, as
// the icmpv6 service, which this module registers, each packet tagged with its session.
// This module
//  - answers a Router Solicitation with a Router Advertisement, unicast to the
//    soliciting UE, which makes this module's link-local address the UE's default
//    router;
//  - answers a Neighbor Solicitation for its link-local address (a reachability
//    probe) with a solicited Neighbor Advertisement;
//  - consumes duplicate address detection probes without an answer, meaning the
//    address is free;
//  - discards everything else with a warning.
// The replies go back up through the dispatcher, tagged with the session of the
// solicitation, which names the tunnel they go into (see ~TrafficFlowFilter).
//
// Simplifications: the UEs' addresses are configured statically (see
// ~Ipv6NetworkConfigurator), so the Router Advertisement carries the session's
// prefix (the /64 of the UE's global address) for information only, with neither
// the on-link nor the autonomous flag; and no unsolicited Router Advertisements
// are sent, the router lifetime is meant to outlast the simulation.
//
simple Ipv6NdResponder
{
    parameters:
        @display("i=block/control");
        string binderModule = default("binder");
        string linkLocalAddress = default("fe80::1");  // The router's address on every session's link
        double routerLifetime @unit(s) = default(9000s);  // Router Lifetime in the Router Advertisements (max 9000s, RFC 4861)
        int curHopLimit = default(64);  // Cur Hop Limit in the Router Advertisements (0 = unspecified)
        string checksumMode @enum("declared", "computed") = default("declared");
    gates:
        input in;
        output out;
}
