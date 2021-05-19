/*
 * MecAppBase.cc
 *
 *  Created on: Dec 6, 2020
 *      Author: linofex
 */


#include "apps/mec/MeApps/MecAppBase.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "nodes/mec/MEPlatform/MeServices/httpUtils/httpUtils.h"
#include "nodes/mec/MecCommon.h"
#include "nodes/mec/VirtualisationInfrastructureManager/VirtualisationInfrastructureManager.h"
#include "nodes/mec/MEPlatform/ServiceRegistry/ServiceRegistry.h"
#include "common/utils/utils.h"

using namespace omnetpp;
using namespace inet;


MecAppBase::MecAppBase()
{
    sendTimer = nullptr;
    serviceHttpMessage = nullptr;
    mp1HttpMessage = nullptr;
    vim = nullptr;
}

MecAppBase::~MecAppBase()
{
    if(sendTimer != nullptr)
    {
        if(sendTimer->isSelfMessage())
            cancelAndDelete(sendTimer);
        else
            delete sendTimer;
    }

    if(serviceHttpMessage != nullptr)
    {
        delete serviceHttpMessage;
    }

    if(mp1HttpMessage != nullptr)
    {
        delete mp1HttpMessage;
    }


   cancelAndDelete(processedServiceResponse);
   cancelAndDelete(processedMp1Response);

}

void MecAppBase::initialize(int stage)
{
//    TcpAppBase::initialize(stage);

    /*
     * TODO
     */


    if(stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;

    const char *mp1Ip = par("mp1Address");
    mp1Address = L3AddressResolver().resolve(mp1Ip);
    mp1Port = par("mp1Port");


    serviceSocket_.setOutputGate(gate("socketOut"));
    mp1Socket_.setOutputGate(gate("socketOut"));

    serviceSocket_.setCallback(this);
    mp1Socket_.setCallback(this);


//    // parameters
//    const char *localAddress = par("localAddress");
//    int localPort = par("localPort");
//    socket.bind(*localAddress ? L3AddressResolver().resolve(localAddress) : L3Address(), localPort);
//

    mecAppId = par("mecAppId"); // FIXME use ueAppId!
    requiredRam = par("requiredRam").doubleValue();
    requiredDisk = par("requiredDisk").doubleValue();
    requiredCpu = par("requiredCpu").doubleValue();

    vim = check_and_cast<VirtualisationInfrastructureManager*>(getParentModule()->getSubmodule("vim"));
    meHost = getParentModule();
    mePlatform = meHost->getSubmodule("mecPlatform");

    serviceRegistry = check_and_cast<ServiceRegistry *>(mePlatform->getSubmodule("serviceRegistry"));

    processedServiceResponse = new cMessage("processedServiceResponse");
    processedMp1Response = new cMessage("processedMp1Response");

}

void MecAppBase::connect(inet::TcpSocket* socket, const inet::L3Address& address, const int port)
{
    // we need a new connId if this is not the first connection
    socket->renewSocket();

    int timeToLive = par("timeToLive");
    if (timeToLive != -1)
        socket->setTimeToLive(timeToLive);

    int dscp = par("dscp");
    if (dscp != -1)
        socket->setDscp(dscp);

    int tos = par("tos");
    if (tos != -1)
        socket->setTos(tos);

    if (address.isUnspecified()) {
        EV_ERROR << "Connecting to " << address << " port=" << port << ": cannot resolve destination address\n";
    }
    else {
        EV_INFO << "Connecting to " << address << " port=" << port << endl;

        socket->connect(address, port);
    }
}

void MecAppBase::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        EV << "MecAppBase::handleMessage" << endl;
        if(strcmp(msg->getName(), "processedServiceResponse") == 0)
        {
            handleServiceMessage();
            if(serviceHttpMessage != nullptr)
            {
                delete serviceHttpMessage;
                serviceHttpMessage = nullptr;
            }
        }
        else if (strcmp(msg->getName(), "processedMp1Response") == 0)
        {
            handleMp1Message();
            if(mp1HttpMessage != nullptr)
            {
                delete mp1HttpMessage;
                mp1HttpMessage = nullptr;
            }
        }
        else
        {
            handleSelfMessage(msg);
        }
    }
    else
    {
        // from service or from ue app?
        if(serviceSocket_.belongsToSocket(msg))
        {
            serviceSocket_.processMessage(msg);
        }
        else if(mp1Socket_.belongsToSocket(msg))
        {
            mp1Socket_.processMessage(msg);
        }

    }
}

void MecAppBase::socketEstablished(TcpSocket * socket)
{
    established(socket->getSocketId());
}

void MecAppBase::socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool)
{
    EV << "MecAppBase::socketDataArrived" << endl;


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
//    EV << "MecAppBase::socketDataArrived - payload: " << endl;
//
    std::vector<uint8_t> bytes =  msg->peekDataAsBytes()->getBytes();
    std::string packet(bytes.begin(), bytes.end());
//    EV << packet << endl;

    if(serviceSocket_.belongsToSocket(msg))
    {
        bool res =  Http::parseReceivedMsg(packet, &bufferedData, &serviceHttpMessage);
        if(res)
        {
            serviceHttpMessage->setSockId(serviceSocket_.getSocketId());
            double time = vim->calculateProcessingTime(mecAppId, 150);
            scheduleAt(simTime()+time, processedServiceResponse);
        }
    }
    else if (mp1Socket_.belongsToSocket(msg))
    {
        bool res =  Http::parseReceivedMsg(packet, &bufferedData, &mp1HttpMessage);
        if(res)
        {
            mp1HttpMessage->setSockId(mp1Socket_.getSocketId());
            double time = vim->calculateProcessingTime(mecAppId, 150);
            scheduleAt(simTime()+time, processedMp1Response);
        }

    }
    else
    {
        throw cRuntimeError("MecAppBase::socketDataArrived - Socket %s not recognized", socket->getSocketId());
    }
    delete msg;

}

