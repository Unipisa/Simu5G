
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
// This module implements a simplified version of the GTP-U (GPRS Tunneling Protocol - User Plane) protocol.
// It is used for creating data tunnels between GTP peers in LTE and 5G networks.
// The module handles encapsulation and decapsulation of IP packets for transmission over the core network.
//
// Features:
// - Can communicate with traffic flow filters to receive IP datagrams that need to be tunneled.
// - Utilizes UDP sockets for GTP-U communication with peer nodes.
// - Supports configurable local and peer tunnel ports.
// - Allows optional specification of outgoing network interface.
//
simple GtpUser
{
    parameters:
        string binderModule = default("binder");
        string interfaceTableModule;            // path to the InterfaceTable module
        string ipOutInterface = default("");    // optional: manual specification of outgoing interface for received IP
        int localPort = default(31);
        int tunnelPeerPort = default(31);
        bool filter = default(true);

        @display("i=block/tunnel");

    gates:
        output socketOut;
        input socketIn;

        input trafficFlowFilterGate;
        output pppGate;
}

