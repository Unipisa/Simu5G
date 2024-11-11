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

#ifndef __SOCKETMANAGER_H
#define __SOCKETMANAGER_H

#include <inet/common/INETDefs.h>
#include <inet/common/lifecycle/ILifecycle.h>
#include <inet/transportlayer/contract/tcp/TcpSocket.h>

#include "nodes/mec/MECPlatform/MECServices/MECServiceBase/MecServiceBase.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpMessages_m.h"

namespace simu5g {

using namespace omnetpp;

/*
 * For each new connection, the MecServiceBase creates a new SocketManager
 * object to manage the state of the TCP connection by implementing the
 * inet::TcpSocket::CallbackInterface interface.
 * It reassembles (if necessary) HTTP packets sent by the MEC application
 * and puts them in the request queue of the server.
 */

class SocketManager : public cSimpleModule, public inet::TcpSocket::ICallback
{
  protected:
    MecServiceBase *service = nullptr;
    inet::TcpSocket *sock = nullptr;    // pointer into socketMap managed by TCPSrvHostApp
    HttpBaseMessage *currentHttpMessage = nullptr;
    cQueue httpMessageQueue;
    std::string bufferedData;

    // internal: inet::TcpSocket::CallbackInterface methods
    void socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent) override { dataArrived(packet, urgent); }
    void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) override { socket->accept(availableInfo->getNewSocketId()); }
    void socketEstablished(inet::TcpSocket *socket) override { established(); }
    void socketPeerClosed(inet::TcpSocket *socket) override { peerClosed(); }
    void socketClosed(inet::TcpSocket *socket) override { closed(); }
    void socketFailure(inet::TcpSocket *socket, int code) override { failure(code); }
    void socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) override { statusArrived(status); }
    void socketDeleted(inet::TcpSocket *socket) override { if (socket == sock) sock = nullptr; }

  public:

    ~SocketManager() override { delete sock; }

    // internal: called by TCPSrvHostApp after creating this module
    virtual void init(MecServiceBase *serv, inet::TcpSocket *socket) { service = serv; sock = socket; }

    /*
     * Returns the socket object
     */
    virtual inet::TcpSocket *getSocket() { return sock; }

    /*
     * Returns pointer to the host module
     */
    virtual MecServiceBase *getHostModule() { return service; }

    /**
     * Called when the connection is established. To be redefined.
     */
    virtual void established();

    /*
     * Called when a data packet arrives. To be redefined.
     */
    virtual void dataArrived(inet::Packet *msg, bool urgent);

    /*
     * Called when the client closes the connection. By default, it closes
     * our side too, but it can be redefined to do something different.
     */
    virtual void peerClosed();

    /*
     * Called when the connection closes (successful TCP teardown). By default
     * it deletes this thread, but it can be redefined to do something different.
     */
    virtual void closed();

    /*
     * Called when the connection breaks (TCP error). By default, it deletes
     * this thread, but it can be redefined to do something different.
     */
    virtual void failure(int code);

    /*
     * Called when a status arrives in response to getSocket()->getStatus().
     * By default, it deletes the status object; redefine it to add code
     * to examine the status.
     */
    virtual void statusArrived(inet::TcpStatusInfo *status) { delete status; }
};

} //namespace

#endif

