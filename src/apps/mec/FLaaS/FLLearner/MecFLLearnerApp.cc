/*
 * FLComputationEngine.cc
 *
 *  Created on: Oct 12, 2022
 *      Author: alessandro
 */

#include "apps/mec/FLaaS/FLController/FLControllerApp.h"

#include "apps/mec/FLaaS/packets/FLaasMsgs_m.h"
#include "inet/common/packet/ChunkQueue.h"
#include "MecFLLearnerApp.h"

Define_Module(MecFLLearnerApp);

MecFLLearnerApp::MecFLLearnerApp()
{
    flComputationEnginesocket_ = nullptr;
    endRoundMsg_ = nullptr;
    trainingDurationMsg_ = nullptr;
    roundId_ = 0;
}

MecFLLearnerApp::~MecFLLearnerApp()
{
    cancelAndDelete(trainingDurationMsg_);
    cancelAndDelete(endRoundMsg_);
}

void MecFLLearnerApp::initialize(int stage)
{
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    std::string parentModule = par("parentModule").stdstringValue();

    // do not initialize mec hosr stuf if learner is in UE!
    // only processMessage_ is important
    if(parentModule.compare("MEC_HOST") == 0)
        MecAppBase::initialize(stage);
    else
        processMessage_ = new cMessage("processedMessage");

    //retrieving parameters
    size_ = par("packetSize");
    localUePort = par("localUePort");

    localModelSize_ = par("localModelSize");

    //testing
    EV << "MecFLLearnerApp::initialize - Mec application "<< getClassName() << " with mecAppId["<< mecAppId << "] has started!" << endl;

    trainingDuration = par("trainingDuration");


    trainingDurationMsg_ = new cMessage("trainingDurationMsg");
    endRoundMsg_ = new cMessage("endRoundMsg");


    listeningSocket_.setOutputGate(gate("socketOut"));
    listeningSocket_.setCallback(this);
    listeningSocket_.bind(localUePort);
    listeningSocket_.listen();

    flaas_startRoundSignal_ = registerSignal("flaas_startRoundSignal");
    flaas_recvGlobalModelSignal_ = registerSignal("flaas_recvGlobalModelSignal");
    flaas_sendLocalModelSignal_ = registerSignal("flaas_sendLocalModelSignal");
}


void MecFLLearnerApp::handleSelfMessage(cMessage *msg)
{
    if(msg->isName("trainingDurationMsg"))
    {
        EV << "MecFLLearnerApp::handleSelfMessage - trainingDurationMsg for round [" << roundId_ << "] ended. Sending back the new Model" << endl;
        sendTrainedLocalModel();
        if(endRoundMsg_->isScheduled())
        {
            cancelEvent(endRoundMsg_);
        }

    }
    else if(msg->isName("endRoundMsg"))
    {
        EV << "MecFLLearnerApp::handleSelfMessage - round [" << roundId_ << "] ended. Stop training" << endl;
        cancelEvent(trainingDurationMsg_);
    }
}


inet::TcpSocket* MecFLLearnerApp::addNewSocket()
{
    inet::TcpSocket* newSocket = new inet::TcpSocket();
    newSocket->setOutputGate(gate("socketOut"));
    newSocket->setCallback(this);
    newSocket->setUserData(new inet::ChunkQueue());
    sockets_.addSocket(newSocket);

    EV << "MecFLLearnerApp::addNewSocket(): added socket with ID: " << newSocket->getSocketId() << endl;
    return newSocket;
}


void MecFLLearnerApp::handleMessage(cMessage *msg)
{
    if(!msg->isSelfMessage())
    {
        if(listeningSocket_.belongsToSocket(msg))
        {
            // TODO implement callback to create a new socket! ID should not be necessary
            listeningSocket_.processMessage(msg);
            return;
        }
    }
//    else
//    {
        MecAppBase::handleMessage(msg);
//    }
}


void MecFLLearnerApp::socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo)
{
    EV << "MecFLLearnerApp::socketAvailable" << endl;
    // new TCP connection -- create new socket object and server process
    inet::TcpSocket *newSocket = new inet::TcpSocket(availableInfo);
    newSocket->setOutputGate(gate("socketOut"));
    newSocket->setCallback(this);
    newSocket->setUserData(new inet::ChunkQueue());
    sockets_.addSocket(newSocket);
    flComputationEnginesocket_ = newSocket;

    socket->accept(availableInfo->getNewSocketId());
}

