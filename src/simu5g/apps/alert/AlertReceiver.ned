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
// This module receives alert messages sent by nodes running an ~AlertSender
// application. It listens for incoming packets and logs their arrival.
// It records end-to-end delay statistics. The module also supports multicast by
// joining local multicast groups.
//
simple AlertReceiver like IApp
{
    parameters:
        int localPort = default(3000);
        string interfaceTableModule;
        string multicastInterface = default("cellular");  // if not empty, set the multicast output interface option on the socket (interface name expected)
        @signal[alertDelay];
        @statistic[alertDelay](title="Alert Message Delay"; unit="s"; source="alertDelay"; record=mean,vector);
        @signal[alertRcvdMsg];
        @statistic[alertRcvdMsg](title="Alert Messages Received"; unit="s"; source="alertRcvdMsg"; record=sum,vector);
        @display("i=block/source");
    gates:
        output socketOut;
        input socketIn;
}
