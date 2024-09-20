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

//
// This module represents the container for all X2 applications that an e/gNodeB instantiates
// to enable X2 communications with its peer e/gNodeBs. Specifically, for each peer,
// this module contains two submodules:
// - a server that sends data to the peer X2App
// - a client that receives data from the peer X2App
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

