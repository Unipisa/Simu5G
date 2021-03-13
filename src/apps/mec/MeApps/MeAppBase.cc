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
    responseMessageLength = 0;
    receivedMessage.clear();
    sendTimer = nullptr;
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
    EV << "MeAppBase::socketDataArrived - payload: " << endl;

    std::vector<uint8_t> bytes =  msg->peekDataAsBytes()->getBytes();
    std::string packet(bytes.begin(), bytes.end());
    //EV << packet << endl;
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

    if(receivingMessage == true)
    {
        EV << "MeAppBase::parseReceivedMsg - receiving HTTP message" << endl;
        int len = packet.length();
        receivedMessage.at("body") += packet.substr(0, responseMessageLength);
        packet.erase(0, responseMessageLength);
        responseMessageLength -= (len - packet.length());
        if(packet.length() == 0 && responseMessageLength == 0 )
        {
            EV << "MeAppBase::parseReceivedMsg - HTTP message received" << endl;
            // pacchetto letto tutto -> invia all'app
            handleTcpMsg();
            receivedMessage.clear();
            receivingMessage = false;
            return;
        }
        else if(packet.length() == 0 && responseMessageLength > 0 )
        {
            EV << "MeAppBase::parseReceivedMsg - new chunk of a HTTP message: " << responseMessageLength << " bytes remaining" << endl;
            // pacchetto non tutto arrivato -> aspetta
            return;
        }
        else if(packet.length() == 0)
        {
            throw cRuntimeError("MeAppBase::parseReceivedMsg - This should not happen. It is just a test");
            // errore
        }
    }

    // se arrivo qua, o receiving era false,
    //oppure ho ancora dati da leggere dal pacchetto arrivato
    //i dati da leggere partono con uno header
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
        Http::HTTPHeader headers = Http::parseHeader(header);
        if(headers.type == Http::UNKNOWN)
        {
            EV << "MeAppBase::parseReceivedMsg - Unknown data" << endl;
            return;
        }
        receivingMessage = true;
        if(headers.fields.find("Content-Length") != headers.fields.end())
            responseMessageLength = std::stoi(headers.fields.at("Content-Length")); // prendo content length
        else
            responseMessageLength = 0;
        EV << "MeAppBase::parseReceivedMsg - responseMessageLength: " << responseMessageLength << endl;

        int len = packet.length();
        receivedMessage["body"] = packet.substr(0, responseMessageLength);
        packet.erase(0, responseMessageLength);
        responseMessageLength -= (len - packet.length());
        if(packet.length() == 0 && responseMessageLength == 0 )
        {
           EV << "MeAppBase::parseReceivedMsg - received complete HTTP message" << endl;

           // pacchetto letto tutto -> invia all'app
           handleTcpMsg();
           receivedMessage.clear();
           receivingMessage = false;
           return;
        }
        else if(packet.length() == 0 && responseMessageLength > 0 )
        {
            EV << "MeAppBase::parseReceivedMsg - received first chunk of a HTTP message: " << responseMessageLength << " bytes remaining" << endl;

           // pacchetto non tutto arrivato -> aspetta
           return;
        }
        else if(packet.length() == 0)
        {
           throw cRuntimeError("This should not happen. It is just a test");
           // errore
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
    TcpAppBase::socketPeerClosed(socket_);
}

void MeAppBase::socketClosed(TcpSocket *)
{
    // *redefine* to start another session etc.
    EV_INFO << "connection closed\n";
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



