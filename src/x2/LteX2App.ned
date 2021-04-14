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

// LteX2App
//
// This module represents the X2 application, which works on top of the 
// Sctp transport protocol (see 3GPP TS 36.423)
// It contains two submodules:
// - a server that sends data to the peering X2App
// - a client that receive data from the peering X2App
// 
// An eNodeB has one X2App module for each peering eNodeB (see eNodeB.ned).
// Since Sctp is connection-oriented, the client submodule starts the association
// by sending INIT message at given time instant (configurable from .ini) to the 
// peer's server module.
// Once the connection has been established, the server submodule receives messages
// from the X2 Manager and send them to the peer (via Sctp). The client submodule receives the 
// message and forward it to the X2 Manager for processing.
//
module LteX2App
{
    parameters:
        @display("i=block/app;bgb=258,148");

    gates:
        input sctpIn[2] @labels(SctpCommand/up);
        output sctpOut[2] @labels(SctpCommand/down);

        input x2ManagerIn @labels(X2Msg);
        output x2ManagerOut @labels(X2Msg);

    submodules:
        client: X2AppClient {
            @display("i=block/app;p=92,76");
        }
        server: X2AppServer {
            @display("i=block/app;p=163,76");
        }
        
    connections:

        //# client connections
        sctpIn[0] --> client.socketIn;
        sctpOut[0] <-- client.socketOut;

		//# server connections
        sctpIn[1] --> server.socketIn;
        sctpOut[1] <-- server.socketOut;

        //# X2 manager connections
        x2ManagerIn --> server.x2ManagerIn;
        x2ManagerOut <-- client.x2ManagerOut;
}

