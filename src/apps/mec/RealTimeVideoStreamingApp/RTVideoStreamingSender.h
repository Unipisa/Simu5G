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

#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "common/binder/Binder.h"

//inet mobility
#include "inet/common/geometry/common/Coord.h"
#include "inet/common/geometry/common/EulerAngles.h"
#include "inet/mobility/contract/IMobility.h"

//WarningAlertPacket
#include "nodes/mec/MECPlatform/MEAppPacket_Types.h"
#include "apps/mec/WarningAlert/packets/WarningAlertPacket_m.h"


using namespace omnetpp;

typedef struct
{
    int frameNumber;
    int frameSize;
    int remainingFragments;
    int numberOfFragments;
    int bytesPerPacket;
    int pictureType;
    uint32_t rtpTimestamp;

    int remainingFrameBytes;
    int remainingFramesSlices;

} FragmentedFrameStatus;


/**
 * This is a UE app that asks to a Device App to instantiate the MECRTVideoStreamingReceiver app
 * @author Alessandro Noferi
 */

class RTVideoStreamingSender: public cSimpleModule
{
    //communication to device app and Mec app
    inet::UdpSocket socket;

    int size_;
    simtime_t lifeCyclePeriod_;
    int localPort_;
    int deviceAppPort_;
    inet::L3Address deviceAppAddress_;

    char* sourceSimbolicAddress;            //Ue[x]
    char* deviceSimbolicAppAddress_;        //meHost.virtualisationInfrastructure

    // MEC application endPoint (returned by the device app)
    inet::L3Address mecAppAddress_;
    int mecAppPort_;

    std::string mecAppName;

    //scheduling
    cMessage *selfRTVideoStreamingAppStart_;
    cMessage *selfRTVideoStreamingAppStop_;

    cMessage *selfMecAppStart_;
    cMessage *selfMecAppStop_;

    cMessage* selfSessionStart_;
    cMessage* selfSessionStop_;

    bool stop_;

    inet::UdpSocket videoStreamSocket_;
    int port;

    const char* fileName;
    /**
     * The input file stream for the data file.
     */
    std::ifstream _inputFileStream;

    bool pendingStartRequest;
    bool moduleCreated;

    int mtu_;
    cMessage* _nextFrame;

    /**
     * duration of each session
     */
    double sessionDuration_;

    /**
     * period between sessions
     */
    double periodBetweenSession_;

    /**
     * The initial delay of the mpeg video.
     */
    double _initialDelay;

    /**
     * The number of frames per second of the video.
     */
    double _framesPerSecond;

    /**
     * The number of the current frame. Needed for calculating
     * the rtp time stamp in the rtp data packets.
     */
    unsigned long _frameNumber;
    unsigned long _sequenceNumber;

    int sessionId_;

    bool sendAllOnOneTime_;

    // mobility informations
    cModule* ue;
    inet::IMobility *mobility;
    inet::Coord position;
    omnetpp::cMessage *mobilityStats_;

    omnetpp::simsignal_t positionSignalX;
    omnetpp::simsignal_t positionSignalY;
    omnetpp::simsignal_t positionSignalZ;

    omnetpp::simsignal_t velocitySignal;
    double mobilityUpdateInterval_; // send pos and speed info

    FragmentedFrameStatus fragFrameStatus_;

    public:
        ~RTVideoStreamingSender();
        RTVideoStreamingSender();

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);
        virtual void finish();

        // communication with device app
        void sendStartMECApp();
        void sendStopMECApp();
        // communication with mec app
        void sendMessage();
        void sendStartMessage();
        void sendStopMessage();
        void sendSessionStopMessage();
        void sendSessionStartMessage();


        // handlers
        void handleStartAck(cMessage* msg);
        void handleStartNack(cMessage* msg);

        void handleStopAck(cMessage* msg);
        void handleStopNack(cMessage* msg);

        void handleStartSessionAck(cMessage* msg);
        void handleStartSessionNack(cMessage* msg);

        void handleStopSessionAck(cMessage* msg);
        void handleStopSessionNack(cMessage* msg);

        void handleAckStartMECApp(cMessage* msg);
        void handleInfoMECApp(cMessage* msg);
        void handleAckStopMECApp(cMessage* msg);

        void openFileStream();
        void initializeVideoStream();
};


#endif /* APPS_MEC_RTVIDEOSTREAMINGSENDER_H_ */
