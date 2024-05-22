
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

package simu5g.x2;

import inet.applications.sctpapp.SctpServer;

// X2AppServer
//
// This module extends the SctpServer application, by adding
// an input gate to receive messages from the X2 Manager module.
//
// The X2AppServer module is responsible for handling messages
// from the X2 Manager, forwarding them to the SCTP layer for
// transmission. It ensures that the messages are properly encapsulated
// in SCTP packets before sending.
//
simple X2AppServer extends SctpServer
{
    parameters:
        @class("X2AppServer");
        string binderModule = default("binder");

    gates:
        input x2ManagerIn @labels(X2Msg);
}

