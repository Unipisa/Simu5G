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

package simu5g.apps.vod;
import inet.applications.contract.IApp;


simple VoDUDPServer like IApp
{
    parameters:
        int localPort; // port to listen on
        string vod_trace_file;
        int fps = default(25);
        string traceType = default("SVC");

        // clients parameters
        string destAddresses;
        int destPort;
        volatile double clientsStartStreamTime = default(0);
        string clientsReqTime = default("");
        double startTime @unit("s") = default(0s);
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value

        @display("i=block/app");
    gates:
        input socketIn @labels(UdpControlInfo/up);
        output socketOut @labels(UdpControlInfo/down);
}
