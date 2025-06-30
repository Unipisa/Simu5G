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
// This module provides an application model to be run at a User Equipment (UE), which
// generates periodic requests to be sent to a ~MecResponseApp module hosted on an (edge)
// host, located next to a base station. For each request, it expects to receive a
// response from the ~MecResponseApp. This module records statistics about the Round-trip Time (RTT).
// Optionally, the effect of migration can be simulated when the User Equipment (UE)
// performs a handover to a different base station. In this case, the application can be
// configured to forward new requests to a different (edge) host, namely, the one located next to
// the new base station.
//
simple MecRequestApp like IApp
{
    parameters:
        @display("i=block/source");

        string phyModule = default("^.cellularNic.nrPhy");
        int localPort = default(4000);
        int destPort = default(4000);
        string destAddress;
        volatile int packetSize @unit(B) = intuniform(8000B,12000B);
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value

        double period @unit("s") = default(1s);
        double startTime @unit("s") = default(0s);
        double stopTime @unit("s") = default(200s);

        int appId = default(1);
        bool enableMigration = default(false);

        @signal[requestSize];
        @statistic[requestSize](title="Request packet size"; unit=""; source="requestSize"; record=stats,vector);
        @signal[requestRTT];
        @statistic[requestRTT](title="Round trip time"; unit=""; source="requestRTT"; record=stats,vector);
        @signal[recvResponseSno];
        @statistic[recvResponseSno](title="Received Sequence number"; unit=""; source="recvResponseSno"; record=vector);
    gates:
        output socketOut;
        input socketIn;
}

