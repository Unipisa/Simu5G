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

package simu5g.apps.d2dMultihop;

import inet.applications.contract.IApp;

//
// This module implements an application for disseminating event notifications,
// using multihop D2D transmissions. New messages are generated upon the reception
// of external events, signaled by the ~EventGenerator module. The application
// sends the message to an IP multicast address, which is then mapped to D2D
// multicast transmission mode within the LTE/NR protocol stack. When the application
// receives a message, it can relay it to the same IP multicast address in order
// to extend the coverage of the D2D transmission.
// To avoid flooding, two mechanisms can be used:
// - specifying a time-to-live (TTL) for the message, which decreases every time
//   the message is relayed by a UE;
// - specifying a target broadcast radius for the message: a UE relays the message
//   only if it is located within a configurable distance from the originator.
//
// Moreover, if the Trickle suppression mechanism is enabled, the application avoids
// relaying a message when more than 'k' duplicates have already been received.
//
// Every message originated by a node is associated with a *target area*, i.e., the
// set of nodes that should receive the notification. This is used for gathering
// statistics about the dissemination of the message.
//
// Reference: G. Nardini, G. Stea, A. Virdis, "A fast and reliable broadcast service
//      for LTE-Advanced exploiting multihop device-to-device transmissions",
//      Future Internet, 2017, 9(4), 89
//
simple MultihopD2D like IApp
{
    parameters:
        int localPort = default(3000);
        int destPort = default(3000);
        string destAddress = default("0.0.0.0");
        string interfaceName = default("cellular");
        int msgSize @unit(B) = default(40B);
        double maxBroadcastRadius @unit(m) = default(-1m);
        int ttl = default(3);
        double maxTransmissionDelay @unit("s") = default(0.01s);
        double selfishProbability = default(0.0);  // probability that the app does not relay the received message
        string eventGeneratorModule = default("eventGenerator");
        string multihopD2DStatisticsModule = default("d2dMultihopStatistics");

        //# Trickle suppression mechanism parameters
        bool trickle = default(false);
        double I @unit("s") = default(0.01s);
        int k = default(3);

        //# Network Layer specs
        string interfaceTableModule = default(absPath(".interfaceTable"));
        string lteMacModule = default("^.cellularNic.mac");
        string ltePhyModule = default("^.cellularNic.phy");

        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value

        //# Statistics
        @signal[d2dMultihopGeneratedMsg];
        @statistic[d2dMultihopGeneratedMsg](title="Number of locally generated packets"; unit=""; source="d2dMultihopGeneratedMsg"; record=sum,vector);
        @signal[d2dMultihopSentMsg];
        @statistic[d2dMultihopSentMsg](title="Number of sent/relayed packets"; unit=""; source="d2dMultihopSentMsg"; record=sum,vector);
        @signal[d2dMultihopRcvdMsg];
        @statistic[d2dMultihopRcvdMsg](title="Number of received packets"; unit=""; source="d2dMultihopRcvdMsg"; record=sum,vector);
        @signal[d2dMultihopRcvdDupMsg];
        @statistic[d2dMultihopRcvdDupMsg](title="Number of received duplicate packets"; unit=""; source="d2dMultihopRcvdDupMsg"; record=sum,vector);

        @signal[d2dMultihopTrickleSuppressedMsg];
        @statistic[d2dMultihopTrickleSuppressedMsg](title="Number of suppressed packets by the Trickle suppression mechanism"; unit=""; source="d2dMultihopTrickleSuppressedMsg"; record=sum,vector);

        @display("i=block/source");
    gates:
        output socketOut;
        input socketIn;
}
