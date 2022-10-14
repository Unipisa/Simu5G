/*
 * FLComputationEngine.cc
 *
 *  Created on: Oct 12, 2022
 *      Author: alessandro
 */

#include "apps/mec/FLaaS/FLComputationEngine/FLComputationEngine.h"
#include "apps/mec/FLaaS/FLController/FLControllerApp.h"

#include "apps/mec/FLaaS/packets/FLaasMsgs_m.h"

Define_Module(FLComputationEngineApp);

FLComputationEngineApp::FLComputationEngineApp()
{
    serviceSocket_ = nullptr;
    mp1Socket_ = nullptr;
    mp1HttpMessage = nullptr;
    serviceHttpMessage = nullptr;
    flControllerApp_ = nullptr;
    roundId_ = 0;
    startRoundMsg_ = nullptr;
    stopRoundMsg_ = nullptr;
    aggregationMsg_ = nullptr;
}


void FLComputationEngineApp::initialize(int stage)
{
    MecAppBase::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    //retrieving parameters
    size_ = par("packetSize");

    localUePort = par("localUePort");

    //testing
    EV << "FLComputationEngineApp::initialize - Mec application "<< getClassName() << " with mecAppId["<< mecAppId << "] has started!" << endl;

    roundDuration_ = par("roundDuration");
    modelAggregationDuration_ = par("modelAggregationDuration");
    localModelTreshold_ = par("localModelTreshold");
    int tm =  par("trainingMode");
    trainingMode_ = FLTrainingMode(tm);
    modelDimension_ = inet::B(par("modelDimension"));
    minLearners_ = par("minLearners");
    repeatTime_ = par("repeatTime").doubleValue();

    // schedule first round
    startRoundMsg_ = new cMessage("startRoundMsg");
}


void FLComputationEngineApp::handleSelfMessage(cMessage *msg)
{
    if(msg->isName("startRoundMsg"))
    {
        // ask the controller for a list
        AvailableLearnersMap *avLearners = flControllerApp_->getLearnersEndpoint(minLearners_);
        bool starRound = false;
        if(avLearners->size() < minLearners_)
        {
            EV << "FLComputationEngineApp::handleSelfMessage - startRoundMsg - No enough learners available to star a new round. Trying again in " << repeatTime_ << " seconds" << endl;
            scheduleAfter(repeatTime_, startRoundMsg_);
            return;
        }
        else
        {
            // send start round a tutti i learners. but check if they are already connected to the engine
            auto it = avLearners->begin();
            int id = it->first;
            if(learnerId2socketId_.find(id) != learnerId2socketId_.end())
            {
                // send message if connected
                inet::Packet* packet = new inet::Packet("startRoundPacket");
                auto start = inet::makeShared<FLaasStartRoundPacket>();
                start->setRoundId(roundId_);
                start->setRoundDuration(roundDuration_);
                start->setChunkLength(inet::B(12));
                packet->insertAtBack(start);
                int socketId = learnerId2socketId_[id];
                auto socket = sockets_.getSocketById(socketId);
                socket->send(packet);
            }
            else
            {
                EV << "FLComputationEngineApp::handleSelfMessage - startRoundMsg - Learner not yet connected. Start msg will be sent after the connection" << endl;
                auto newSocket = addNewSocket();
                pendingSocketId2Learners_[newSocket->getSocketId()] = {id, roundId_};
                Endpoint ep = it->second.endpoint;
                connect(newSocket, ep.addr, ep.port);
            }
        }



    }
    else if(msg->isName("stopRoundMsg"))
    {

    }
}

void FLComputationEngineApp::established(int connId)
{
    if(pendingSocketId2Learners_.find(connId) != pendingSocketId2Learners_.end())
    {
        int round = pendingSocketId2Learners_[connId].roundId;
        if(round == roundId_)
        {
            //send start
        }

    }
}
inet::TcpSocket* FLComputationEngineApp::addNewSocket()
{
    inet::TcpSocket* newSocket = new inet::TcpSocket();
    newSocket->setOutputGate(gate("socketOut"));
    newSocket->setCallback(this);
    sockets_.addSocket(newSocket);
    EV << "FLComputationEngineApp::addNewSocket(): added socket with ID: " << newSocket->getSocketId() << endl;
    return newSocket;
}


void FLComputationEngineApp::handleMessage(cMessage *msg)
{
    if (learnerSocket_.belongsToSocket(msg))
    {
        // TODO implement callback to create a new socket! ID should not be necesary
        learnerSocket_.processMessage(msg);
    }
    else {
        MecAppBase::handleMessage(msg);
    }
}


void FLComputationEngineApp::socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool)
{
    auto pkt = msg->peekAtFront<FLaasPacket>();
    if(pkt->getType() == START_ROUND_ACK)
    {
        auto startRoundPkt = msg->removeAtFront<FLaasStartRoundPacket>();
    }
    else if(pkt->getType() == TRAINED_MODEL)
    {
        auto trainedModel = msg->removeAtFront<FLaaSTrainedModelPacket>();
    }

    //sapendo che questa app non avrà contatti con il Location service posso rifarla da 0!
}

