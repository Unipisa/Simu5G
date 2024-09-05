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

package simu5g.apps.mecRequestResponseApp;

import inet.applications.contract.IApp;

//
// This module provides a simplified model of an edge application, and it is intended
// to run in a (edge) host associated to a base station. It receives requests from a
// User Equipment (UE) application, e.g. a ~MecRequestApp, and sends a response after 
// some processing time. The latter is computed based on the size of request packet
// and a value representing the computation capabilities of the node hosting this
// application module.
// If the UE sending the request is not served by the base station associated to this
// application, then an additional delay to the response is considered, i.e. representing
// the time needed to traverse the cellular core network.
//
simple MecResponseApp like IApp
{
    parameters:
        @display("i=block/app;is=s");

        int localPort = default(5000);
        double coreNetworkDelay @unit("s") = default(0.1s);
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value

        @signal[recvRequestSno];
        @statistic[recvRequestSno](title="Received Sequence number"; unit=""; source="recvRequestSno"; record=vector);

    gates:
        output socketOut;
        input socketIn;
}

