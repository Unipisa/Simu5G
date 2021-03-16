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
#include "nodes/mec/MEPlatform/MeServices/packets/HttpRequestMessage/HttpRequestMessageSerializer.h"
#include "nodes/mec/MEPlatform/MeServices/packets/HttpRequestMessage/HttpRequestMessage.h"


using namespace omnetpp;
Register_Class(SocketManager);
Define_Module(SocketManager);

// TODO manage HTTP messages splitted in more TCP segments

void SocketManager::dataArrived(inet::Packet *msg, bool urgent){
        EV << "SocketManager::dataArrived" << endl;
        msg->removeControlInfo();

        std::vector<uint8_t> bytes =  msg->peekDataAsBytes()->getBytes();
        EV << "SocketManager::dataArrived - payload length: " << bytes.size() << endl;
        std::string packet(bytes.begin(), bytes.end());
        EV << "SocketManager::dataArrived - payload : " << packet << endl;

        //check fake packet

       delete msg;
      std::string delimiter = "\r\n\r\n";
      size_t pos = 0;
      std::string header;


      if(currentHttpMessage != nullptr && currentHttpMessage->isReceivingMsg())
      {
          EV << "MeAppBase::parseReceivedMsg - Continue receiving data for the current HttpMessage" << endl;
          Http::HttpMsgState res = Http::parseTcpData(&packet, currentHttpMessage);
          switch (res)
          {
          case (Http::COMPLETE_NO_DATA):
              EV << "MeAppBase::parseReceivedMsg - passing HttpMessage to application: " << res << endl;
              currentHttpMessage->setSockId(sock->getSocketId());
              service->emitRequestQueueLength();
              service->newRequest(currentHttpMessage);
              if(currentHttpMessage != nullptr)
              {
                  currentHttpMessage = nullptr;
              }
              return;
              break;
          case (Http::COMPLETE_DATA):
              EV << "MeAppBase::parseReceivedMsg - passing HttpMessage to application: " << res << endl;
              currentHttpMessage->setSockId(sock->getSocketId());
              service->emitRequestQueueLength();
              service->newRequest(currentHttpMessage);
              if(currentHttpMessage != nullptr)
              {
                  currentHttpMessage = nullptr;
              }
              break;
          case (Http::INCOMPLETE_DATA):
                  break;
          case (Http::INCOMPLETE_NO_DATA):
                  return;

          }
      }

      /*
       * If I get here OR:
       *  - I am not receiving an http message
       *  - I was receiving an http message but I still have data (i.e a new HttpMessage) to manage.
       *    Start reading the header
       */

      std::string temp;
      if(bufferedData.length() > 0)
      {
          EV << "MeAppBase::parseReceivedMsg - buffered data" << endl;
          temp = packet;
          packet = bufferedData + temp;

      }
      // inserici la roba vecchia e ricordati dove sei arrivato
      while ((pos = packet.find(delimiter)) != std::string::npos) {
          header = packet.substr(0, pos);
          packet.erase(0, pos+delimiter.length()); //remove header
          currentHttpMessage = Http::parseHeader(header);
          Http::HttpMsgState res = Http::parseTcpData(&packet, currentHttpMessage);
          switch (res)
          {
          case (Http::COMPLETE_NO_DATA):
              EV << "MeAppBase::parseReceivedMsg - passing HttpMessage to application: " << res << endl;
              currentHttpMessage->setSockId(sock->getSocketId());
              service->emitRequestQueueLength();
              service->newRequest(currentHttpMessage);
              if(currentHttpMessage != nullptr)
              {
                  currentHttpMessage = nullptr;
              }
              return;
              break;
          case (Http::COMPLETE_DATA):
                currentHttpMessage->setSockId(sock->getSocketId());
                service->emitRequestQueueLength();
                service->newRequest(currentHttpMessage);
              if(currentHttpMessage != nullptr)
              {
                  currentHttpMessage = nullptr;
              }
              break;
          case (Http::INCOMPLETE_DATA):
                  break;
          case (Http::INCOMPLETE_NO_DATA):
                  return;

          }
      }
      // posso arrivare qua se non trovo il delimitatore
      // a causa del segmento frammentato strano, devo salvare il contenuto
      if(packet.length() != 0)
      {
          bufferedData = packet;
      }

//
//        else if(packet.find("BulkRequest") != std::string::npos)
//        {
//            EV << "Bulk Request ";
//            std::string size = lte::utils::splitString(packet, ": ")[1];
//            int requests = std::stoi(size);
//            if(requests < 0)
//              throw cRuntimeError("Number of request must be non negative");
//            EV << " of size "<< requests << endl;
//            if(requests == 0)
//            {
//                delete msg;
//                return;
//            }
//            for(int i = 0 ; i < requests ; ++i)
//            {
//              if(i == requests -1)
//              {
//                  msg->setName("lastFakeRequest");
//                  service->newRequest(msg); //use it to send back response message (only for the last message)
//              }
//              else
//              {
//                  // simple cMessage, it does not need a socket
//                  cMessage *request = new cMessage("fakeRequest");
//                  service->newRequest(request);
//              }
//            }
//            return;
//        }
//        else
//        {
//            EV << "packet: " << packet << endl;
//            delete msg;
//        }

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
