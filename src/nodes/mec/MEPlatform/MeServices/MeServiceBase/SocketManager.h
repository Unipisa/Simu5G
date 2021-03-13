/*
 * socketManager.c
 *
 *  Created on: Dec 4, 2020
 *      Author: linofex
 */

#ifndef __SOCKETMANAGER_H
#define __SOCKETMANAGER_H

#include "nodes/mec/MEPlatform/MeServices/MeServiceBase/MeServiceBase.h"
#include "inet/common/INETDefs.h"
#include "inet/common/lifecycle/ILifecycle.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"

/*
 * For each new connection, the MeServiceBase creates a new SocketManager
 * object to manage the state TCP connection by implementing the
 * inet::TcpSocket::CallbackInterface interface.
 * It reassembles (if necessary) HTTP packets sent by the MEC application
 * and it puts them in the request queue of the server.
 */


class SocketManager : public omnetpp::cSimpleModule, public inet::TcpSocket::ICallback
{
  protected:
    MeServiceBase *service;
    inet::TcpSocket *sock;    // ptr into socketMap managed by TCPSrvHostApp

    // internal: inet::TcpSocket::CallbackInterface methods
    virtual void socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent) override { dataArrived(packet, urgent); }
    virtual void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) override { socket->accept(availableInfo->getNewSocketId()); }
    virtual void socketEstablished(inet::TcpSocket *socket) override { established(); }
    virtual void socketPeerClosed(inet::TcpSocket *socket) override { peerClosed(); }
    virtual void socketClosed(inet::TcpSocket *socket) override { closed(); }
    virtual void socketFailure(inet::TcpSocket *socket, int code) override { failure(code); }
    virtual void socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) override { statusArrived(status); }
    virtual void socketDeleted(inet::TcpSocket *socket) override { if (socket == sock) sock = nullptr; }

//    virtual void socketStatusArrived(int, void *, inet::TCPStatusInfo *status) override { statusArrived(status); }

  public:

    SocketManager() { sock = nullptr; service = nullptr; }
    virtual ~SocketManager() {delete sock;}

    // internal: called by TCPSrvHostApp after creating this module
    virtual void init(MeServiceBase *serv, inet::TcpSocket *socket) { service = serv; sock = socket; }

    /*
     * Returns the socket object
     */
    virtual inet::TcpSocket *getSocket() { return sock; }

    /*
     * Returns pointer to the host module
     */
    virtual MeServiceBase *getHostModule() { return service; }

    /**
     * Schedule an event. Do not use getContextPointer() of omnetpp::cMessage, because
     * TCPServerThreadBase uses it for its own purposes.
     */
//    virtual void scheduleAt(simtime_t t, omnetpp::cMessage *msg) { msg->setContextPointer(this); hostmod->scheduleAt(t, msg); }

//    /*
//     *  Cancel an event
//     */
//    virtual omnetpp::cMessage cancelEvent(omnetpp::cMessage *msg) { return service->cancelEvent(msg); }

    /**
     * Called when connection is established. To be redefined.
     */
    virtual void established();

    /*
     * Called when a data packet arrives. To be redefined.
     */
    virtual void dataArrived(inet::Packet *msg, bool urgent);

    /*
     * Called when a timer (scheduled via scheduleAt()) expires. To be redefined.
     */
//    virtual void timerExpired(omnetpp::cMessage *timer) = 0;

    /*
     * Called when the client closes the connection. By default it closes
     * our side too, but it can be redefined to do something different.
     */
    virtual void peerClosed();

    /*
     * Called when the connection closes (successful TCP teardown). By default
     * it deletes this thread, but it can be redefined to do something different.
     */
    virtual void closed();

    /*
     * Called when the connection breaks (TCP error). By default it deletes
     * this thread, but it can be redefined to do something different.
     */
    virtual void failure(int code);

    /*
     * Called when a status arrives in response to getSocket()->getStatus().
     * By default it deletes the status object, redefine it to add code
     * to examine the status.
     */
    virtual void statusArrived(inet::TcpStatusInfo *status) { delete status; }
};

#endif
