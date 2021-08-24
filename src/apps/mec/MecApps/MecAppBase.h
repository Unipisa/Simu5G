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


#ifndef APPS_MEC_MEAPPS_MECAPPBASE_H_
#define APPS_MEC_MEAPPS_MECAPPBASE_H_

#include <omnetpp.h>
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpRequestMessage/HttpRequestMessage.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpMessages_m.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"

#include "nodes/mec/VirtualisationInfrastructureManager/VirtualisationInfrastructureManager.h"
#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h"

/*
 * This is a case class for rapidly implementation a MEC app. It is supposed that the MEC app
 * consumes only one MEC service. It manages socket connections, HTTP message parsing and
 * simulates processing time to execute block of codes.
 * Use this app when HTTP messages arrives in a synchronous way and so there is not the need of
 * queuing them during other processing.
 * A MECAppBase that store HTTP messages in a queue is planned to be implemented.
 *
 * To develop a real MEC app, it is only necessary to implement the abstract methods related to
 * the logic behavior.
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

    // FIXME not used, yet. These structures are supposed to be used
    omnetpp::cQueue serviceHttpMessages_;
    omnetpp::cQueue mp1HttpMessages_;

    HttpBaseMessage* serviceHttpMessage;
    HttpBaseMessage* mp1HttpMessage;


    std::string bufferedData;

    VirtualisationInfrastructureManager* vim;

    omnetpp::cMessage *sendTimer;

    //references to MeHost module
    omnetpp::cModule* mecHost;
    omnetpp::cModule* mecPlatform;
    ServiceRegistry * serviceRegistry;

    int mecAppId;
    double requiredRam;
    double requiredDisk;
    double requiredCpu;


    omnetpp::cMessage* processedServiceResponse;
    omnetpp::cMessage* processedMp1Response;


protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void handleMessage(omnetpp::cMessage *msg) override;
    virtual void finish() override;

    /* Method to be implemented for real MEC apps */
    virtual void handleSelfMessage(omnetpp::cMessage *msg) = 0;
    virtual void handleServiceMessage() = 0;
    virtual void handleMp1Message() = 0;
    virtual void handleUeMessage(omnetpp::cMessage *msg) = 0;
    virtual void established(int connId) = 0;

    virtual void connect(inet::TcpSocket* socket, const inet::L3Address& address, const int port);

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
