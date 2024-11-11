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

#include <inet/networklayer/common/L3Address.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include "nodes/mec/MECPlatform/MEAppPacket_Types.h"
#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h"

#include "apps/mec/MecApps/MecAppBase.h"
#include "apps/mec/RealTimeVideoStreamingApp/packets/RTVideoStreamingPacket_Types.h"
#include "apps/mec/RealTimeVideoStreamingApp/packets/RTVideoStreamingPackets_m.h"

namespace simu5g {

using namespace omnetpp;

class MecRTVideoStreamingReceiver : public MecAppBase
{

    struct ReceivingFrameStatus {
        int frameNumber; // i.e. segment sequence number
        int frameSize;
        int currentSize;
        int numberOfFragments;
        int numberOfFragmentsReceived;
        double playoutTime;
    };

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

    int currentSessionId_ = -1;

    double initialPlayoutDelay;
    cMessage *displayFrame = nullptr;

    bool firstFrameDisplayed;
    long lastFrameDisplayed_;
    int expectedFrameDisplayed_;
    simtime_t lastfragment_;

    // reference to the UE app module, for statistic purposes (statistics will be recorded at the UE side)
    opp_component_ptr<cModule> ueAppModule_;

    // signals
    static simsignal_t e2eDelaySegmentSignal_;
    static simsignal_t interArrTimeSignal_;
    static simsignal_t segmentSizeSignal_;
    static simsignal_t frameSizeSignal_;
    static simsignal_t frameDisplayedSignal_;

    static simsignal_t playoutBufferLengthSignal_;
    static simsignal_t playoutDelayTimeSignal_;
    static simsignal_t playoutDelayTimeAllSignal_;
    static simsignal_t segmentLossSignal_;

    static simsignal_t startSessionSignal_;
    static simsignal_t stopSessionSignal_;

  protected:
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;
    void finish() override;

    void handleHttpMessage(int connId) override {};
    void handleServiceMessage(int connId) override {};
    void handleMp1Message(int connId) override {};
    void established(int connId) override {};
    void handleSelfMessage(cMessage *msg) override;
    void handleUeMessage(cMessage *msg) override;

    void handleStartMessage(cMessage *msg);
    void handleStopMessage(cMessage *msg);
    void handleSessionStartMessage(cMessage *msg);
    void handleSessionStopMessage(cMessage *msg);

    double playoutFrame();
    void processPacket(inet::Packet *packet);

  public:
    ~MecRTVideoStreamingReceiver() override;
};

} //namespace

#endif /* APPS_MEC_MECRTVIDEOSTREAMINGRECEIVER_H_ */

