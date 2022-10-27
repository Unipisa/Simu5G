/*
 * FLComputationEngine.cc
 *
 *  Created on: Oct 12, 2022
 *      Author: alessandro
 */

#include "apps/mec/FLaaS/FLController/FLControllerApp.h"

#include "apps/mec/FLaaS/packets/FLaasMsgs_m.h"
#include "FLComputationEngineApp.h"

#include "inet/common/packet/ChunkQueue.h"

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
    inARound_ = false;
}

FLComputationEngineApp::~FLComputationEngineApp()
{
    cancelAndDelete(stopRoundMsg_);
    cancelAndDelete(startRoundMsg_);
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
    EV << "dd" << trainingMode_ << endl;
    modelDimension_ = par("modelDimension");
    minLearners_ = par("minLearners");
    repeatTime_ = par("repeatTime").doubleValue();

    // schedule first round
    startRoundMsg_ = new cMessage("startRoundMsg");
    stopRoundMsg_ = new cMessage("stopRoundMsg");

    scheduleAfter(repeatTime_, startRoundMsg_);

    roundLifeCycleSignal_ = registerSignal("flaas_roundLifeCycle");
    flaas_startRoundSignal_ = registerSignal("flaas_startRoundSignal");
    flaas_sentGlobalModelSignal_ = registerSignal("flaas_sentGlobalModelSignal");
    flaas_recvLocalModelSignal_ = registerSignal("flaas_recvLocalModelSignal");}


void FLComputationEngineApp::handleSelfMessage(cMessage *msg)
{
    if(msg->isName("startRoundMsg"))
    {
        // ask the controller for a list
        AvailableLearnersMap *avLearners = flControllerApp_->getLearnersEndpoint(minLearners_);
        if(avLearners->size() < minLearners_)
        {
            EV << "FLComputationEngineApp::handleSelfMessage - startRoundMsg - No enough learners available to star a new round. Requested: " <<minLearners_ << " selected: " <<avLearners->size() << ". Trying again in " << repeatTime_ << " seconds" << endl;
            scheduleAfter(repeatTime_, startRoundMsg_);
            return;
        }
        else
        {
            // send start round a tutti i learners. but check if they are already connected to the engine
            inARound_ = true;
            auto it = avLearners->begin();
            for(; it != avLearners->end() ; ++it)
            {

                int id = it->first;
                if(learnerId2socketId_.find(id) != learnerId2socketId_.end())
                {
                    // send message if connected
                    sendStartRoundMessage(id);
                    // add the leraner to the current learner. this is also used to check if the received ACK comes from a choosen learners
                    LearnerStatus newLear = {id, 0., false};
                    currentLearners_[id] = newLear;
                }
                else
                {
                    EV << "FLComputationEngineApp::handleSelfMessage - startRoundMsg - Learner with id [" << id << "] not yet connected. Start msg will be sent after the connection" << endl;
                    auto newSocket = addNewSocket();
                    pendingSocketId2Learners_[newSocket->getSocketId()] = {id, roundId_};
                    Endpoint ep = it->second.endpoint;
                    connect(newSocket, ep.addr, ep.port);
                }
            }


            EV << "FLComputationEngineApp::handleSelfMessage - startRoundMsg - Stop round " << roundId_ << " in " << roundDuration_ << " seconds" << endl;
            scheduleAfter(roundDuration_, stopRoundMsg_);

            emit(roundLifeCycleSignal_, roundId_);

        }
        delete avLearners;
    }
    else if(msg->isName("stopRoundMsg"))
    {
        //calculate aggregation time base on the number of model received
        int count = 0;
        for(auto& learnerStatus : currentLearners_)
        {
            if(learnerStatus.second.modelSize > 0)
                count++;
        }

        if (count < minLearners_)
        {
            EV << "FLComputationEngineApp::handleSelfMessage - stopRoundMsg - Too few learners sent their local model, aggregation will not be done" << endl;
            scheduleAfter(repeatTime_, startRoundMsg_);
        }

        else
        {
            double aggregationTime = modelAggregationDuration_ * count;
            aggregationMsg_ = new cMessage("aggregationMsg");
            scheduleAfter(aggregationTime, aggregationMsg_);
            EV << "FLComputationEngineApp::handleSelfMessage - stopRoundMsg - Start aggregating the global model. Finishing in " << aggregationTime << " seconds" << endl;
        }
        emit(roundLifeCycleSignal_, roundId_);
        currentLearners_.clear();
        roundId_++;
        inARound_ = false;
    }
    else if(msg->isName("aggregationMsg"))
    {
        delete msg;
        // TODO do something
        // start a new round
        EV << "FLComputationEngineApp::handleSelfMessage - aggregationMsg - Starting new round with id " << roundId_ <<" in " << repeatTime_ << "s" << endl;
        if(trainingMode_ == ONE_SHOT)
        {
            EV << "FLComputationEngineApp::handleSelfMessage - aggregationMsg - The training mode is ONE_SHOT, no other rounds will be scheduled" << endl;
        }
        else
        {
            scheduleAfter(repeatTime_, startRoundMsg_);
        }
        MLModel newModel = {simTime(),modelDimension_};
        flControllerApp_->updateGlobalModel(newModel);
    }
}

