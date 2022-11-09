/*
 * MecFLLearnerApp.h
 *
 *  Created on: Oct 12, 2022
 *      Author: alessandro
 */

#ifndef APPS_MEC_FLAAS_FLCOMPUTATIONENGINE_MecFLLearnerApp_H_
#define APPS_MEC_FLAAS_FLCOMPUTATIONENGINE_MecFLLearnerApp_H_


#include "omnetpp.h"

#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h"

#include "apps/mec/MecApps/MecAppBase.h"

#include "apps/mec/FLaaS/FLaaSUtils.h"

class MecFLLearnerApp : public MecAppBase
{
    //UDP socket to communicate with the UeApp
//    inet::UdpSocket ueSocket;
    int localUePort;
    double localModelSize_;

    inet::TcpSocket listeningSocket_;
    inet::TcpSocket* flComputationEnginesocket_;

    int roundId_;


    double trainingDuration;
    double shortPeriodProbality;

    cMessage* trainingDurationMsg_;
    cMessage* endRoundMsg_;

    int size_;

    simtime_t modelArriving_;

    simsignal_t flaas_startRoundSignal_;
    simsignal_t flaas_recvGlobalModelSignal_;
    simsignal_t flaas_sendLocalModelSignal_;


    protected:
        virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;
        virtual void finish() override {};

//        virtual void handleProcessedMessage(omnetpp::cMessage *msg) override;

        virtual void handleHttpMessage(int connId) override {};
        virtual void handleServiceMessage(int connId) override {};
        virtual void handleMp1Message(int connId) override {};
        virtual void handleUeMessage(omnetpp::cMessage *msg) override {};
        virtual void handleMessage(omnetpp::cMessage *msg) override;
        virtual void handleSelfMessage(cMessage *msg) override;

        virtual double scheduleNextMsg(cMessage* msg) override;


//        virtual void modifySubscription();
//        virtual void sendSubscription();
//        virtual void sendDeleteSubscription();


        virtual inet::TcpSocket* addNewSocket() override;
        void sendTrainedLocalModel();
        bool recvGlobalModelToTrain();

//        /* TCPSocket::CallbackInterface callback methods */
        virtual void socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent) override;
        virtual void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) override;
        virtual void established(int connId) override {};

    public:
       MecFLLearnerApp();
       virtual ~MecFLLearnerApp();
};




#endif /* APPS_MEC_FLAAS_FLCOMPUTATIONENGINE_MecFLLearnerApp_H_ */
