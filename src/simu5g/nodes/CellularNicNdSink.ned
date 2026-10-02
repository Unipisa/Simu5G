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

package simu5g.nodes;

//
// WORKAROUND: takes what the IPv6 layer of a base station sends to the cellular
// NIC, which is no interface of the base station's own network layer: the NIC is
// wired to the user plane (~TrafficFlowFilter, ~GtpUser), not to the link-layer
// dispatcher (nl). The NIC stays in the interface table nevertheless, because the
// address configurator builds the routes of the UEs through it, as their neighbor on
// the cellular link. The network layer's dispatcher maps the NIC to this module
// (see ~eNodeB), which discards IPv6 Neighbor Discovery messages (Router and
// Neighbor Solicitations and Advertisements, Redirects): IPv6 runs them on every
// interface, and they reach no one over the cellular NIC. Any other packet is an
// error.
//
// The proper model, which makes this module unnecessary: a UE's IP link runs to the
// UPF over its PDU session, not to the base station; the address configurator
// cannot express that yet.
//
simple CellularNicNdSink
{
    parameters:
        @display("i=block/sink;is=s");
    gates:
        input in;
        output out;  // Unused: pairs the input at the dispatcher, whose gates come in pairs
}
