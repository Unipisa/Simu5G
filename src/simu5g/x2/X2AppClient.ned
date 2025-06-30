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

import inet.applications.sctpapp.SctpClient;

//
// This module extends INET's SctpClient application by adding
// an output gate to send messages to the ~LteX2Manager and
// manages the connection initiation towards the peer's server module
// based on the configuration specified in the .ini file.
// Once the connection has been established, it receives messages
// from the peer's server module and forwards them to the ~LteX2Manager
// module for further processing and redirection to the intended submodule
// of the LTE/NR protocol stack.
//
simple X2AppClient extends SctpClient
{
    parameters:
        @class("X2AppClient");
        string binderModule = default("binder");
        connectPort @mutable = default(5000); // it will be overwritten by the initialization procedure
        startTime = default(0s);
    gates:
        output x2ManagerOut @labels(X2Msg);
}

