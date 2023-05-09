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


using namespace omnetpp;
Register_Class(SocketManager);
Define_Module(SocketManager);


void SocketManager::dataArrived(inet::Packet *msg, bool urgent){
    EV << "SocketManager::dataArrived" << endl;
    msg->removeControlInfo();

    std::vector<uint8_t> bytes =  msg->peekDataAsBytes()->getBytes();
    EV << "SocketManager::dataArrived - payload length: " << bytes.size() << endl;
    std::string packet(bytes.begin(), bytes.end());
    EV << "SocketManager::dataArrived - payload : " << packet << endl;

    /**
     * This block of code inserts fictitious requests to load the server without the need to
     * instantiate and manage MEC apps. A  fictitious request contains the number of requests
     * to insert in the request queue:
     *    BulkRequest: #reqNum
     */
    if(packet.rfind("BulkRequest", 0) == 0)
    {
        EV << "Bulk Request ";
        std::string size = lte::utils::splitString(packet, ": ")[1];
        int requests = std::stoi(size);
        if(requests < 0)
          throw cRuntimeError("Number of request must be non negative");
        EV << " of size "<< requests << endl;
        if(requests == 0)
        {
            return;
        }
        for(int i = 0 ; i < requests ; ++i)
        {
          if(i == requests -1)
          {
              HttpRequestMessage *req = new HttpRequestMessage();
              req->setLastBackGroundRequest(true);
              req->setSockId(sock->getSocketId());
              req->setState(CORRECT);
              service->newRequest(req); //use it to send back response message (only for the last message)
          }
          else
          {
              HttpRequestMessage *req = new HttpRequestMessage();
              req->setBackGroundRequest(true);
              req->setSockId(sock->getSocketId());
              req->setState(CORRECT);
              service->newRequest(req);
          }
        }
        delete msg;
        return;
    }
    // ########################

    bool res = Http::parseReceivedMsg(sock->getSocketId(), packet, httpMessageQueue, &bufferedData, &currentHttpMessage);

    if(res)
    {
//        currentHttpMessage->setSockId(sock->getSocketId());
        while(!httpMessageQueue.isEmpty())
        {
            // TODO handle response in service!!
            service->emitRequestQueueLength();
            HttpBaseMessage* msg =  (HttpBaseMessage*)httpMessageQueue.pop();
            msg->setArrivalTime(simTime());
            if(msg->getType() == REQUEST)
                service->newRequest(check_and_cast<HttpRequestMessage*>(msg));
            else
                delete msg;
        }
    }
    delete msg;
    return;
}

void SocketManager::established(){
    EV_INFO << "New connection established " << endl;
}

void SocketManager::peerClosed()
{
    std::cout<<"SocketManager::peerClosed - Closed connection from: " << sock->getRemoteAddress()<< std::endl;
    EV << "SocketManager::peerClosed() - Closed connection from: " << sock->getRemoteAddress()<< std::endl;
//    sock->setState(inet::TcpSocket::PEER_CLOSED);
    service->removeSubscritions(sock->getSocketId()); // if any

    // close the connection (if not already closed)
    if (sock->getState() == inet::TcpSocket::PEER_CLOSED) {
        EV << "SocketManager::socketPeerClose - remote TCP closed, closing here as well. Connection id is " << sock->getSocketId() << endl;
        sock->close(); // Call the close method to properly dispose of the socket.
    }
}

void SocketManager::closed()
{
    std::cout <<"SocketManager::closed - Removed socket of: " << sock->getRemoteAddress() << " from map" << std::endl;
    sock->setState(inet::TcpSocket::CLOSED);
    service->removeSubscritions(sock->getSocketId());
    service->closedConnection(this);
}

void SocketManager::failure(int code)
{
    EV <<"Socket of: " << sock->getRemoteAddress() << " failed. Code: " << code << std::endl;
    service->removeSubscritions(sock->getSocketId());
    service->removeConnection(this);
}