void FLComputationEngineApp::established(int connId)
{
    if(pendingSocketId2Learners_.find(connId) != pendingSocketId2Learners_.end())
    {
        int round = pendingSocketId2Learners_[connId].roundId;
        int learnerId = pendingSocketId2Learners_[connId].id;
        if(round == roundId_ && inARound_)
        {
            learnerId2socketId_[learnerId] = connId;
            sendStartRoundMessage(learnerId);
            // add the leraner to the current learner. this is also used to check if the received ACK comes from a choosen learners
            LearnerStatus newLear = {learnerId, 0., false};
            currentLearners_[learnerId] = newLear;
            //send start
        }

    }
}

void FLComputationEngineApp::sendStartRoundMessage(int learnerId)
{
    inet::Packet* packet = new inet::Packet("startRoundPacket");
    auto start = inet::makeShared<FLaasStartRoundPacket>();
    start->setType(START_ROUND);
    start->setRoundId(roundId_);
    start->setLearnerId(learnerId);
    start->setRoundDuration(roundDuration_); // TODO put the remaining duration
    start->setChunkLength(inet::B(12));
    start->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(start);
    int socketId = learnerId2socketId_.at(learnerId);
    auto socket = sockets_.getSocketById(socketId);
    socket->send(packet);
    emit(flaas_startRoundSignal_, learnerId);

    EV << "FLComputationEngineApp::sendStartRoundMessage: sent start round message to learner with id " << learnerId<< endl;
}

void FLComputationEngineApp::sendModelToTrain(int learnerId)
{
    inet::Packet* packet = new inet::Packet("modelToTrainPacket");
    auto model = inet::makeShared<FLaaSGlobalModelPacket>();
    model->setType(TRAIN_GLOBAL_MODEL);
    model->setRoundId(roundId_);
    model->setLearnerId(learnerId);
    model->setTimeRemaining(roundDuration_); // TODO put the remaining duration
    model->setModelSize(modelDimension_);
    model->setChunkLength(inet::B(modelDimension_));
    model->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(model);
    int socketId = learnerId2socketId_.at(learnerId);
    auto socket = sockets_.getSocketById(socketId);
    socket->send(packet);
    EV << "FLComputationEngineApp::sendModelToTrain: model sent to learner with id " << learnerId<< endl;
    emit(flaas_sentGlobalModelSignal_, learnerId);
}


inet::TcpSocket* FLComputationEngineApp::addNewSocket()
{
    inet::TcpSocket* newSocket = new inet::TcpSocket();
    newSocket->setOutputGate(gate("socketOut"));
    newSocket->setCallback(this);
    newSocket->setUserData(new inet::ChunkQueue());
    sockets_.addSocket(newSocket);

    EV << "FLComputationEngineApp::addNewSocket(): added socket with ID: " << newSocket->getSocketId() << endl;
    return newSocket;
}


//void FLComputationEngineApp::handleMessage(cMessage *msg)
//{
//
//    if (learnerSocket_.belongsToSocket(msg))
//    {
//        // TODO implement callback to create a new socket! ID should not be necesary
//        learnerSocket_.processMessage(msg);
//    }
//    else {
//        MecAppBase::handleMessage(msg);
//    }
//}