//sapendo che questa app non avrà contatti con i MEC service posso rifarla da 0!
void MecFLLearnerApp::socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool)
{
    EV << "MecFLLearnerApp::socketDataArrived" << endl;
    auto queue = (inet::ChunkQueue*)socket->getUserData();
    auto chunk = msg->peekDataAt(inet::B(0), msg->getTotalLength());
    queue->push(chunk);
    EV <<"Message queue size: " << queue->getLength() << endl;

    while (queue->has<FLaasPacket>(inet::b(-1))) {
        auto pkt = queue->peek<FLaasPacket>(inet::b(-1));
        if(pkt->getType() == START_ROUND)
        {
            auto startRoundPkt = queue->pop<FLaasStartRoundPacket>(inet::b(-1));
            roundId_ = startRoundPkt->getRoundId();
            EV << "MecFLLearnerApp::socketDataArrived - START_ROUND round id " << roundId_ << endl;

            int learnerId = startRoundPkt->getLearnerId();
            inet::Packet* resp = new inet::Packet("FLaasStartRoundPacket");
            auto startRoundResPkt = inet::makeShared<FLaasStartRoundPacket>();
//            startRoundResPkt = startRoundPkt;
            startRoundResPkt->setType(START_ROUND_ACK);
            startRoundResPkt->setRoundId(roundId_);
            startRoundResPkt->setLearnerId(learnerId);
            startRoundResPkt->setResponse(true);
            startRoundResPkt->setChunkLength(startRoundPkt->getChunkLength());
            startRoundResPkt->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());


            resp->insertAtFront(startRoundResPkt);
            flComputationEnginesocket_->send(resp);

            // schedule timeout if I am slow
            double endRound = startRoundPkt->getRoundDuration();
            scheduleAfter(endRound, endRoundMsg_);

            //stop current training
            if(trainingDurationMsg_->isScheduled())
            {
                cancelEvent(trainingDurationMsg_);
            }
            emit(flaas_startRoundSignal_, learnerId);


        }
        else if(pkt->getType() == TRAIN_GLOBAL_MODEL)
        {
            auto trainedModel = queue->pop<FLaaSGlobalModelPacket>(inet::b(-1));
            recvGlobalModelToTrain();
            int roundId = trainedModel->getRoundId();
            if(trainingDurationMsg_->isScheduled() || roundId_ != roundId)
            {
                // todo what to do?
                throw cRuntimeError("MecFLLearnerApp::socketDataArrived - TRAIN_GLOBAL_MODEL - Round [%d] is currently training. Arrived message with roundId [%d]", roundId_, roundId);

            }
            // scheudle training
            scheduleAfter(trainingDuration, trainingDurationMsg_);
            emit(flaas_recvGlobalModelSignal_, trainedModel->getLearnerId());
            EV << "MecFLLearnerApp::socketDataArrived - TRAIN_GLOBAL_MODEL round id " << roundId << endl;

        }
    }
    delete msg;
}

void MecFLLearnerApp::sendTrainedLocalModel()
{
    inet::Packet* pkt  = new inet::Packet("FLaaSTrainedModelPacket");
    auto model =inet::makeShared<FLaaSTrainedModelPacket>();
    model->setType(TRAINED_MODEL);
    model->setRoundId(roundId_);
    model->setModelSize(localModelSize_);
    model->setLearnerId(getId());
    model->setChunkLength(inet::B(localModelSize_));
    model->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    pkt->insertAtBack(model);
    flComputationEnginesocket_->send(pkt);

    emit(flaas_sendLocalModelSignal_, model->getLearnerId());
}
bool MecFLLearnerApp::recvGlobalModelToTrain()
{
    return true;
}

double MecFLLearnerApp::scheduleNextMsg(cMessage* msg)
{
//    if(vim != nullptr)
//        MecAppBase::scheduleNextMsg(msg);
//    else
        return 0.;
}




