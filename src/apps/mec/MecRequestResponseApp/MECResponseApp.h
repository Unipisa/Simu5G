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

#include "omnetpp.h"

#include <queue>

#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "apps/mec/MecApps/MecAppBase.h"
#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h"

#define UEAPP_REQUEST 0
#define MECAPP_RESPONSE 1

#define UEAPP_STOP 2
#define UEAPP_ACK_STOP 3


class MECResponseApp : public MecAppBase
{
protected:
    inet::TcpSocket *mp1Socket_;
    inet::TcpSocket *serviceSocket_;

    inet::UdpSocket ueAppSocket_;
    int localUePort_;

    HttpBaseMessage* mp1HttpMessage;

    cMessage* currentRequestfMsg_;
    cMessage* processingTimer_;
    simtime_t msgArrived_;
    simtime_t getRequestSent_;
    simtime_t getRequestArrived_;
    double processingTime_;

    int packetSize_;

    int minInstructions_;
    int maxInstructions_;

    // address+port of the UeApp
    inet::L3Address ueAppAddress;
    int ueAppPort;


    omnetpp::cOutVector rt_stats_mec;

    // endpoint for contacting the Location Service
    // this is obtained by sending a GET request to the Service Registry as soon as
    // the connection with the latter has been established
    inet::L3Address serviceAddress_;
    int servicePort_;


    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleProcessedMessage(cMessage *msg) override;
    virtual void finish() override;
    virtual void handleSelfMessage(cMessage *msg) override;
    virtual void handleHttpMessage(int connId) override;
    virtual void handleUeMessage(omnetpp::cMessage *msg) override {}

    virtual double scheduleNextMsg(cMessage* msg) override;

    // @brief handler for data received from the service registry
    virtual void handleMp1Message(int connId) override;

    // @brief handler for data received from a MEC service
    virtual void handleServiceMessage(int connId) override;

    virtual void handleRequest(cMessage* msg);

    virtual void handleStopRequest(cMessage* msg);
    virtual void sendStopAck();

    virtual void sendResponse();

    virtual void doComputation();

    // @brief used to require the position of the cars of a platoon to the Location Service
    void sendGetRequest();

    /* TCPSocket::CallbackInterface callback methods */
    virtual void established(int connId) override;
    virtual void socketClosed(inet::TcpSocket *socket) override;


  public:
    MECResponseApp();
    virtual ~MECResponseApp();
};

#endif