//sapendo che questa app non avrà contatti con i MEC service posso rifarla da 0!
void FLComputationEngineApp::socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool)
{
    auto queue = (inet::ChunkQueue*)socket->getUserData();
    auto chunk = msg->peekDataAt(inet::B(0), msg->getTotalLength());
    queue->push(chunk);
    EV <<"Message queue size: " << queue->getLength() << endl;


    while (queue->has<FLaasPacket>(inet::b(-1))) {
        auto pkt = queue->peek<FLaasPacket>(inet::b(-1));
        if(pkt->getType() == START_ROUND_ACK)
        {
            auto startRoundPkt = queue->pop<FLaasStartRoundPacket>(inet::b(-1));
            int roundId = startRoundPkt->getRoundId();
            int learnerId = startRoundPkt->getLearnerId();
            if(inARound_ && roundId == roundId_ && startRoundPkt->getResponse())
            {
                if(currentLearners_.find(learnerId) != currentLearners_.end())
                {
                    currentLearners_[learnerId].responded = true;
                    sendModelToTrain(learnerId);
                }
                else
                {
                    EV << "FLComputationEngineApp::socketDataArrived - START_ROUND_ACK - Learner with id "<< learnerId << " not present in the currentLearners_ structure" << endl;
                }
            }
            else
            {
                EV << "FLComputationEngineApp::socketDataArrived - START_ROUND_ACK - Learner with id "<< learnerId << " will be not involved in the training for this round" << endl;
                EV << "FLComputationEngineApp::socketDataArrived - START_ROUND_ACK - inARound var ["<< inARound_ << "], roundId_ ["<< roundId_<< "], roundMsg ["<< roundId<< "], response [" << startRoundPkt->getResponse() << "]"<< endl;
                if(inARound_ && roundId == roundId_ && startRoundPkt->getResponse() == false)
                {
                    currentLearners_.erase(learnerId);
                    // peek a new learner from the the available ones and send start message to it
//                    std::set<int> currentLearners;
    //                for(auto const& l: currentLearners_)
    //                {
    //                    currentLearners.insert(l.first);
    //                }
    //                flControllerApp_->getLearnersEndpointExcept(currentLearners);
                }
            }
        }
        else if(pkt->getType() == TRAINED_MODEL)
        {
            auto trainedModel = queue->pop<FLaaSTrainedModelPacket>(inet::b(-1));
            int roundId = trainedModel->getRoundId();
            int learnerId = trainedModel->getLearnerId();
            if(inARound_ && roundId == roundId_)
            {
                if(currentLearners_.find(learnerId) != currentLearners_.end())
                {
                    EV << "FLComputationEngineApp::socketDataArrived - TRAINED_MODEL - Learner with id "<< learnerId << " returned its localModel of " << trainedModel->getModelSize() << "B" << endl;
                    currentLearners_[learnerId].modelSize = trainedModel->getModelSize();
                    currentLearners_[learnerId].responded = trainedModel->getModelSize();
                    emit(flaas_recvLocalModelSignal_, learnerId);
                }
                else
                {
                    EV << "FLComputationEngineApp::socketDataArrived - TRAINED_MODEL - Learner with id "<< learnerId << " not present in the currentLearners_ structure" << endl;
                }
                //check all learners response and stop the round earlier
                if(allModelsRetrieved())
                {
                    // stop stopRoundMessage and schedule now
                    EV << "FLComputationEngineApp::socketDataArrived - TRAINED_MODEL - All local models received from the learners!" << endl;
                    if(stopRoundMsg_->isScheduled())
                    {
                        cancelEvent(stopRoundMsg_);
                        scheduleAfter(0, stopRoundMsg_);
                    }
                }
            }
            else
            {
                EV << "FLComputationEngineApp::socketDataArrived - TRAINED_MODEL - Learner with id "<< learnerId << " sent Model after round end or relative to a different round.. discarding" << endl;
                EV << "FLComputationEngineApp::socketDataArrived - TRAINED_MODEL - inARound var ["<< inARound_ << "], roundId_ ["<< roundId_<< "], roundMsg ["<< roundId<< "]" << endl;
            }
        }

        }
    delete msg;
}

bool FLComputationEngineApp::allModelsRetrieved()
{
    for(const auto& learn : currentLearners_)
    {
        if(learn.second.modelSize == 0.){
            return false;
        }
    }
    return true;
}

double FLComputationEngineApp::scheduleNextMsg(cMessage* msg) {return 0.;}

