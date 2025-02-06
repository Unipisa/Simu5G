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

package simu5g.apps.cbr;

import inet.applications.contract.IApp;

//
// This module receives Constant Bit Rate (CBR) traffic sent by a ~CbrSender
// application module. It binds to a specified local port and records
// statistics such as packet delay, jitter, packet loss, throughput, and the
// total amount of bytes received.
//
simple CbrReceiver like IApp
{
    parameters:
        int localPort = default(3000);

        @signal[cbrFrameDelay](type="simtime_t");
        @signal[cbrJitter](type="simtime_t");
        @signal[cbrFrameLoss](type="double");
        @signal[cbrReceivedThroughput];
        @signal[cbrReceivedBytes];
        @statistic[cbrFrameDelay](title="Cbr Frame Delay"; unit="s"; source="cbrFrameDelay"; record=mean,vector);
        @statistic[cbrJitter](title="Cbr Playout Loss"; unit="s"; source="cbrJitter"; record=mean,vector);
        @statistic[cbrFrameLoss](title="Cbr Frame Loss"; unit="ratio"; source="cbrFrameLoss"; record=mean);
        @statistic[cbrReceivedThroughput](title="Throughput received at the application level"; unit="Bps"; source="cbrReceivedThroughput"; record=timeavg,mean,sum);
        @statistic[cbrReceivedBytes](title="Bytes received at the application level"; unit="Bps"; source="cbrReceivedBytes"; record=timeavg,mean,vector,sum);



        @signal[cbrRcvdPkt];
        @statistic[cbrRcvdPkt](title="Received packet ID"; unit=""; source="cbrRcvdPkt"; record=vector);

        @display("i=block/source");
    gates:
        output socketOut;
        input socketIn;
}

