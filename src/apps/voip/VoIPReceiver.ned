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
// This module represents a model for the receiver of Voice-over-IP (VoIP) traffic.
// It receives packets sent by a ~VoIPSender application via UDP and records statistics
// such as packet loss, end-to-end latency, jitter, and throughput. Furthermore, it computes
// and emits a MOS (Mean Opinion Score) value, which represents the Quality-of-Experience of
// the end users.
//
simple VoIPReceiver like IApp
{
    parameters:
        int localPort = default(3000);
        int emodel_Ie_ = default(5);  // TODO from some ITU standard, figure it out
        int emodel_Bpl_ = default(10);
        int emodel_A_ = default(5);
        double emodel_Ro_ = default(93.2);
        double playout_delay @unit("s") = default(0s);
        int dim_buffer @unit(pk) = default(20pk); // in the number of packets
        double sampling_time @unit("s") = default(0.02s);

        @signal[voIPFrameLoss];
        @statistic[voIPFrameLoss](title="VoIP Frame Loss"; unit="ratio"; source="voIPFrameLoss"; record=mean);
        @signal[voIPFrameDelay];
        @statistic[voIPFrameDelay](title="VoIP Frame Delay"; unit="s"; source="voIPFrameDelay"; record=mean,vector);
        @signal[voIPPlayoutDelay];
        @statistic[voIPPlayoutDelay](title="VoIP Playout Delay"; unit="s"; source="voIPPlayoutDelay"; record=mean,);
        @signal[voIPPlayoutLoss];
        @statistic[voIPPlayoutLoss](title="VoIP Playout Loss"; unit="ratio"; source="voIPPlayoutLoss"; record=mean);
        @signal[voIPJitter];
        @statistic[voIPJitter](title="VoIP Jitter"; unit="s"; source="voIPJitter"; record=mean);
        @signal[voIPMos];
        @statistic[voIPMos](title="VoIP Mos Signal"; unit="MOS"; source="voIPMos"; record=mean);
        @signal[voIPTaildropLoss];
        @statistic[voIPTaildropLoss](title="VoIP Tail Drop Loss"; unit="ratio"; source="voIPTaildropLoss"; record=mean);
        @signal[voIPReceivedThroughput];
        @statistic[voIPReceivedThroughput](title="Throughput received at the application level"; unit="Bps"; source="voIPReceivedThroughput"; record=mean,vector);
        @display("i=block/source");
    gates:
        output socketOut;
        input socketIn;
}

