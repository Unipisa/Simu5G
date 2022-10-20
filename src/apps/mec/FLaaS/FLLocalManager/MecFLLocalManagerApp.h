/*
 * MecFLLocalManager.h
 *
 *  Created on: Oct 18, 2022
 *      Author: alessandro
 */

#ifndef APPS_MEC_FLAAS_FLLOCALMANAGER_MECFLLOCALMANAGERAPP_H_
#define APPS_MEC_FLAAS_FLLOCALMANAGER_MECFLLOCALMANAGERAPP_H_

#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"


#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h"
#include "apps/mec/MecApps/MecAppBase.h"
#include "apps/mec/FLaaS/FLaaSUtils.h"

typedef enum {REQ_PROCESS, REQ_CONTROLLER, REQ_TRAIN, IDLE} LMState;

class MecFLLocalManagerApp : public MecAppBase
{
    private:
    //UDP socket to communicate with the UeApp
        inet::UdpSocket ueSocket;
        int localUePort;

        inet::L3Address ueAppAddress;
        int ueAppPort;

        inet::TcpSocket* serviceSocket_;
        inet::TcpSocket* mp1Socket_;
        inet::TcpSocket* flControllerSocket_;


        Endpoint flControllerEndpoint_;
        Endpoint flSericeProviderEndpoint_;
        Endpoint flLearnerEndpoint_;


        HttpBaseMessage* mp1HttpMessage;
        HttpBaseMessage* serviceHttpMessage; //for FLsp

        std::string fLServiceName_;
        std::string flProcessId_;
        bool learner_;

        LMState state_;

        int size_;
        std::string subId;
        std:: string resourceUrl_;

        cModule* learnerApp_;


    protected:
        virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;
        virtual void finish() override {};

//        virtual void handleProcessedMessage(omnetpp::cMessage *msg) override;

        virtual void handleHttpMessage(int connId) override;
        virtual void handleServiceMessage(int connId) override;
        virtual void handleMp1Message(int connId) override;
        virtual void handleUeMessage(omnetpp::cMessage *msg) override {};
//
//        virtual void modifySubscription();
//        virtual void sendSubscription();
//        virtual void sendDeleteSubscription();

        virtual void handleSelfMessage(cMessage *msg) override {};

        MecAppInstanceInfo* instantiateFLLearner();


//        /* TCPSocket::CallbackInterface callback methods */
       virtual void established(int connId) override;

    public:
       MecFLLocalManagerApp();
       virtual ~MecFLLocalManagerApp() {};

    };




#endif /* APPS_MEC_FLAAS_FLLOCALMANAGER_MECFLLOCALMANAGERAPP_H_ */
