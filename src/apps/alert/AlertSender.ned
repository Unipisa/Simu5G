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

package simu5g.apps.alert;

import inet.applications.contract.IApp;

//
// This module generates periodic alert messages and sends them to a specified multicast
// group or unicast destination. The packet size, transmission period, start time, and stop
// time are configurable using the corresponding parameters. The destination address must be a
// multicast IP address. This module can be used in simulations that require periodic alert
// generation via device-to-device (D2D) communications, such as vehicular networks.
//
simple AlertSender like IApp
{
    parameters:
        int localPort = default(-1);
        int destPort = default(3000);
        string destAddress;
        int packetSize @unit(B) = default(10B);
        volatile double period @unit("s") = default(0.02s);
        double startTime @unit("s") = default(0s);
        double stopTime @unit("s") = default(0s); // 0 means "never stops"
        string interfaceTableModule;
        string multicastInterface = default("cellular");  // if not empty, set the multicast output interface option on the socket (interface name expected)
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value

        @signal[alertSentMsg];
        @statistic[alertSentMsg](title="Alert messages sent"; unit=""; source="alertSentMsg"; record=sum,vector);

        @display("i=block/source");
    gates:
        output socketOut;
        input socketIn;
}
