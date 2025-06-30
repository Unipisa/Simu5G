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

#ifndef __MECRESPONSEAPP_H_
#define __MECRESPONSEAPP_H_

#include <queue>

#include <inet/networklayer/common/L3Address.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include "apps/mec/MecApps/MecAppBase.h"
#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h"

namespace simu5g {

using namespace omnetpp;

#define UEAPP_REQUEST      0
#define MECAPP_RESPONSE    1
#define UEAPP_STOP         2
#define UEAPP_ACK_STOP     3

class MECResponseApp : public MecAppBase
{
  protected:
    inet::TcpSocket *mp1Socket_ = nullptr;
    inet::TcpSocket *serviceSocket_ = nullptr;

    inet::UdpSocket ueAppSocket_;
    int localUePort_;

    HttpBaseMessage *mp1HttpMessage = nullptr;

    cMessage *currentRequestfMsg_ = nullptr;
    cMessage *processingTimer_ = nullptr;
    simtime_t msgArrived_;
    simtime_t getRequestSent_;
    simtime_t getRequestArrived_;
    double processingTime_;

    inet::B packetSize_;

    int minInstructions_;
    int maxInstructions_;

    // address+port of the UeApp
    inet::L3Address ueAppAddress;
    int ueAppPort;

    // endpoint for contacting the Location Service
    // this is obtained by sending a GET request to the Service Registry as soon as
    // the connection with the latter has been established
    inet::L3Address serviceAddress_;
    int servicePort_;

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleProcessedMessage(cMessage *msg) override;
    void finish() override;
    void handleSelfMessage(cMessage *msg) override;
    void handleHttpMessage(int connId) override;
    void handleUeMessage(cMessage *msg) override {}

    double scheduleNextMsg(cMessage *msg) override;

    // @brief handler for data received from the service registry
    void handleMp1Message(int connId) override;

    // @brief handler for data received from a MEC service
    void handleServiceMessage(int connId) override;

    virtual void handleRequest(cMessage *msg);
    virtual void handleStopRequest(cMessage *msg);
    virtual void sendStopAck();
    virtual void sendResponse();

    virtual void doComputation();

    // @brief used to require the position of the cars of a platoon to the Location Service
    void sendGetRequest();

    /* TCPSocket::CallbackInterface callback methods */
    void established(int connId) override;
    void socketClosed(inet::TcpSocket *socket) override;

  public:
    ~MECResponseApp() override;
};

} //namespace

#endif

