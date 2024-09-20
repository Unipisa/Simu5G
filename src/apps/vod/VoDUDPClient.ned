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

//
// This module simulates a video-on-demand (VoD) client application
// that receives video frames over UDP. It is assumed that the transmitted
// video is encoded using multiple layers according to the Scalable Video
// Coding (SVC) technique. Hence, the module collects separate statistics
// (such as throughput and delay) for each layer.
//
simple VoDUDPClient like IApp
{
    parameters:
        int localPort;
        double startStreamTime @unit("s") = default(0s);
        bool startMetrics = default(false);
        string vod_trace_file;
        string bsePath = default("");
        string origVideoYuv = default("");
        string origVideoSvc = default("");
        string decPath = default("");
        string traceType = default("SVC");
        int playbackSize @unit("s") = default(2s);
        string avipluginPath = default("");
        int numFrame = default(0);
        int numPktPerFrame = default(0);
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value

        @display("i=block/app");
        @class(VoDUDPClient);
        @signal[VoDTptLayer0];
        @statistic[VoDTptLayer0](title="VoD Tpt Layer 0"; unit=""; source="VoDTptLayer0"; record=mean,vector);
        @signal[VoDTptLayer1];
        @statistic[VoDTptLayer1](title="VoD Tpt Layer 1"; unit=""; source="VoDTptLayer1"; record=mean,vector);
        @signal[VoDTptLayer2];
        @statistic[VoDTptLayer2](title="VoD Tpt Layer 2"; unit=""; source="VoDTptLayer2"; record=mean,vector);
        @signal[VoDTptLayer3];
        @statistic[VoDTptLayer3](title="VoD Tpt Layer 3"; unit=""; source="VoDTptLayer3"; record=mean,vector);

        @signal[VoDDelayLayer0];
        @statistic[VoDDelayLayer0](title="VoD Delay Layer 0"; unit=""; source="VoDDelayLayer0"; record=mean,vector);
        @signal[VoDDelayLayer1];
        @statistic[VoDDelayLayer1](title="VoD Delay Layer 1"; unit=""; source="VoDDelayLayer1"; record=mean,vector);
        @signal[VoDDelayLayer2];
        @statistic[VoDDelayLayer2](title="VoD Delay Layer 2"; unit=""; source="VoDDelayLayer2"; record=mean,vector);
        @signal[VoDDelayLayer3];
        @statistic[VoDDelayLayer3](title="VoD Delay Layer 3"; unit=""; source="VoDDelayLayer3"; record=mean,vector);


    gates:
        input socketIn @labels(UdpControlInfo/up);
        output socketOut @labels(UdpControlInfo/down);
}

