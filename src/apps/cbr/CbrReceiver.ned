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


simple CbrReceiver like IApp
{
    parameters:
        int localPort = default(3000);

        @signal[cbrFrameDelaySignal](type="simtime_t");
        @signal[cbrJitterSignal](type="simtime_t");
        @signal[cbrFrameLossSignal](type="double");
        @signal[cbrReceivedThroughtputSignal];
        @signal[cbrReceivedBytesSignal];
        @statistic[cbrFrameDelay](title="Cbr Frame Delay"; unit="s"; source="cbrFrameDelaySignal"; record=mean,vector);
        @statistic[cbrJitter](title="Cbr Playout Loss"; unit="s"; source="cbrJitterSignal"; record=mean,vector);
        @statistic[cbrFrameLoss](title="Cbr Frame Loss"; unit="ratio"; source="cbrFrameLossSignal"; record=mean);
        @statistic[cbrReceivedThroughtput](title="Throughput received at the application level"; unit="Bps"; source="cbrReceivedThroughtputSignal"; record=timeavg,mean,sum);
        @statistic[cbrReceivedBytes](title="Bytes received at the application level"; unit="Bps"; source="cbrReceivedBytesSignal"; record=timeavg,mean,vector,sum);
        
        
        
        @signal[cbrRcvdPkt];
        @statistic[cbrRcvdPkt](title="Received packet ID"; unit=""; source="cbrRcvdPkt"; record=vector);
        
        @display("i=block/source");
    gates:
        output socketOut;
        input socketIn;
}

