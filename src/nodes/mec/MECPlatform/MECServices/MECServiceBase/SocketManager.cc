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

#include "nodes/mec/MECPlatform/MECServices/MECServiceBase/SocketManager.h"

#include "common/utils/utils.h"
#include "nodes/mec/MECPlatform/MECServices/MECServiceBase/MecServiceBase.h"
#include <iostream>
#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpRequestMessage/HttpRequestMessage.h"

namespace simu5g {

using namespace omnetpp;
Register_Class(SocketManager);
Define_Module(SocketManager);

void SocketManager::dataArrived(inet::Packet *msg, bool urgent) {
    EV << "SocketManager::dataArrived" << endl;
    msg->removeControlInfo();

    std::vector<uint8_t> bytes = msg->peekDataAsBytes()->getBytes();
    EV << "SocketManager::dataArrived - payload length: " << bytes.size() << endl;
    std::string packet(bytes.begin(), bytes.end());
    EV << "SocketManager::dataArrived - payload : " << packet << endl;

    /**
     * This block of code inserts fictitious requests to load the server without the need to
     * instantiate and manage MEC apps. A fictitious request contains the number of requests
     * to insert in the request queue:
     *    BulkRequest: #reqNum
     */
    if (packet.rfind("BulkRequest", 0) == 0) {
        EV << "Bulk Request ";
        std::string size = simu5g::utils::splitString(packet, ": ")[1];
        int requests = std::stoi(size);
        if (requests < 0)
            throw cRuntimeError("Number of requests must be non-negative");
        EV << " of size " << requests << endl;
        if (requests == 0) {
            return;
        }
        for (int i = 0; i < requests; ++i) {
            HttpRequestMessage *req = new HttpRequestMessage();
            req->setSockId(sock->getSocketId());
            req->setState(CORRECT);
            if (i == requests - 1)
                req->setLastBackGroundRequest(true);
            else
                req->setBackGroundRequest(true);
            service->newRequest(req);
        }
        delete msg;
        return;
    }
    // ########################

    bool res = Http::parseReceivedMsg(sock->getSocketId(), packet, httpMessageQueue, bufferedData, currentHttpMessage);

    if (res) {
        while (!httpMessageQueue.isEmpty()) {
            // TODO handle response in service!!
            service->emitRequestQueueLength();
            HttpBaseMessage *msg = check_and_cast<HttpBaseMessage *>(httpMessageQueue.pop());
            msg->setArrivalTime(simTime());
            if (msg->getType() == REQUEST)
                service->newRequest(check_and_cast<HttpRequestMessage *>(msg));
            else
                delete msg;
        }
    }
    delete msg;
}

void SocketManager::established() {
    EV_INFO << "New connection established " << endl;
}

void SocketManager::peerClosed()
{
    EV << "Closed connection from: " << sock->getRemoteAddress() << std::endl;
    sock->setState(inet::TcpSocket::PEER_CLOSED);
    service->removeSubscriptions(sock->getSocketId()); // if any
}

void SocketManager::closed()
{
    EV << "Removed socket of: " << sock->getRemoteAddress() << " from map" << std::endl;
    sock->setState(inet::TcpSocket::CLOSED);
    service->removeSubscriptions(sock->getSocketId());
    service->closeConnection(this);
}

void SocketManager::failure(int code)
{
    EV << "Socket of: " << sock->getRemoteAddress() << " failed. Code: " << code << std::endl;
    service->removeSubscriptions(sock->getSocketId());
    service->removeConnection(this);
}

} //namespace

