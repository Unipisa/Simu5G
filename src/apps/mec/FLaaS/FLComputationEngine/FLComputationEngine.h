/*
 * FLComputationEngineApp.h
 *
 *  Created on: Oct 12, 2022
 *      Author: alessandro
 */

#ifndef APPS_MEC_FLAAS_FLCOMPUTATIONENGINE_FLCOMPUTATIONENGINE_H_
#define APPS_MEC_FLAAS_FLCOMPUTATIONENGINE_FLCOMPUTATIONENGINE_H_


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
    inet::B modelDimension_;
    FLControllerApp* flControllerApp_;
    int roundId_;
    int minLearners_;

    inet::TcpSocket* serviceSocket_;
    inet::TcpSocket* mp1Socket_;

    HttpBaseMessage* mp1HttpMessage;
    HttpBaseMessage* serviceHttpMessage;

    int size_;
    std::string subId;

    // circle danger zone
    cOvalFigure * circle;
    double centerPositionX;
    double centerPositionY;
    double radius;

    protected:
        virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;
        virtual void finish() override;

        virtual void handleProcessedMessage(omnetpp::cMessage *msg) override;

        virtual void handleHttpMessage(int connId) override;
        virtual void handleServiceMessage(int connId) override;
        virtual void handleMp1Message(int connId) override;
        virtual void handleUeMessage(omnetpp::cMessage *msg) override;
        virtual void handleMessage(omnetpp::cMessage *msg) override;
        virtual void handleSelfMessage(cMessage *msg) override;

        virtual void socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool) override;


        virtual void modifySubscription();
        virtual void sendSubscription();
        virtual void sendDeleteSubscription();


        virtual inet::TcpSocket* addNewSocket() override;





//        /* TCPSocket::CallbackInterface callback methods */
       virtual void established(int connId) override;

    public:
       FLComputationEngineApp();
       virtual ~FLComputationEngineApp();

       void setFLControllerPointer(FLControllerApp* controller) {flControllerApp_ = controller;}

};




#endif /* APPS_MEC_FLAAS_FLCOMPUTATIONENGINE_FLCOMPUTATIONENGINE_H_ */
