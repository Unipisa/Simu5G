/*
 * socketManager.c
 *
 *  Created on: Dec 4, 2020
 *      Author: linofex
 */

#include "nodes/mec/MEPlatform/MeServices/MeServiceBase/SocketManager.h"

#include "common/utils/utils.h"
#include "../MeServiceBase/MeServiceBase.h"
#include <iostream>
#include "nodes/mec/MEPlatform/MeServices/httpUtils/httpUtils.h"

using namespace omnetpp;
Register_Class(SocketManager);
Define_Module(SocketManager);

// TODO manage HTTP messages splitted in more TCP segments
void SocketManager::dataArrived(inet::Packet *msg, bool urgent){
        EV << "SocketManager::dataArrived" << endl;
//        std::string packet = lte::utils::getPacketPayload(msg);
        std::vector<uint8_t> bytes =  msg->peekDataAsBytes()->getBytes();
        std::string packet(bytes.begin(), bytes.end());
        EV << packet << endl;
        Http::DataType type = service->getDataType(packet);
        if (type == Http::REQUEST)
        {
            service->emitRequestQueueLength();
            service->newRequest(msg);
        }
        else if(type == Http::RESPONSE)
        {
            EV << "Response Arrived" << endl;
            // manage depending the method used
            delete msg;
        }
        else if(packet.find("BulkRequest") != std::string::npos)
        {
            EV << "Bulk Request ";
            std::string size = lte::utils::splitString(packet, ": ")[1];
            int requests = std::stoi(size);
            if(requests < 0)
              throw cRuntimeError("Number of request must be non negative");
            EV << " of size "<< requests << endl;
            if(requests == 0)
            {
                delete msg;
                return;
            }
            for(int i = 0 ; i < requests ; ++i)
            {
              if(i == requests -1)
              {
                  msg->setName("lastFakeRequest");
                  service->newRequest(msg); //use it to send back response message (only for the last message)
              }
              else
              {
                  // simple cMessage, it does not need a socket
                  cMessage *request = new cMessage("fakeRequest");
                  service->newRequest(request);
              }
            }
            return;
        }
        else
        {
            EV << "packet: " << packet << endl;
            delete msg;
        }

}

void SocketManager::established(){
    EV_INFO << "New connection established " << endl;
}

void SocketManager::peerClosed()
{
    std::cout<<"Closed connection from: " << sock->getRemoteAddress()<< std::endl;
//    service_->removeSubscription(sock);
    sock->close();
}

void SocketManager::closed()
{
    std::cout <<"Removed socket of: " << sock->getRemoteAddress() << " from map" << std::endl;
    service->removeSubscritions(sock->getSocketId());
    service->closeConnection(this);
}

void SocketManager::failure(int code)
{
    std::cout <<"Socket of: " << sock->getRemoteAddress() << " failed. Code: " << code << std::endl;
    service->removeSubscritions(sock->getSocketId());
    service->removeConnection(this);
}
