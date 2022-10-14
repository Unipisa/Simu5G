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

#include "apps/mec/FLaaS/FLController/FLControllerApp.h"

#include "nodes/mec/MECOrchestrator/MECOMessages/MECOrchestratorMessages_m.h"
#include "nodes/mec/MECPlatformManager/MecPlatformManager.h"
#include "apps/mec/FLaaS/FLComputationEngine/FLComputationEngine.h"

#include "common/utils/utils.h"

Define_Module(FLControllerApp);

FLControllerApp::FLControllerApp()
{
    baseUriQueries_ = "/example/flController/";
    baseUriSubscriptions_ = baseUriQueries_;
    subscriptionId_ = 0;
    subscriptions_.clear();
    instantiationMsg_ = nullptr;
    computationEngineModule_ = nullptr;
    learnersId_ = 0;
}


void FLControllerApp::initialize(int stage)
{
    EV << "FLControllerApp::initialize stage " << stage << endl;
    if (stage == inet::INITSTAGE_LOCAL)
    {
        EV << "MecServiceBase::initialize" << endl;
        serviceName_ = par("serviceName").stringValue();

        requestServiceTime_ = par("requestServiceTime");
        requestService_ = new cMessage("serveRequest");
        requestQueueSize_ = par("requestQueueSize");

        subscriptionServiceTime_ = par("subscriptionServiceTime");
        subscriptionService_ = new cMessage("serveSubscription");
        subscriptionQueueSize_ = par("subscriptionQueueSize");
        currentRequestMessageServed_ = nullptr;
        currentSubscriptionServed_ = nullptr;

        requestQueueSizeSignal_ = registerSignal("requestQueueSize");
        responseTimeSignal_ = registerSignal("responseTime");
        binder_ = getBinder();

        instantiationTime_ = par("instantiationTime");
        instantiationMsg_ = new cMessage("instantiationMsg");
        scheduleAfter(instantiationTime_, instantiationMsg_);
    }

    else if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        baseSubscriptionLocation_ = host_+ baseUriSubscriptions_ + "/";
    }

    inet::ApplicationBase::initialize(stage);
}

void FLControllerApp::handleMessageWhenUp(omnetpp::cMessage *msg)
{
    if(msg->isSelfMessage() && msg->isName("instantiationMsg"))
    {
        MecAppInstanceInfo* appInfo = instantiateFLComputationEngine();
        if(appInfo->status)
        {
            computationEngineModule_ = appInfo->module;
            FLComputationEngineApp* app = check_and_cast<FLComputationEngineApp*>(appInfo->module);
            app->setFLControllerPointer(this);
//            computationEngineModule_->par("controllerModuleName") = getFullPath();
        }
        delete appInfo;

    }
    else
    {
       MecServiceBase::handleMessageWhenUp(msg);
    }
}

void FLControllerApp::handleStartOperation(inet::LifecycleOperation *operation)
{
    EV << "FLControllerApp::handleStartOperation" << endl;
    const char *localAddress = par("localAddress");
    int localPort = par("localUePort");
    EV << "Local Address: " << localAddress << " port: " << localPort << endl;
    inet::L3Address localAdd(inet::L3AddressResolver().resolve(localAddress));
    EV << "Local Address resolved: "<< localAdd << endl;

    // e.g. 1.2.3.4:5050
    std::stringstream hostStream;
    hostStream << localAddress<< ":" << localPort;
    host_ = hostStream.str();

    serverSocket.setOutputGate(gate("socketOut"));
    serverSocket.setCallback(this);
    //serverSocket.bind(localAddress[0] ? L3Address(localAddress) : L3Address(), localPort);
    serverSocket.bind(inet::L3Address(), localPort); // bind socket for any address

    serverSocket.listen();
}

