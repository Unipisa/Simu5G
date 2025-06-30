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

#ifndef APPS_MEC_RTVIDEOSTREAMINGSENDER_H_
#define APPS_MEC_RTVIDEOSTREAMINGSENDER_H_

#include <inet/common/ModuleRefByPar.h>
#include <inet/common/geometry/common/Coord.h>
#include <inet/common/geometry/common/EulerAngles.h>
#include <inet/mobility/contract/IMobility.h>
#include <inet/networklayer/common/L3Address.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>

#include "common/binder/Binder.h"
#include "apps/mec/WarningAlert/packets/WarningAlertPacket_m.h"
#include "nodes/mec/MECPlatform/MEAppPacket_Types.h"

namespace simu5g {

using namespace omnetpp;

struct FragmentedFrameStatus {
    int frameNumber;
    int frameSize;
    int remainingFragments;
    int numberOfFragments;
    int bytesPerPacket;
    int pictureType;
    uint32_t rtpTimestamp;

    int remainingFrameBytes;
    int remainingFramesSlices;
};

/**
 * This is a UE app that asks a Device App to instantiate the MECRTVideoStreamingReceiver app
 * @author Alessandro Noferi
 */

class RTVideoStreamingSender : public cSimpleModule
{
    //communication to device app and MEC app
    inet::UdpSocket socket;

    int size_;
    simtime_t lifeCyclePeriod_;
    int localPort_;
    int deviceAppPort_;
    inet::L3Address deviceAppAddress_;

    // MEC application endPoint (returned by the device app)
    inet::L3Address mecAppAddress_;
    int mecAppPort_;

    std::string mecAppName;

    //scheduling
    enum MsgKind {
        KIND_SELF_RT_VIDEO_STREAMING_APP_START = 1000,
        KIND_SELF_RT_VIDEO_STREAMING_APP_STOP,
        KIND_SELF_MEC_APP_START,
        KIND_SELF_MEC_APP_STOP,
        KIND_SELF_SESSION_START,
        KIND_SELF_SESSION_STOP,
        KIND_SELF_NEXT_FRAME,
        KIND_SELF_MOBILITY_STATS,
    };

    cMessage *selfRTVideoStreamingAppStart_ = nullptr;
    cMessage *selfRTVideoStreamingAppStop_ = nullptr;

    cMessage *selfMecAppStart_ = nullptr;
    cMessage *selfMecAppStop_ = nullptr;

    cMessage *selfSessionStart_ = nullptr;
    cMessage *selfSessionStop_ = nullptr;

    inet::UdpSocket videoStreamSocket_;

    const char *fileName;
    /**
     * The input file stream for the data file.
     */
    std::ifstream _inputFileStream;

    int mtu_;
    cMessage *_nextFrame = nullptr;

    /**
     * The initial delay of the MPEG video.
     */
    double _initialDelay;

    /**
     * The number of frames per second of the video.
     */
    double _framesPerSecond;

    /**
     * The number of the current frame. Needed for calculating
     * the RTP time stamp in the RTP data packets.
     */
    unsigned long _frameNumber = 0;
    unsigned long _sequenceNumber = 0;

    int sessionId_ = 0;

    bool sendAllOnOneTime_;

    // mobility information
    opp_component_ptr<cModule> ue;
    inet::ModuleRefByPar<inet::IMobility> mobility;
    inet::Coord position;
    cMessage *mobilityStats_ = nullptr;

    static simsignal_t positionSignalXSignal_;
    static simsignal_t positionSignalYSignal_;
    static simsignal_t positionSignalZSignal_;

    static simsignal_t velocitySignal_;
    double mobilityUpdateInterval_; // send position and speed info

    FragmentedFrameStatus fragFrameStatus_;

  public:
    ~RTVideoStreamingSender() override;

  protected:

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;
    void finish() override;

    // communication with device app
    void sendStartMECApp();
    void sendStopMECApp();
    // communication with MEC app
    void sendMessage();
    void sendStartMessage();
    void sendStopMessage();
    void sendSessionStopMessage();
    void sendSessionStartMessage();

    // handlers
    void handleStartAck(cMessage *msg);
    void handleStartNack(cMessage *msg);

    void handleStopAck(cMessage *msg);
    void handleStopNack(cMessage *msg);

    void handleStartSessionAck(cMessage *msg);
    void handleStartSessionNack(cMessage *msg);

    void handleStopSessionAck(cMessage *msg);
    void handleStopSessionNack(cMessage *msg);

    void handleAckStartMECApp(cMessage *msg);
    void handleInfoMECApp(cMessage *msg);
    void handleAckStopMECApp(cMessage *msg);

    void openFileStream();
    void initializeVideoStream();
};

} //namespace

#endif /* APPS_MEC_RTVIDEOSTREAMINGSENDER_H_ */

