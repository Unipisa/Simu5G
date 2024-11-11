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

#ifndef __UEREQUESTAPP_H_
#define __UEREQUESTAPP_H_

#include <inet/networklayer/common/L3Address.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>

#include "common/binder/Binder.h"

namespace simu5g {

using namespace omnetpp;

enum {
    UEAPP_REQUEST   = 0,
    MECAPP_RESPONSE = 1,
    UEAPP_STOP      = 2,
    UEAPP_ACK_STOP  = 3,
};

class UERequestApp : public cSimpleModule
{
    //communication to device app and mec app
    inet::UdpSocket socket;

    unsigned int sno_;
    inet::B requestPacketSize_;
    double requestPeriod_;

    simtime_t start_;
    simtime_t end_;

    // DeviceApp info
    int localPort_;
    int deviceAppPort_;
    inet::L3Address deviceAppAddress_;

    // MEC application endPoint (returned by the device app)
    inet::L3Address mecAppAddress_;
    int mecAppPort_;

    std::string mecAppName;

    //scheduling
    enum MsgKind {
        KIND_SELF_START = 1000,
        KIND_SELF_STOP,
        KIND_SEND_REQUEST,
        KIND_UN_BLOCKING_MSG
    };
    cMessage *selfStart_ = nullptr;
    cMessage *selfStop_ = nullptr;
    cMessage *sendRequest_ = nullptr;
    cMessage *unBlockingMsg_ = nullptr; //it prevents to stop the send/response pattern if msg gets lost

    // signals for statistics
    static simsignal_t processingTimeSignal_;
    static simsignal_t serviceResponseTimeSignal_;
    static simsignal_t upLinkTimeSignal_;
    static simsignal_t downLinkTimeSignal_;
    static simsignal_t responseTimeSignal_;

  public:
    ~UERequestApp() override;

  protected:

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;
    void finish() override;

    void emitStats();

    // --- Functions to interact with the DeviceApp --- //
    void sendStartMECRequestApp();
    void sendStopMECRequestApp();
    void handleStopApp(cMessage *msg);
    void sendStopApp();

    void handleAckStartMECRequestApp(cMessage *msg);
    void handleAckStopMECRequestApp(cMessage *msg);

    // --- Functions to interact with the MECPlatooningApp --- //
    void sendRequest();
    void recvResponse(cMessage *msg);

};

} //namespace

#endif

