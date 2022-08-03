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

#define UEAPP_REQUEST 0
#define MECAPP_RESPONSE 1
#define UEAPP_STOP 2
#define UEAPP_ACK_STOP 3


#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "common/binder/Binder.h"


using namespace omnetpp;

class UERequestApp: public cSimpleModule
{
    //communication to device app and mec app
    inet::UdpSocket socket;

    unsigned int sno_;
    int requestPacketSize_;
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
    cMessage *selfStart_;
    cMessage *selfStop_;
    cMessage *sendRequest_;
    cMessage *unBlockingMsg_; //it prevents to stop the send/response pattern if msg gets lost


    // signals for statistics
    simsignal_t processingTime_;
    simsignal_t serviceResponseTime_;
    simsignal_t upLinkTime_;
    simsignal_t downLinkTime_;
    simsignal_t responseTime_;

  public:
    ~UERequestApp();
    UERequestApp();

  protected:

    virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
    void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
    virtual void finish();

    void emitStats();

    // --- Functions to interact with the DeviceApp --- //
    void sendStartMECRequestApp();
    void sendStopMECRequestApp();
    void handleStopApp(cMessage* msg);
    void sendStopApp();

    void handleAckStartMECRequestApp(cMessage* msg);
    void handleAckStopMECRequestApp(cMessage* msg);

    // --- Functions to interact with the MECPlatooningApp --- //
    void sendRequest();
    void recvResponse(cMessage* msg);

};

#endif
