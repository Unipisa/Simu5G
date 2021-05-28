/*
 * MecAppBase.h
 *
 *  Created on: May 15, 2021
 *      Author: linofex
 */

#ifndef APPS_MEC_MEAPPS_MECAPPBASE_H_
#define APPS_MEC_MEAPPS_MECAPPBASE_H_

#include <omnetpp.h>
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "nodes/mec/MEPlatform/MeServices/packets/HttpRequestMessage/HttpRequestMessage.h"
#include "nodes/mec/MEPlatform/MeServices/packets/HttpResponseMessage/HttpResponseMessage.h"
#include "nodes/mec/MEPlatform/MeServices/packets/HttpMessages_m.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
/**
 * Base class for clients app for TCP-based request-reply protocols or apps.
 * Handles a single session (and TCP connection) at a time.
 *
 * It needs the following NED parameters: localAddress, localPort, connectAddress, connectPort.
 */

class VirtualisationInfrastructureManager;
class ServiceRegistry;

class  MecAppBase : public omnetpp::cSimpleModule, public inet::TcpSocket::ICallback
{
  protected:

    inet::TcpSocket serviceSocket_;
    inet::TcpSocket mp1Socket_;

    inet::L3Address mp1Address;
    int mp1Port;
    inet::L3Address serviceAddress;
    int servicePort;


    // FIXME not used, yet
    omnetpp::cQueue serviceHttpMessages_;
    omnetpp::cQueue mp1HttpMessages_;

    HttpBaseMessage* serviceHttpMessage;
    HttpBaseMessage* mp1HttpMessage;


    std::string bufferedData;

    VirtualisationInfrastructureManager* vim;

    omnetpp::cMessage *sendTimer;

    //references to MeHost module
    omnetpp::cModule* meHost;
    omnetpp::cModule* mePlatform;
    ServiceRegistry * serviceRegistry;

    int mecAppId;
    double requiredRam;
    double requiredDisk;
    double requiredCpu;

    //UDP socket to communicate with the UeApp
    inet::UdpSocket ueSocket;

    omnetpp::cMessage* processedServiceResponse;
    omnetpp::cMessage* processedMp1Response;


protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void handleMessage(omnetpp::cMessage *msg) override;
    virtual void finish() override;

    /* Utility functions */
    virtual void handleSelfMessage(omnetpp::cMessage *msg) = 0;
    virtual void handleServiceMessage() = 0;
    virtual void handleMp1Message() = 0;

    virtual void handleUeMessage(omnetpp::cMessage *msg) = 0;

    virtual void connect(inet::TcpSocket* socket, const inet::L3Address& address, const int port);

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

  public:
        MecAppBase();
        virtual ~MecAppBase();


};




#endif /* APPS_MEC_MEAPPS_MECAPPBASE_H_ */
