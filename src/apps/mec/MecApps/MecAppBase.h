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

#include <inet/common/ModuleRefByPar.h>
#include <inet/common/socket/SocketMap.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/transportlayer/contract/tcp/TcpSocket.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>

#include "nodes/mec/MECPlatform/MECServices/packets/HttpRequestMessage/HttpRequestMessage.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpMessages_m.h"
#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h"
#include "nodes/mec/VirtualisationInfrastructureManager/VirtualisationInfrastructureManager.h"

namespace simu5g {

using namespace omnetpp;

/*
 * This is a case class for rapid implementation of a MEC app. It is supposed that the MEC app
 * consumes only one MEC service. It manages socket connections, HTTP message parsing and
 * simulates processing time to execute block of codes.
 * Use this app when HTTP messages arrive in a synchronous way and so there is not the need of
 * queuing them during other processing.
 * A MECAppBase that stores HTTP messages in a queue is planned to be implemented.
 *
 * To develop a real MEC app, it is only necessary to implement the abstract methods related to
 * the logic behavior.
 */

class VirtualisationInfrastructureManager;
class ProcessingTimeMessage;
class ServiceRegistry;

struct HttpMessageStatus
{
    HttpBaseMessage *currentMessage = nullptr;
    std::string bufferedData;
    cQueue httpMessageQueue;
    ProcessingTimeMessage *processMsgTimer = nullptr;
};

class MecAppBase : public cSimpleModule, public inet::TcpSocket::ICallback
{
  protected:
    /* TCP sockets are dynamically created by the user according to her needs
     * the HttpBaseMessage* will be linked to the userData variable in TCPSocket class
     * The base implementation already provides one socket to the ServiceRegistry and one socket to a MEC service
     * key is the name of the socket, just to std::string,. E.g. LocServiceSocket, RNISSocket
     * NOTE: remember to delete the HttpBaseMessage* pointer!
     */
    inet::SocketMap sockets_;

    cQueue packetQueue_;
    cMessage *currentProcessedMsg_ = nullptr;
    cMessage *processMessage_ = nullptr;

    // endpoint for contacting the Service Registry
    inet::L3Address mp1Address;
    int mp1Port;

    inet::L3Address serviceAddress;
    int servicePort;

    // FIXME not used, yet. These structures are supposed to be used
    cQueue serviceHttpMessages_;
    cQueue mp1HttpMessages_;

    inet::ModuleRefByPar<VirtualisationInfrastructureManager> vim;

    cMessage *sendTimer = nullptr;

    //references to modules
    inet::ModuleRefByPar<cModule> mecPlatform;
    inet::ModuleRefByPar<ServiceRegistry> serviceRegistry;

    int mecAppId;
    int mecAppIndex_;
    double requiredRam;
    double requiredDisk;
    double requiredCpu;

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;
    void finish() override;

    /* Method to be implemented for real MEC apps */
    virtual void handleProcessedMessage(cMessage *msg);
    virtual void handleSelfMessage(cMessage *msg) = 0;
    virtual void handleServiceMessage(int connId) = 0;
    virtual void handleMp1Message(int connId) = 0;
    virtual void handleHttpMessage(int connId) = 0;
    virtual void handleUeMessage(cMessage *msg) = 0;
    virtual void established(int connId) = 0;

    virtual double scheduleNextMsg(cMessage *msg);

    virtual inet::TcpSocket *addNewSocket();

    virtual void connect(inet::TcpSocket *socket, const inet::L3Address& address, const int port);

    virtual void removeSocket(inet::TcpSocket *tcpSock);

    /* inet::TcpSocket::CallbackInterface callback methods */
    void socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool urgent) override;
    void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) override { socket->accept(availableInfo->getNewSocketId()); }
    void socketEstablished(inet::TcpSocket *socket) override;
    void socketPeerClosed(inet::TcpSocket *socket) override;
    void socketClosed(inet::TcpSocket *socket) override;
    void socketFailure(inet::TcpSocket *socket, int code) override;
    void socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) override {}
    void socketDeleted(inet::TcpSocket *socket) override {}

  public:
    ~MecAppBase() override;

};

} //namespace

#endif /* APPS_MEC_MEAPPS_MECAPPBASE_H_ */

