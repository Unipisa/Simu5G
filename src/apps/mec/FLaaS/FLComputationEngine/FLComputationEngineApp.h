/*
 * FLComputationEngineApp.h
 *
 *  Created on: Oct 12, 2022
 *      Author: alessandro
 */

#ifndef APPS_MEC_FLAAS_FLCOMPUTATIONENGINE_FLCOMPUTATIONENGINEAPP_H_
#define APPS_MEC_FLAAS_FLCOMPUTATIONENGINE_FLCOMPUTATIONENGINEAPP_H_


#include "omnetpp.h"

#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h"

#include "apps/mec/MecApps/MecAppBase.h"

#include "apps/mec/FLaaS/FLaaSUtils.h"

class FLControllerApp;

typedef struct
{
    int id;
    int roundId;
} PendingLearner;

typedef struct
{
    int id;
    double modelSize;
    bool responded;
    simtime_t modelSent;
} LearnerStatus;



typedef std::map<int, LearnerStatus> CurrentLearnerMap;

class FLComputationEngineApp : public MecAppBase
{
    //UDP socket to communicate with the UeApp
//    inet::UdpSocket ueSocket;
    int localUePort;

    inet::TcpSocket learnerSocket_;

    inet::L3Address ueAppAddress;
    int ueAppPort;

    inet::SocketMap learners_;
    std::map<int, int> learnerId2socketId_; // learners already connected to the engine
    std::map<int, PendingLearner> pendingSocketId2Learners_; // learners already connected to the engine


    double roundDuration_;
    double repeatTime_;
    double modelAggregationDuration_;
    cMessage* startRoundMsg_;
    cMessage* stopRoundMsg_;
    cMessage* aggregationMsg_;

    int localModelTreshold_;
    FLTrainingMode trainingMode_;
    double modelDimension_;
    FLControllerApp* flControllerApp_;
    int roundId_;
    bool inARound_;
    int minLearners_;
    CurrentLearnerMap currentLearners_;

    inet::TcpSocket* serviceSocket_;
    inet::TcpSocket* mp1Socket_;

    HttpBaseMessage* mp1HttpMessage;
    HttpBaseMessage* serviceHttpMessage;

    simtime_t roundStart_;

    int size_;
    std::string subId;

    simsignal_t roundLifeCycleSignal_;

    simsignal_t flaas_startRoundSignal_;
    simsignal_t flaas_sentGlobalModelSignal_;
    simsignal_t flaas_recvLocalModelSignal_;

    simsignal_t flaas_ulTimeSignal;
    simsignal_t flaas_dlTimeSignal;
    simsignal_t flaas_trainingTimeSignal;
    simsignal_t flaas_trainingDurationSignal;
    simsignal_t flaas_trainingDurationConnectionSignal;



    protected:
        virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;
        virtual void finish() override {};

//        virtual void handleProcessedMessage(omnetpp::cMessage *msg) override;

        virtual void handleHttpMessage(int connId) override {};
        virtual void handleServiceMessage(int connId) override {};
        virtual void handleMp1Message(int connId) override {};
        virtual void handleUeMessage(omnetpp::cMessage *msg) override {};
//        virtual void handleMessage(omnetpp::cMessage *msg) override;
        virtual void handleSelfMessage(cMessage *msg) override;

        virtual void socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool) override;

//        virtual void modifySubscription();
//        virtual void sendSubscription();
//        virtual void sendDeleteSubscription();

        virtual double scheduleNextMsg(cMessage* msg) override;


        virtual inet::TcpSocket* addNewSocket() override;
        void sendStartRoundMessage(int learnerId);
        void sendModelToTrain(int learnerId);
        bool allModelsRetrieved();



//        /* TCPSocket::CallbackInterface callback methods */
       virtual void established(int connId) override;

    public:
       FLComputationEngineApp();
       virtual ~FLComputationEngineApp();

       void setFLControllerPointer(FLControllerApp* controller) {flControllerApp_ = controller;}

};




#endif /* APPS_MEC_FLAAS_FLCOMPUTATIONENGINE_FLCOMPUTATIONENGINEAPP_H_ */
