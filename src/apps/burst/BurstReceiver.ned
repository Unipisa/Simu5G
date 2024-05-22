
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
// This module implements a BurstReceiver which handles incoming packets
// by binding to a specified local UDP port. It logs received packets
// and computes the delay of each packet.
//
// Parameters:
// - localPort: the local UDP port to bind to (default 3000)
//
// Signals:
// - burstRcvdPkt: emitted when a packet is received, carrying the packet ID
// - burstPktDelay: emitted when a packet is received, representing the packet's delay
//
// Gates:
// - socketOut: output gate for the socket
// - socketIn: input gate for receiving packets
//
simple BurstReceiver like IApp
{
    parameters:
        int localPort = default(3000);

        @signal[burstRcvdPkt];
        @statistic[burstRcvdPkt](title="Received packet ID"; unit=""; source="burstRcvdPkt"; record=vector);
        @signal[burstPktDelay];
        @statistic[burstPktDelay](title="Delay of received packets"; unit="s"; source="burstPktDelay"; record=mean,vector);

        @display("i=block/source");
    gates:
        output socketOut;
        input socketIn;
}

