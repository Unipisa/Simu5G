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


#ifndef APPS_MEC_FLAAS_FLCONTROLLER_FLCONTROLLERAPP_H_
#define APPS_MEC_FLAAS_FLCONTROLLER_FLCONTROLLERAPP_H_



#include "omnetpp.h"

#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "apps/mec/FLaaS/FLaaSUtils.h"

#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h"
#include "apps/mec/FLaaS/FLController/LocalManagerStatus.h"

//#include "apps/mec/MecApps/MecAppBase.h"
#include "nodes/mec/MECPlatform/MECServices/MECServiceBase/MecServiceBase.h"


using namespace std;
using namespace omnetpp;

typedef std::map<int, AvailableLearner> AvailableLearnersMap;
class MecAppInstanceInfo;


class FLControllerApp : public MecServiceBase
{
    int size_;
    std::string subId;

    double instantiationTime_;
    cMessage* instantiationMsg_;

    cModule* computationEngineModule_;

    std::vector<MLModel> modelHistory_;
    int learnersId_;
    std::map<int, LocalManagerStatus> learners_;

    //TODO think about subscriptions

    protected:
        virtual void initialize(int stage) override;
        virtual void finish() override;

        virtual void handleMessageWhenUp(omnetpp::cMessage *msg) override;
        void handleStartOperation(inet::LifecycleOperation *operation) override;

        // GET the list of available MEC app descriptors
        virtual void handleGETRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket) override;
        // POST the instantiation of a MEC app
        virtual void handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)   override;
        // PUT not implemented, yet
        virtual void handlePUTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)    override;
        // DELETE a MEC app previously instantiated
        virtual void handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket) override;

        MecAppInstanceInfo* instantiateFLComputationEngine();

    public:
       FLControllerApp();
       virtual ~FLControllerApp();

       AvailableLearnersMap* getLearnersEndpoint(int minLearners);
};





#endif /* APPS_MEC_FLAAS_FLCONTROLLER_FLCONTROLLERAPP_H_ */
