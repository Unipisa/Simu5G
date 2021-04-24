/*
 * MeAppBase.h
 *
 *  Created on: Dec 6, 2020
 *      Author: linofex
 */

#ifndef APPS_MEC_MEAPPS_MEAPPBASE_H_
#define APPS_MEC_MEAPPS_MEAPPBASE_H_

#include "inet/applications/tcpapp/TcpAppBase.h"
#include "nodes/mec/MEPlatform/MeServices/packets/HttpRequestMessage/HttpRequestMessage.h"
#include "nodes/mec/MEPlatform/MeServices/packets/HttpResponseMessage/HttpResponseMessage.h"
#include "nodes/mec/MEPlatform/MeServices/packets/HttpMessages_m.h"
/**
 * Base class for clients app for TCP-based request-reply protocols or apps.
 * Handles a single session (and TCP connection) at a time.
 *
 * It needs the following NED parameters: localAddress, localPort, connectAddress, connectPort.
 */

class ResourceManager;
class ServiceRegistry;

class  MeAppBase : public inet::TcpAppBase
{
  protected:

    HttpBaseMessage* currentHttpMessage;
    std::string bufferedData;
    omnetpp::cMessage *sendTimer;
    ResourceManager* resourceManager;

    //references to MeHost module
    cModule* meHost;
    cModule* mePlatform;
    ServiceRegistry * serviceRegistry;

    int mecAppId;
    int requiredRam;
    int requiredDisk;
    double requiredCpu;

    omnetpp::cMessage* processedServiceResponse;

protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void handleMessageWhenUp(omnetpp::cMessage *msg) override;
    virtual void finish() override;

    /* Utility functions */
    virtual void parseReceivedMsg(std::string& packet);
    virtual void handleTimer(omnetpp::cMessage *msg) override {};
    virtual void handleSelfMessage(omnetpp::cMessage *msg) = 0;
    virtual void handleServiceMessage() = 0;
    virtual void established(int connId) = 0;

    /* inet::TcpSocket::CallbackInterface callback methods */
    virtual void socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool urgent) override;
    virtual void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) override { socket->accept(availableInfo->getNewSocketId()); }
    virtual void socketEstablished(inet::TcpSocket *socket) override;
    virtual void socketPeerClosed(inet::TcpSocket *socket) override;
    virtual void socketClosed(inet::TcpSocket *socket) override;
    virtual void socketFailure(inet::TcpSocket *socket, int code) override;
    virtual void socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) override {}
    virtual void socketDeleted(inet::TcpSocket *socket) override {}


    virtual void handleStartOperation(inet::LifecycleOperation *operation) override;
    virtual void handleStopOperation(inet::LifecycleOperation *operation) override;
    virtual void handleCrashOperation(inet::LifecycleOperation *operation) override;

  public:
        MeAppBase();
        virtual ~MeAppBase();


};


#endif /* APPS_MEC_MEAPPS_MEAPPBASE_H_ */
