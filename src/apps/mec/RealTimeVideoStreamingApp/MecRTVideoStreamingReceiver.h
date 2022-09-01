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

#ifndef APPS_MEC_MECRTVIDEOSTREAMINGRECEIVER_H_
#define APPS_MEC_MECRTVIDEOSTREAMINGRECEIVER_H_

#include "omnetpp.h"

#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "nodes/mec/MECPlatform/MEAppPacket_Types.h"
#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h"

#include "apps/mec/RealTimeVideoStreamingApp/packets/RTVideoStreamingPacket_Types.h"
#include "apps/mec/RealTimeVideoStreamingApp/packets/RTVideoStreamingPackets_m.h"
#include "apps/mec/MecApps/MecAppBase.h"


class MecRTVideoStreamingReceiver : public MecAppBase
{

    typedef struct
    {
        int frameNumber; // i.e. segment sequence number
        int frameSize;
        int currentSize;
        int numberOfFragments;
        int numberOfFragmentsReceived;
        double playoutTime;
    } ReceivingFrameStatus;

//    double playoutDelay_;
//
//    cMessage* displayTimer_;
//
//    ReceivingFrameStatus recFrameStatus_;

    std::map<uint32_t, ReceivingFrameStatus> playoutBuffer_;

    bool dropPackets_;
    bool stopped;
    int fps;

    //UDP socket to communicate with the UeApp
    inet::UdpSocket ueSocket;
    int localUePort;

    inet::L3Address ueAppAddress;
    int ueAppPort;

    int size_;

    int currentSessionId_;

    double initialPlayoutDelay;
    cMessage *displayFrame;


    bool firstFrameDisplayed ;
    long lastFrameDisplayed_ ;
    int expectedFrameDisplayed_ ;
    simtime_t lastfragment_;

    // reference to the UE app module, for statistic purposes (statistics will be recorded at the UE side)
    cModule* ueAppModule_;

    // signals
    simsignal_t e2eDelaySegment_;
    simsignal_t interArrTime_;
    simsignal_t segmentSize_;
    simsignal_t frameSize_;
    simsignal_t frameDisplayed_;

    simsignal_t playoutBufferLength_;
    simsignal_t playoutDelayTime_;
    simsignal_t playoutDelayTimeAll_;
    simsignal_t segmentLoss_;

    simsignal_t startSession_;
    simsignal_t stopSession_;

  protected:
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

//    virtual void handleProcessedMessage(cMessage *msg) override;

    virtual void handleHttpMessage(int connId) override {};
    virtual void handleServiceMessage(int connId) override {};
    virtual void handleMp1Message(int connId) override {};
    virtual void established(int connId) override {};
    virtual void handleSelfMessage(cMessage *msg) override;
    virtual void handleUeMessage(omnetpp::cMessage *msg) override;

    void handleStartMessage(omnetpp::cMessage *msg);
    void handleStopMessage(omnetpp::cMessage *msg);
    void handleSessionStartMessage(omnetpp::cMessage *msg);
    void handleSessionStopMessage(omnetpp::cMessage *msg);

    double playoutFrame();
    void processPacket(inet::Packet *packet);

  public:
    MecRTVideoStreamingReceiver();
    virtual ~MecRTVideoStreamingReceiver();
};



#endif /* APPS_MEC_MECRTVIDEOSTREAMINGRECEIVER_H_ */
