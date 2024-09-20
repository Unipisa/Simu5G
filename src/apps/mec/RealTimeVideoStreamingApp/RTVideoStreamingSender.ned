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


package simu5g.apps.mec.RealTimeVideoStreamingApp;

import inet.applications.contract.IApp;

//
// This application is to be instantiated on the UE side. It requires that a ~DeviceApp is
// also instantiated on the UE. This module requests the ~DeviceApp to instantiate a
// ~MecRTVideoStreamingReceiver in the MEC system.
// Once the peer MEC application is up and running, this module starts sending packets
// containing video frames as read from the trace file specified in the configuration.
//
simple RTVideoStreamingSender like IApp
{
    parameters:
        @display("i=block/source");

        string mobilityModule;

        //connection infos
        int localPort = default(4000);
        int deviceAppPort = default(4000); // ipAddress of the DeviceApp
        string deviceAppAddress; // port of the DeviceApp

        int packetSize @unit(B) = default(10B);

        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value

        //autoscheduling infos
        double period @unit("s") = default(1s);
        double startTime @unit("s") = default(0s);
        double stopTime @unit("s") = default(120s);

        string mecAppName = default("MecrtVideoStreamingReceiverApp"); // but could be MEWarningAlertApp_rest_External

        // session parameters
        volatile double sessionDuration @unit("s") = default(uniform(30s,120s));     // distribution?
        volatile double periodBetweenSession @unit("s") = default(uniform(10s,60s)); // distribution?

        string fileName;
        bool sendAllOnOneTime = true;

        @signal[positionX](type=double);
        @statistic[positionX](title="UE positionX"; source=positionX; record=vector);
        @signal[positionY](type=double);
        @statistic[positionY](title="UE positionY"; source=positionY; record=vector);
        @signal[positionZ](type=double);
        @statistic[positionZ](title="UE positionY"; source=positionZ; record=vector);

        @signal[velocity](type=double);
        @statistic[velocity](title="UE velocity"; source=velocity; record=vector);

        @signal[rtVideoStreamingFirstFrameElapsedTime](type=simtime_t);
        @statistic[rtVideoStreamingFirstFrameElapsedTime](title="Frames displayed"; source="rtVideoStreamingFirstFrameElapsedTime"; record=vector; interpolationmode=none);

        @signal[rtVideoStreamingFramesDisplayed](type=double);
        @statistic[rtVideoStreamingFramesDisplayed](title="Frames displayed"; source="rtVideoStreamingFramesDisplayed"; record=count,vector; interpolationmode=none);

        @signal[rtVideoStreamingEnd2endDelaySegment](type=simtime_t);
        @statistic[rtVideoStreamingEnd2endDelaySegment](title="end to end delay of segment packets"; source="rtVideoStreamingEnd2endDelaySegment"; record=mean,vector);

        @signal[rtVideoStreamingSegmentPacketSize](type=int);
        @statistic[rtVideoStreamingSegmentPacketSize](title="size of segment packets"; source="rtVideoStreamingSegmentPacketSize"; record=mean,vector);

        @signal[rtVideoStreamingFrameSize](type=int);
        @statistic[rtVideoStreamingFrameSize](title="size of frames"; source="rtVideoStreamingFrameSize"; record=mean,vector);

        @signal[rtVideoStreamingPlayoutBufferLength](type=int);
        @statistic[rtVideoStreamingPlayoutBufferLength](title="playout buffer Length"; source="rtVideoStreamingPlayoutBufferLength"; record=mean,vector);

        @signal[rtVideoStreamingInterArrivalTimeSegment](type=simtime_t);
        @statistic[rtVideoStreamingInterArrivalTimeSegment](title="inter arrival of segment packets"; source="rtVideoStreamingInterArrivalTimeSegment"; record=mean,vector);

        @signal[rtVideoStreamingPlayoutDelay](type=simtime_t);
        @statistic[rtVideoStreamingPlayoutDelay](title="playout delay"; source="rtVideoStreamingPlayoutDelay"; record=mean,vector);

        @signal[rtVideoStreamingPlayoutDelayAll](type=simtime_t);
        @statistic[rtVideoStreamingPlayoutDelayAll](title="playout delay"; source="rtVideoStreamingPlayoutDelayAll"; record=mean,vector);

        @signal[rtVideoStreamingSegmentLoss](type=int);
        @statistic[rtVideoStreamingSegmentLoss](title="segment packets lost"; source="rtVideoStreamingSegmentLoss"; record=mean,vector; interpolationmode=none);

        @signal[rtVideoStreamingStartSession](type=simtime_t);
        @statistic[rtVideoStreamingStartSession](title="start session timestamps"; source="rtVideoStreamingStartSession"; record=mean,vector; interpolationmode=none);

        @signal[rtVideoStreamingStopSession](type=simtime_t);
        @statistic[rtVideoStreamingStopSession](title="stop session timestamps"; source="rtVideoStreamingStopSession"; record=mean,vector; interpolationmode=none);



    gates:
        output socketOut;
        input socketIn;
}

