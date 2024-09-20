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

package simu5g.apps.burst;

import inet.applications.contract.IApp;

//
// This module models an application that sends bursts of packets to a ~BurstReceiver
// application. During each burst, the application sends a configurable number of
// packets at regular intervals. The time interval between bursts can also be configured.
//
simple BurstSender like IApp
{
    parameters:
        int localPort = default(-1);                        // local port to bind the sender (-1 means any port)
        int destPort = default(3000);                       // destination port to which packets will be sent
        string destAddress;                                 // destination address in string format
        double startTime @unit("s") = default(0s);          // starting time of the first burst

        int burstSize = default(10);                        // number of packets to be sent in each burst
        double interBurstTime @unit("s") = default(1s);     // time interval between consecutive bursts

        int packetSize @unit(B) = default(40B);             // size of one packet, in bytes
        double intraBurstTime @unit("s") = default(0.01s);  // time interval between packets within a burst

        int tos = default(-1);                              // Type Of Service/Traffic Class for the sent packets (-1 means no specific TOS/Traffic Class is set)

        @signal[burstSentPkt];
        @statistic[burstSentPkt](title="Sent packet ID"; unit=""; source="burstSentPkt"; record=vector);

        @display("i=block/source");
    gates:
        output socketOut;
        input socketIn;
}
