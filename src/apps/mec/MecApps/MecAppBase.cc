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

#include <inet/common/ProtocolTag_m.h>
#include <inet/common/ProtocolGroup.h>
#include <inet/common/Protocol.h>

#include "apps/mec/MecApps/packets/ProcessingTimeMessage_m.h"
#include "nodes/mec/utils/MecCommon.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"

namespace simu5g {

using namespace omnetpp;
using namespace inet;


MecAppBase::~MecAppBase()
{
    std::cout << "MecAppBase::~MecAppBase()" << std::endl;
    cancelAndDelete(sendTimer);

    sockets_.deleteSockets();

    cancelAndDelete(processMessage_);
}

void MecAppBase::initialize(int stage)
{

    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;

    const char *mp1Ip = par("mp1Address");
    mp1Address = L3AddressResolver().resolve(mp1Ip);
    mp1Port = par("mp1Port");

    mecAppId = par("mecAppId"); // FIXME mecAppId is the deviceAppId (it does not change anything, though)
    mecAppIndex_ = par("mecAppIndex");
    requiredRam = par("requiredRam").doubleValue();
    requiredDisk = par("requiredDisk").doubleValue();
    requiredCpu = par("requiredCpu").doubleValue();

    vim.reference(this, "vimModule", true);
    mecPlatform.reference(this, "mecPlatformModule", true);

    serviceRegistry.reference(this, "serviceRegistryModule", true);

    processMessage_ = new cMessage("processedMessage");
}

void MecAppBase::connect(inet::TcpSocket *socket, const inet::L3Address& address, const int port)
{
    // we need a new connId if this is not the first connection
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
    if (msg->isSelfMessage()) {
        if (strcmp(msg->getName(), "processedMessage") == 0) {
            handleProcessedMessage(check_and_cast<cMessage *>(packetQueue_.pop()));
            if (!packetQueue_.isEmpty()) {
                double processingTime = scheduleNextMsg(check_and_cast<cMessage *>(packetQueue_.front()));
                EV << "MecAppBase::scheduleNextMsg() - next msg is processed in " << processingTime << "s" << endl;
                scheduleAt(simTime() + processingTime, processMessage_);
            }
            else {
                EV << "MecAppBase::handleMessage - no more messages are present in the queue" << endl;
            }
        }
        else if (strcmp(msg->getName(), "processedHttpMsg") == 0) {
            EV << "MecAppBase::handleMessage(): processedHttpMsg " << endl;
            ProcessingTimeMessage *procMsg = check_and_cast<ProcessingTimeMessage *>(msg);
            int connId = procMsg->getSocketId();
            TcpSocket *sock = static_cast<TcpSocket *>(sockets_.getSocketById(connId));
            if (sock != nullptr) {
                HttpMessageStatus *msgStatus = static_cast<HttpMessageStatus *>(sock->getUserData());
                handleHttpMessage(connId);
                delete msgStatus->httpMessageQueue.pop();
                if (!msgStatus->httpMessageQueue.isEmpty()) {
                    EV << "MecAppBase::handleMessage(): processedHttpMsg - the httpMessageQueue is not empty, schedule next HTTP message" << endl;
                    double time = vim->calculateProcessingTime(mecAppId, 150);
                    scheduleAt(simTime() + time, msg);
                }
            }
        }
        else {
            handleSelfMessage(msg);
        }
    }
    else {
        if (!processMessage_->isScheduled() && packetQueue_.isEmpty()) {
            packetQueue_.insert(msg);
            double processingTime;
            if (strcmp(msg->getFullName(), "data") == 0)
                processingTime = MecAppBase::scheduleNextMsg(msg);
            else
                processingTime = scheduleNextMsg(msg);
            EV << "MecAppBase::scheduleNextMsg() - next msg is processed in " << processingTime << "s" << endl;
            scheduleAt(simTime() + processingTime, processMessage_);
        }
        else if (processMessage_->isScheduled() && !packetQueue_.isEmpty()) {
            packetQueue_.insert(msg);
        }
        else {
            throw cRuntimeError("MecAppBase::handleMessage - This situation is not possible");
        }
    }
}

double MecAppBase::scheduleNextMsg(cMessage *msg)
{
    double processingTime = vim->calculateProcessingTime(mecAppId, 20);
    return processingTime;
}

void MecAppBase::handleProcessedMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        handleSelfMessage(msg);
    }
    else {
        ISocket *sock = sockets_.findSocketFor(msg);
        if (sock != nullptr) {
            EV << "MecAppBase::handleProcessedMessage(): message for socket with ID: " << sock->getSocketId() << endl;
            sock->processMessage(msg);
        }
        else {
            delete msg;
        }
    }
}

void MecAppBase::socketEstablished(TcpSocket *socket)
{
    established(socket->getSocketId());
}

void MecAppBase::socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool)
{
    EV << "MecAppBase::socketDataArrived" << endl;

    std::vector<uint8_t> bytes = msg->peekDataAsBytes()->getBytes();
    std::string packet(bytes.begin(), bytes.end());
    HttpMessageStatus *msgStatus = static_cast<HttpMessageStatus *>(socket->getUserData());

    bool res = Http::parseReceivedMsg(socket->getSocketId(), packet, msgStatus->httpMessageQueue, msgStatus->bufferedData, msgStatus->currentMessage);
    if (res) {
        if (vim == nullptr)
            throw cRuntimeError("MecAppBase::socketDataArrived - vim is null!");
        double time = vim->calculateProcessingTime(mecAppId, 150);
        if (!msgStatus->processMsgTimer->isScheduled())
            scheduleAt(simTime() + time, msgStatus->processMsgTimer);
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

TcpSocket *MecAppBase::addNewSocket()
{
    TcpSocket *newSocket = new TcpSocket();
    newSocket->setOutputGate(gate("socketOut"));
    newSocket->setCallback(this);
    HttpMessageStatus *msg = new HttpMessageStatus;
    msg->processMsgTimer = new ProcessingTimeMessage("processedHttpMsg");
    msg->processMsgTimer->setSocketId(newSocket->getSocketId());
    newSocket->setUserData(msg);

    sockets_.addSocket(newSocket);
    EV << "MecAppBase::addNewSocket(): added socket with ID: " << newSocket->getSocketId() << endl;
    return newSocket;
}

void MecAppBase::removeSocket(inet::TcpSocket *tcpSock)
{
    HttpMessageStatus *msgStatus = static_cast<HttpMessageStatus *>(tcpSock->getUserData());
    std::cout << "Deleting httpMessages in socket with sockId " << tcpSock->getSocketId() << std::endl;
    while (!msgStatus->httpMessageQueue.isEmpty()) {
        std::cout << "Deleting httpMessages message" << std::endl;
        delete msgStatus->httpMessageQueue.pop();
    }
    if (msgStatus->currentMessage != nullptr)
        delete msgStatus->currentMessage;
    if (msgStatus->processMsgTimer != nullptr) {
        cancelAndDelete(msgStatus->processMsgTimer);
    }
    delete sockets_.removeSocket(tcpSock);
}

void MecAppBase::finish()
{
    EV << "MecAppBase::finish()" << endl;
}

} //namespace

