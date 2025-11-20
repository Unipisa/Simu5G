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

package simu5g.apps.voip;

import inet.applications.contract.IApp;

//
// Represents a model for the receiver of Voice-over-IP (VoIP) traffic.
// It receives packets sent by a ~VoipSender application via UDP and records statistics
// such as packet loss, end-to-end latency, jitter, and throughput. Furthermore, it computes
// and emits a MOS (Mean Opinion Score) value, which represents the Quality-of-Experience of
// the end users.
//
simple VoipReceiver like IApp
{
    parameters:
        int localPort = default(3000);
        int emodel_Ie = default(5);  // TODO from some ITU standard, figure it out
        int emodel_Bpl = default(10);
        int emodel_A = default(5);
        double emodel_Ro = default(93.2);
        double playoutDelay @unit("s") = default(0s);
        int dimBuffer @unit(pk) = default(20pk); // in the number of packets
        double samplingTime @unit("s") = default(0.02s);

        @signal[voipFrameLoss];
        @statistic[voipFrameLoss](title="VoIP Frame Loss"; unit="ratio"; source="voipFrameLoss"; record=mean);
        @signal[voipFrameDelay];
        @statistic[voipFrameDelay](title="VoIP Frame Delay"; unit="s"; source="voipFrameDelay"; record=mean,vector);
        @signal[voipPlayoutDelay];
        @statistic[voipPlayoutDelay](title="VoIP Playout Delay"; unit="s"; source="voipPlayoutDelay"; record=mean,);
        @signal[voipPlayoutLoss];
        @statistic[voipPlayoutLoss](title="VoIP Playout Loss"; unit="ratio"; source="voipPlayoutLoss"; record=mean);
        @signal[voipJitter];
        @statistic[voipJitter](title="VoIP Jitter"; unit="s"; source="voipJitter"; record=mean);
        @signal[voipMos];
        @statistic[voipMos](title="VoIP Mos Signal"; unit="MOS"; source="voipMos"; record=mean);
        @signal[voipTaildropLoss];
        @statistic[voipTaildropLoss](title="VoIP Tail Drop Loss"; unit="ratio"; source="voipTaildropLoss"; record=mean);
        @signal[voipReceivedThroughput];
        @statistic[voipReceivedThroughput](title="Throughput received at the application level"; unit="Bps"; source="voipReceivedThroughput"; record=simu5g_rateavg,vector);
        @display("i=block/source");
    gates:
        output socketOut;
        input socketIn;
}

