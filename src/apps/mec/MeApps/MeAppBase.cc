/*
 * MeAppBase.cc
 *
 *  Created on: Dec 6, 2020
 *      Author: linofex
 */


#include "apps/mec/MeApps/MeAppBase.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "nodes/mec/MEPlatform/MeServices/httpUtils/httpUtils.h"
#include "common/utils/utils.h"

using namespace omnetpp;
using namespace inet;

//simsignal_t MeAppBase::connectSignal = registerSignal("connect");
//simsignal_t MeAppBase::rcvdPkSignal = registerSignal("rcvdPk");
//simsignal_t MeAppBase::sentPkSignal = registerSignal("sentPk");

MeAppBase::MeAppBase()
{
    sendTimer = nullptr;
    currentHttpMessage = nullptr;
    }

MeAppBase::~MeAppBase()
{
   if(sendTimer != nullptr)
   {
    if(sendTimer->isSelfMessage())
        cancelAndDelete(sendTimer);
    else
        delete sendTimer;
   }

   if(currentHttpMessage != nullptr)
   {
       delete currentHttpMessage;
       currentHttpMessage = nullptr;
   }

}

void MeAppBase::initialize(int stage)
{
    TcpAppBase::initialize(stage);
}

void MeAppBase::handleStartOperation(LifecycleOperation *operation)
{

}

void MeAppBase::handleStopOperation(LifecycleOperation *operation)
{
//    cancelEvent(timeoutMsg);
    if (socket.getState() == TcpSocket::CONNECTED || socket.getState() == TcpSocket::CONNECTING || socket.getState() == TcpSocket::PEER_CLOSED)
        close();
}

void MeAppBase::handleCrashOperation(LifecycleOperation *operation)
{
//    cancelEvent(timeoutMsg);
    if (operation->getRootModule() != getContainingNode(this))
        socket.destroy();
}

void MeAppBase::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage())
            handleSelfMessage(msg);
        else
            socket.processMessage(msg);
}

void MeAppBase::socketEstablished(TcpSocket * socket)
{
    EV_INFO << "connected\n";
    established(socket->getSocketId());
}

void MeAppBase::socketDataArrived(inet::TcpSocket *, inet::Packet *msg, bool)
{
    EV << "MeAppBase::socketDataArrived" << endl;

//  In progress. Trying to handle an app message coming from different tcp segments in a better way
//    bool res = msg->hasAtFront<HttpResponseMessage>();
//    auto pkc = msg->peekAtFront<HttpResponseMessage>(b(-1), Chunk::PF_ALLOW_EMPTY | Chunk::PF_ALLOW_SERIALIZATION | Chunk::PF_ALLOW_INCOMPLETE);
//    if(pkc!=nullptr && pkc->isComplete() &&  !pkc->isEmpty()){
//
//    }
//    else
//    {
//        EV << "Null"<< endl;
//    }


//
//    EV << "MeAppBase::socketDataArrived - payload: " << endl;
//
    std::vector<uint8_t> bytes =  msg->peekDataAsBytes()->getBytes();
    std::string packet(bytes.begin(), bytes.end());
//    EV << packet << endl;
    delete msg;
    parseReceivedMsg(packet);
}

void MeAppBase::parseReceivedMsg(std::string& packet)
{
    EV_INFO << "MeAppBase::parseReceivedMsg" << endl;
    std::string delimiter = "\r\n\r\n";
    size_t pos = 0;
    std::string header;
    int remainingData;

    if(currentHttpMessage != nullptr && currentHttpMessage->isReceivingMsg())
    {
        EV << "MeAppBase::parseReceivedMsg - Continue receiving data for the current HttpMessage" << endl;
        Http::HttpMsgState res = Http::parseTcpData(&packet, currentHttpMessage);
        switch (res)
        {
        case (Http::COMPLETE_NO_DATA):
            EV << "MeAppBase::parseReceivedMsg - passing HttpMessage to application: " << res << endl;
            currentHttpMessage->setSockId(socket.getSocketId());
            handleTcpMsg();
            if(currentHttpMessage != nullptr)
            {
                delete currentHttpMessage;
                currentHttpMessage = nullptr;
            }
            return;
            break;
        case (Http::COMPLETE_DATA):
            EV << "MeAppBase::parseReceivedMsg - passing HttpMessage to application: " << res << endl;
            currentHttpMessage->setSockId(socket.getSocketId());
            handleTcpMsg();
            if(currentHttpMessage != nullptr)
            {
                delete currentHttpMessage;
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

    while ((pos = packet.find(delimiter)) != std::string::npos) {
        header = packet.substr(0, pos);
        packet.erase(0, pos+delimiter.length()); //remove header
        currentHttpMessage = Http::parseHeader(header);
        Http::HttpMsgState res = Http::parseTcpData(&packet, currentHttpMessage);
        switch (res)
        {
        case (Http::COMPLETE_NO_DATA):
            EV << "MeAppBase::parseReceivedMsg - passing HttpMessage to application: " << res << endl;
            currentHttpMessage->setSockId(socket.getSocketId());
            handleTcpMsg();
            if(currentHttpMessage != nullptr)
            {
                delete currentHttpMessage;
                currentHttpMessage = nullptr;
            }
            return;
            break;
        case (Http::COMPLETE_DATA):
            EV << "MeAppBase::parseReceivedMsg - passing HttpMessage to application: " << res << endl;
            currentHttpMessage->setSockId(socket.getSocketId());
            handleTcpMsg();
            if(currentHttpMessage != nullptr)
            {
                delete currentHttpMessage;
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

}

void MeAppBase::socketPeerClosed(TcpSocket *socket_)
{
    EV << "MeAppBase::socketPeerClosed" << endl;
    TcpAppBase::socketPeerClosed(socket_);
}

void MeAppBase::socketClosed(TcpSocket *socket)
{
    TcpAppBase::socketClosed(socket);
    EV_INFO << "MeAppBase::socketClosed" << endl;
}

void MeAppBase::socketFailure(TcpSocket *sock, int code)
{
    // subclasses may override this function, and add code try to reconnect after a delay.
    TcpAppBase::socketFailure(sock,code);
}

void MeAppBase::finish()
{
    TcpAppBase::finish();
}



