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

#include "apps/mec/MecApps/MecAppBase.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/utils/MecCommon.h"
#include "common/utils/utils.h"

using namespace omnetpp;
using namespace inet;


MecAppBase::MecAppBase()
{
    mecHost = nullptr;
    mecPlatform = nullptr;;
    serviceRegistry = nullptr;
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

    if(stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;

    const char *mp1Ip = par("mp1Address");
    mp1Address = L3AddressResolver().resolve(mp1Ip);
    mp1Port = par("mp1Port");

    serviceSocket_.setOutputGate(gate("socketOut"));
    mp1Socket_.setOutputGate(gate("socketOut"));

    serviceSocket_.setCallback(this);
    mp1Socket_.setCallback(this);

    mecAppId = par("mecAppId"); // FIXME mecAppId is the deviceAppId (it does not change anything, though)
    requiredRam = par("requiredRam").doubleValue();
    requiredDisk = par("requiredDisk").doubleValue();
    requiredCpu = par("requiredCpu").doubleValue();

    vim = check_and_cast<VirtualisationInfrastructureManager*>(getParentModule()->getSubmodule("vim"));
    mecHost = getParentModule();
    mecPlatform = mecHost->getSubmodule("mecPlatform");

    serviceRegistry = check_and_cast<ServiceRegistry *>(mecPlatform->getSubmodule("serviceRegistry"));

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
            if(vim == nullptr)
                throw cRuntimeError("MecAppBase::socketDataArrived - vim is null!");
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
            if(vim == nullptr)
                throw cRuntimeError("MecAppBase::socketDataArrived - vim is null!");
            double time = vim->calculateProcessingTime(mecAppId, 150);
            scheduleAt(simTime()+time, processedMp1Response);
        }

    }
    else
    {
        throw cRuntimeError("MecAppBase::socketDataArrived - Socket %d not recognized", socket->getSocketId());
    }
    delete msg;

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
    EV << "MecAppBase::finish()" << endl;
    if(serviceSocket_.getState() == inet::TcpSocket::CONNECTED)
        serviceSocket_.close();
    if(mp1Socket_.getState() == inet::TcpSocket::CONNECTED)
        mp1Socket_.close();
}



