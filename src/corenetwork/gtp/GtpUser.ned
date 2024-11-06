//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.corenetwork.gtp;

//
// This module implements a simplified version of the user plane of the GPRS Tunneling
// Protocol (GTP). On one hand, this module encapsulates incoming IP datagrams into GTP
// packets and sends them to the tunnel destination endpoint (GTP operates on top of
// UDP). The destination endpoint is identified by utilizing information from the
// ~TrafficFlowFilter module. On the other hand, when this module receives GTP packets
// from a tunnel source endpoint, it decapsulates the included IP datagram and sends it
// to the intended destination according to normal IP forwarding.
// This module is used by all entities that need to communicate within the cellular core
// network, such as ~eNodeB, ~gNodeB, ~PgwStandard, ~Upf, and ~MECHost modules.
//
simple GtpUser
{
    parameters:
        string binderModule = default("binder");
        string interfaceTableModule;            // path to the InterfaceTable module
        string ipOutInterface = default("");    // optional: manual specification of outgoing interface for received IP
        int localPort = default(31);
        int tunnelPeerPort = default(31);
        string gateway = default("");			// optional

        @display("i=block/tunnel");

    gates:
        output socketOut;
        input socketIn;

        input trafficFlowFilterGate;
        output pppGate;
}