void MecAppBase::parseReceivedMsg(inet::TcpSocket *socket, HttpBaseMessage* currentHttpMessage, std::string& packet)
{
    EV_INFO << "MecAppBase::parseReceivedMsg" << endl;
    std::string delimiter = "\r\n\r\n";
    size_t pos = 0;
    std::string header;
    int remainingData;

    if(currentHttpMessage != nullptr && currentHttpMessage->isReceivingMsg())
    {
        EV << "MecAppBase::parseReceivedMsg - Continue receiving data for the current HttpMessage" << endl;
        Http::HttpMsgState res = Http::parseTcpData(&packet, currentHttpMessage);
        double time;
        switch (res)
        {
        case (Http::COMPLETE_NO_DATA):
            EV << "MecAppBase::parseReceivedMsg - passing HttpMessage to application: " << res << endl;
            if(vim != nullptr)
                time = vim->calculateProcessingTime(mecAppId, 100);
            else
                time = 0;
            if(socket->getSocketId() == serviceSocket_.getSocketId())
            {
                currentHttpMessage->setSockId(serviceSocket_.getSocketId());
                serviceHttpMessage = currentHttpMessage;
                scheduleAt(simTime()+time, processedServiceResponse);
            }
            else if (socket->getSocketId() == mp1Socket_.getSocketId())
            {
                currentHttpMessage->setSockId(mp1Socket_.getSocketId());
                mp1HttpMessage = currentHttpMessage;
                scheduleAt(simTime()+time, processedMp1Response);

            }
            else
            {
                EV << "!UI" << endl;
            }

//            if(currentHttpMessage != nullptr)
//            {
//                delete currentHttpMessage;
//                currentHttpMessage = nullptr;
//            }
            return;
            break;
        case (Http::COMPLETE_DATA):
            /*
             * TODO if there is an other msg, it is possible that the schedule at will execute the new one..
             * for now thrown exeption since in our test cases, this should not happen
             */
                throw cRuntimeError("MecAppBase::parseReceivedMsg - case COMPLETE_DATA. TODO");
//            EV << "MecAppBase::parseReceivedMsg - passing HttpMessage to application: " << res << endl;
//            currentHttpMessage->setSockId(socket.getSocketId());
//            if(vim != nullptr)
//                time = vim->calculateProcessingTime(mecAppId, 100);
//            else
//                time = 0;                scheduleAt(simTime()+time, processedServiceResponse);
//            if(currentHttpMessage != nullptr)
//            {
//                delete currentHttpMessage;
//                currentHttpMessage = nullptr;
//            }
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
        EV << "MecAppBase::parseReceivedMsg - buffered data" << endl;
        temp = packet;
        packet = bufferedData + temp;

    }

    while ((pos = packet.find(delimiter)) != std::string::npos) {
        header = packet.substr(0, pos);
        packet.erase(0, pos+delimiter.length()); //remove header
        currentHttpMessage = Http::parseHeader(header);

        Http::HttpMsgState res = Http::parseTcpData(&packet, currentHttpMessage);
        double time;
        switch (res)
        {
        case (Http::COMPLETE_NO_DATA):
            EV << "MecAppBase::parseReceivedMsg - passing HttpMessage to application: " << res << endl;

            if(vim != nullptr)
            {
                time = vim->calculateProcessingTime(mecAppId, 100);
            }
            else
            {
                time = 0;
            }

            if(socket->getSocketId() == serviceSocket_.getSocketId())
            {
                currentHttpMessage->setSockId(serviceSocket_.getSocketId());
                serviceHttpMessage = currentHttpMessage;
                scheduleAt(simTime()+time, processedServiceResponse);
            }
            else if (socket->getSocketId() == mp1Socket_.getSocketId())
            {
                currentHttpMessage->setSockId(mp1Socket_.getSocketId());
                mp1HttpMessage = currentHttpMessage;
                scheduleAt(simTime()+time, processedMp1Response);

            }
            else
                        {
                            EV << "!UI" << endl;
                        }
//            if(currentHttpMessage != nullptr)
//               {
//                   delete currentHttpMessage;
//                   currentHttpMessage = nullptr;
//               }
            return;
            break;
        case (Http::COMPLETE_DATA):
            /*
            * TODO if there is an other msg, it is possible that the schedule at will execute the new one..
            * for now thrown exeption since in our test cases, this should not happen
            */
                throw cRuntimeError("MecAppBase::parseReceivedMsg - case COMPLETE_DATA. TODO");
//            EV << "MecAppBase::parseReceivedMsg - passing HttpMessage to application: " << res << endl;
//            currentHttpMessage->setSockId(socket.getSocketId());
//            if(vim != nullptr)
//                            time = vim->calculateProcessingTime(mecAppId, 100);
//                        else
//                            time = 0;                scheduleAt(simTime()+time, processedServiceResponse);
//            if(currentHttpMessage != nullptr)
//            {
//                delete currentHttpMessage;
//                currentHttpMessage = nullptr;
//            }
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

void MecAppBase::socketPeerClosed(TcpSocket *socket_)
{
    EV << "MecAppBase::socketPeerClosed" << endl;
    socket_->close();
}

void MecAppBase::socketClosed(TcpSocket *socket)
{
    EV_INFO << "MecAppBase::socketClosed" << endl;
}

void MecAppBase::socketFailure(TcpSocket *sock, int code)
{
    // subclasses may override this function, and add code try to reconnect after a delay.
}

void MecAppBase::finish()
{
}