void FLControllerApp::handleGETRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)
{
    EV << "UALCMPApp::handleGETRequest" << endl;
    std::string uri = currentRequestMessageServed->getUri();

    // check it is a GET for a query or a subscription
    if(uri.compare(baseUriQueries_+"/model") == 0 ) //queries
    {
        std::string params = currentRequestMessageServed->getParameters();
        if(!params.empty())
        {
            std::vector<std::string> queryParameters = lte::utils::splitString(params, "&");

            // parameters can be trainMode or category
            std::vector<std::string>::iterator it  = queryParameters.begin();
            std::vector<std::string>::iterator end = queryParameters.end();
            std::vector<std::string> param;
            std::vector<std::string> splittedParam;

            // for now it only possible to have ONE parameters
            for(; it != end; ++it){
                if(it->rfind("lastTimestamp", 0) == 0) // accessPointId=par1,par2
                {
                    EV <<"FLControllerApp::handleGETReques - parameters: lastTimestamp " << endl;
                    param = lte::utils::splitString(*it, "=");
                    if(param.size()!= 2) //must be param=values
                    {
                       Http::send400Response(socket);
                       return;
                    }

                    // get last model inserted
                    const auto model = modelHistory_.back();
                    if(model.timestamp == std::atoi(param[1].c_str()))
                    {
                        Http::send200Response(socket, "");
                    }
                    else
                    {
                        Http::send200Response(socket, "{\"name\": \"send Model with BYTE!\"}");
                    }
                }
            }

        }
        else
        {
            // return last model
            const auto model = modelHistory_.back();

            Http::send200Response(socket, "{\"name\": \"send Model with BYTE!\"}");
        }
    }

    else //bad uri
    {
        Http::send404Response(socket);

    }

}
void FLControllerApp::handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)
{
    EV << "FLControllerApp::handlePOSTRequest" << endl;
    std::string uri = currentRequestMessageServed->getUri();
    std::string body = currentRequestMessageServed->getBody();

//    // uri must be in form example/flaas/v1/operations/res

    EV << "FLControllerApp::handlePOSTRequest - baseuri: "<< uri << endl;

    if(uri.compare(baseUriQueries_+"/operations/trainModel") == 0)
    {

        // TODO decide the stuff to include. e.g. battery level, device type
        nlohmann::json jsonBody;
        try
        {
            jsonBody = nlohmann::json::parse(body); // get the JSON structure
        }
        catch(nlohmann::detail::parse_error e)
        {
            std::cout << "FLControllerApp::handlePOSTRequest" << e.what() << "\n" << body << std::endl;
            // body is not correctly formatted in JSON, manage it
            Http::send400Response(socket); // bad body JSON
            return;
        }

        Endpoint learnerEndpoint;
        std::string ipAddress = jsonBody["learnerIpAddress"];
        learnerEndpoint.addr = inet::L3Address(ipAddress.c_str());
        learnerEndpoint.port = jsonBody["learnerPort"];


        inet::L3Address ip = socket->getRemoteAddress();
        int port = socket->getRemotePort();
        Endpoint endpoint = {ip, port};

        LocalManagerStatus newLocalManager(learnersId_++, endpoint, learnerEndpoint);

        // TODO make a decision based on information. For now it is always yes
        bool decision =  true;


        if(!decision)
        {
            EV << "FLControllerApp::handlePOSTRequest- The Local Manager [" << newLocalManager.getLocalManagerEndpoint().str() << "] can not be inserted in the training set " << endl;
            Http::send400Response(socket); // bad body JSON
            return;
        }
        else
        {
            EV << "FLControllerApp::handlePOSTRequest- The Local Manager [" << newLocalManager.getLocalManagerEndpoint().str() << "] inserted in the training set " << endl;
            learners_[newLocalManager.getLocalManagerId()] = newLocalManager;
            std::string resourceUrl = baseUriSubscriptions_+"/operations/trainModel/" + std::to_string(newLocalManager.getLocalManagerId());
            jsonBody["resourceURL"] = resourceUrl;
            std::pair<std::string, std::string> locationHeader("Location: ", resourceUrl);
            Http::send201Response(socket, jsonBody.dump(2).c_str(), locationHeader);
        }
    }
    else
    {
        Http::send404Response(socket); //resource not found
    }
}


AvailableLearnersMap* FLControllerApp::getLearnersEndpoint(int minLearners) {
    ASSERT(minLearners > 0);
    AvailableLearnersMap *learanersList = new AvailableLearnersMap();
    auto learnersIt =  learners_.begin();
    int i = 0;
    while (i < minLearners || learnersIt != learners_.end())
    {
        int id = learnersIt->second.getLocalManagerId();
        Endpoint ep = learnersIt->second.getLearnerEndpoint();
        AvailableLearner  learner = {id, ep};
        (*learanersList)[id] = learner;
    }

    return learanersList;
}


MecAppInstanceInfo* FLControllerApp::instantiateFLComputationEngine()
{
    // get MEC platform manager and require mec app instantiation
    cModule* mecPlatformManagerModule = this->getModuleByPath("^.mecPlatformManager");
    MecPlatformManager* mecpm = check_and_cast<MecPlatformManager*>(mecPlatformManagerModule);

    MecAppInstanceInfo* appInfo = nullptr;
    CreateAppMessage * createAppMsg = new CreateAppMessage();

//    const char* controllerName = par("controllerName"); // It must be in the form of MECPlatooningControllerRajamani;
    std::string contName = par("FLComputationEngineApp");

    //get the name of the app
    std::string appName = contName.substr(contName.rfind(".")+1, contName.length());

    createAppMsg->setUeAppID(par("localUePort")); // TODO choose id
    createAppMsg->setMEModuleName(appName.c_str());
    createAppMsg->setMEModuleType(contName.c_str()); //path

    createAppMsg->setRequiredCpu(5000.);
    createAppMsg->setRequiredRam(10.);
    createAppMsg->setRequiredDisk(10.);
    createAppMsg->setRequiredService("NULL"); // insert OMNeT like services, NULL if not

    appInfo = mecpm->instantiateMEApp(createAppMsg);

    if(!appInfo->status)
    {
        EV << "FLControllerApp::instantiateFLComputationEngine - something went wrong during MEC app instantiation"<< endl;
    }

    else
    {
        EV << "FLControllerApp::instantiateFLComputationEngine - succesfull MEC app instantiation"<< endl;
    }

    return appInfo;
}




