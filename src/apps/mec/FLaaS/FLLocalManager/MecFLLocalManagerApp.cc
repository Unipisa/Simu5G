/*
 * MecFLLocalManagerApp.cc
 *
 *  Created on: Oct 18, 2022
 *      Author: alessandro
 */

#include "MecFLLocalManagerApp.h"
#include "nodes/mec/MECOrchestrator/MECOMessages/MECOrchestratorMessages_m.h"
#include "nodes/mec/MECPlatformManager/MecPlatformManager.h"
#include "common/utils/utils.h"

Define_Module(MecFLLocalManagerApp);


MecFLLocalManagerApp::MecFLLocalManagerApp()
{
    serviceSocket_ = nullptr;
    mp1Socket_ = nullptr;
    flControllerSocket_ = nullptr;
    learnerApp_ = nullptr;
}


MecFLLocalManagerApp::~MecFLLocalManagerApp()
{
    HttpMessageStatus* msgStatus = (HttpMessageStatus *)mp1Socket_->getUserData();
    cancelAndDelete((cMessage*)msgStatus->processMsgTimer);
    delete msgStatus;
    HttpMessageStatus* msgStatus1 = (HttpMessageStatus *)flControllerSocket_->getUserData();
        cancelAndDelete((cMessage*)msgStatus1->processMsgTimer);
        delete msgStatus1;

        HttpMessageStatus* msgStatus2 = (HttpMessageStatus *)serviceSocket_->getUserData();
            cancelAndDelete((cMessage*)msgStatus2->processMsgTimer);
            delete msgStatus2;



}


void MecFLLocalManagerApp::initialize(int stage)
{
    MecAppBase::initialize(stage);

    // avoid multiple initializations
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;

    EV << "MecFLLocalManagerApp::initialize - MEC application " << getClassName() << " with mecAppId[" << mecAppId << "] has started!" << endl;

    fLServiceName_ = par("fLServiceName").stdstringValue();

    ueAppPort = par("localUePort");
    ueSocket.setOutputGate(gate("socketOut"));
    ueSocket.bind(ueAppPort);

    // connect with the service registry
    EV << "MecFLLocalManagerApp::initialize - Initialize connection with the Service Registry via Mp1" << endl;

    // retrieve global PlatoonProducerApp endpoints

    mp1Socket_ = addNewSocket();

    //connect(mp1Socket_, mp1Address, mp1Port);

    state_ = IDLE;
    // fill the map with the number of instructions base on the msg type
}

void MecFLLocalManagerApp::handleMessage(cMessage *msg)
{
    if(!msg->isSelfMessage())
    {
        if(ueSocket.belongsToSocket(msg))
        {
            handleUeMessage(msg);
            return;
        }
    }
    MecAppBase::handleMessage(msg);
}

void MecFLLocalManagerApp::handleUeMessage(cMessage *msg)
{
    auto pkt = check_and_cast<inet::Packet*>(msg);
    auto flaasPacket = pkt->peekAtFront<FLaasPacket>();

    if(flaasPacket->getType() == LEARNER_DEPLOYMENT)
    {
        auto learnerDep = pkt->removeAtFront<LearnerDeploymentPacket>();
        EV << "MecFLLocalManagerApp::handleUeMessage - ML Learner instantiated on UE!!" <<learnerDep->getDeploymentPosition() << endl;
        if(learnerDep->getDeploymentPosition() == 0) //ON_UE
        {
            MecAppInstanceInfo* appInfo = instantiateFLLearnerOnUe(learnerDep);
            if(!appInfo->status)
            {
                throw omnetpp::cRuntimeError("MecFLLocalManagerApp::handleUeMessage - cannot instantiate LM learner!");
            }
            else
            {
                EV << "MecFLLocalManagerApp::handleUeMessage - ML Learner instantiated on UE!!" << endl;
                flLearnerEndpoint_.addr = appInfo->endPoint.addr;
                flLearnerEndpoint_.port = appInfo->endPoint.port;
                learnerApp_ = appInfo->module;
            }

        }
        else
        {
            MecAppInstanceInfo* appInfo = instantiateFLLearner();
            if(!appInfo->status)
            {
                throw omnetpp::cRuntimeError("MecFLLocalManagerApp::handleUeMessage - cannot instantiate LM learner!");
            }
            else
            {
                EV << "MecFLLocalManagerApp::initialize - ML Learner instantiated!!" << endl;
                flLearnerEndpoint_.addr = appInfo->endPoint.addr;
                flLearnerEndpoint_.port = appInfo->endPoint.port;
                learnerApp_ = appInfo->module;
            }
        }
        connect(mp1Socket_, mp1Address, mp1Port);
    }
    else if(flaasPacket->getType() == DATA_MSG)
    {
        EV << "MecFLLocalManagerApp::handleUeMessage - DATA_MSG!" << endl;
        auto dataMsg = pkt->removeAtFront<DataForTrainingPacket>();
        arrivedBytes_ += dataMsg->getDimension();
    }

    delete msg;
}

void MecFLLocalManagerApp::established(int connId)
{
    EV << "MecFLLocalManagerApp::established - connId [" << connId << "]" << endl;

    if (mp1Socket_ != nullptr && connId == mp1Socket_->getSocketId()) {
        EV << "MecFLLocalManagerApp::established - Mp1Socket" << endl;

        // once the connection with the Service Registry has been established, obtain the
        // endPoint (address+port) of the Location Service
        const char *uri = "/example/mec_service_mgmt/v1/services?ser_name=FLServiceProvider";
        std::string host = mp1Socket_->getRemoteAddress().str() + ":" + std::to_string(mp1Socket_->getRemotePort());

        Http::sendGetRequest(mp1Socket_, host.c_str(), uri);
    }
    else if (serviceSocket_ != nullptr && connId == serviceSocket_->getSocketId())
    {
        EV << "MecFLLocalManagerApp::established - serviceSocket_" << endl;
        // request the list of the active processes with category XAI
        std::stringstream uri;
        uri << "/example/flaas/v1/queries/activeProcesses";
        EV << "MecFLLocalManagerApp::request active processes: uri: " << uri.str() << endl;
        std::string host = serviceSocket_->getRemoteAddress().str() + ":" + std::to_string(serviceSocket_->getRemotePort());
        Http::sendGetRequest(serviceSocket_, host.c_str(), uri.str().c_str());
        state_ = REQ_PROCESS;
    }
    else if (flControllerSocket_ != nullptr && connId == flControllerSocket_->getSocketId())
    {
        // request to be in a training
        EV << "MecFLLocalManagerApp::established -flControllerSocket_ " << endl;
//        if(!learner_)
//            return;
        // request the list of the active processes with category XAI
        std::stringstream uri;
        uri << "/example/flController/operations/trainModel";
        EV << "MecFLLocalManagerApp::request active processes: uri: " << uri.str() << endl;
        std::string host = flControllerSocket_->getRemoteAddress().str() + ":" + std::to_string(flControllerSocket_->getRemotePort());
        nlohmann::json jsonBody;
        jsonBody["learnerId"] = learnerApp_->getId();
        jsonBody["lmId"] = learnerApp_->getId();

        jsonBody["learnerAddress"]  = flLearnerEndpoint_.addr.str();
        jsonBody["learnerPort"] = flLearnerEndpoint_.port;

        Http::sendPostRequest(flControllerSocket_, jsonBody.dump(0).c_str(), host.c_str(), uri.str().c_str());
        state_ = REQ_TRAIN;
    }

    else
    {
        throw cRuntimeError("MecAppBase::socketEstablished - Socket %d not recognized", connId);
    }
}

void MecFLLocalManagerApp::handleHttpMessage(int connId)
{
    if (mp1Socket_ != nullptr && connId == mp1Socket_->getSocketId()) {
        handleMp1Message(connId);
    }
    else                // if (connId == serviceSocket_->getSocketId())
    {
        handleServiceMessage(connId);
    }
//    else
//    {
//        throw cRuntimeError("Socket with connId [%d] is not present in thi application!", connId);
//    }
}

void MecFLLocalManagerApp::handleMp1Message(int connId)
{
    // for now I only have just one Service Registry
    HttpMessageStatus *msgStatus = (HttpMessageStatus*) mp1Socket_->getUserData();
    mp1HttpMessage = (HttpBaseMessage*) msgStatus->httpMessageQueue.front();
    EV << "MECPlatooningApp::handleMp1Message - payload: " << mp1HttpMessage->getBody() << endl;

    try {
        nlohmann::json jsonBody = nlohmann::json::parse(mp1HttpMessage->getBody()); // get the JSON structure
        if (!jsonBody.empty()) {
            jsonBody = jsonBody[0];
            std::string serName = jsonBody["serName"];
            if (serName.compare("FLServiceProvider") == 0) {
                if (jsonBody.contains("transportInfo")) {
                    nlohmann::json endPoint = jsonBody["transportInfo"]["endPoint"]["addresses"];
                    EV << "address: " << endPoint["host"] << " port: " << endPoint["port"] << endl;
                    std::string address = endPoint["host"];
                    flSericeProviderEndpoint_.addr = inet::L3AddressResolver().resolve(address.c_str());
                    flSericeProviderEndpoint_.port = endPoint["port"];
                    serviceSocket_ = addNewSocket();
                    EV << " MecFLLocalManagerApp::handleMp1Message - connecting to FL Service Provider" << endl;
                    connect(serviceSocket_, flSericeProviderEndpoint_.addr, flSericeProviderEndpoint_.port);
                }
            }
        }
    }
    catch (nlohmann::detail::parse_error e) {
        EV << e.what() << std::endl;
        // body is not correctly formatted in JSON, manage it
        return;
    }
}

void MecFLLocalManagerApp::handleServiceMessage(int connId)
{
    // for now I only have just one Service Registry
    HttpMessageStatus *msgStatus = nullptr;

    if(serviceSocket_ != nullptr && connId == serviceSocket_->getSocketId())
    {
        msgStatus= (HttpMessageStatus*) serviceSocket_->getUserData();
    }
    else if (flControllerSocket_ != nullptr && connId == flControllerSocket_->getSocketId())
    {
        msgStatus= (HttpMessageStatus*) flControllerSocket_->getUserData();
    }
    else
    {
        EV << "MecFLLocalManagerApp::handleServiceMessage - No socket with connid [" << connId << "] is present!" << endl;
        return;
    }

    serviceHttpMessage = (HttpBaseMessage*) msgStatus->httpMessageQueue.front();
    EV << "MECPlatooningApp::handleServiceMessage - payload: " << serviceHttpMessage->getBody() << endl;

    if (serviceHttpMessage->getType() == RESPONSE) {
        HttpResponseMessage *rspMsg = dynamic_cast<HttpResponseMessage*>(serviceHttpMessage);
        if (rspMsg->getCode() == 200) // in response to a successful GET request
        {
            nlohmann::json jsonBody;
            EV << "MecFLLocalManagerApp::handleServiceMessage - response 200 from Socket with Id [" << connId << "]" << endl;

//            received_++;

            try {
                jsonBody = nlohmann::json::parse(rspMsg->getBody()); // get the JSON structure
                //based on the state of the manager parse the response differently
                if(state_ == REQ_PROCESS)
                {
                    EV << "state_ == REQ_PROCESS" << endl;
                // then ask for the controllers ep
                    bool found = false;
                    nlohmann::json flProcessList = jsonBody["flProcessList"];
                    for(int i = 0; i < flProcessList.size(); ++i)
                    {
                        nlohmann::json process = flProcessList.at(i);
                        std::string name =  process["name"];
                        if(name.compare(fLServiceName_) == 0)
                        {
                            flProcessId_ = process["processId"];
                            found = true;
                            break;
                        }
                    }
                    if(found)
                    {
                        // request controller endpoint
                        std::stringstream uri;
                        uri << "/example/flaas/v1/queries/controllerEndpoint?processId=" << flProcessId_ << endl;
                        EV << "MecFLLocalManagerApp::request process endpoint!: uri: " << uri.str() << endl;
                        std::string host = serviceSocket_->getRemoteAddress().str() + ":" + std::to_string(serviceSocket_->getRemotePort());
                        Http::sendGetRequest(serviceSocket_, host.c_str(), uri.str().c_str());
                        state_ = REQ_CONTROLLER;
                    }
                }
                else if (state_ == REQ_CONTROLLER)
                {
                    EV << "state_ == REQ_CONTRLLER" << endl;
                // connect to controller to post the training mode
                    nlohmann::json controllerEndpoint = jsonBody["FLControllerEndpoint"];
                    std::string addr = controllerEndpoint["address"]["host"];
                    inet::L3Address add = inet::L3Address(addr.c_str());
                    int port = controllerEndpoint["address"]["port"];
                    flControllerEndpoint_ = {add, port};
                    flControllerSocket_ = addNewSocket();
                    connect(flControllerSocket_, add, port);
                }
                else
                {
                    // this should not happen
                }
            }
            catch (nlohmann::detail::parse_error e) {
                EV << e.what() << endl;
                // body is not correctly formatted in JSON, manage it
                return;
            }

        }
        else if(rspMsg->getCode() == 201)
        {
            // response of a post
            nlohmann::json jsonBody;
            EV << "MecFLLocalManagerApp::handleServiceMessage - response 200 from Socket with Id [" << connId << "]" << endl;

//            received_++;

            try {
                jsonBody = nlohmann::json::parse(rspMsg->getBody()); // get the JSON structure
                            //based on the state of the manager parse the response differently
                resourceUrl_ =  jsonBody["resourceURL"];
            EV << "MecFLLocalManagerApp::handleServiceMessage - response with HTTP code: "<< rspMsg->getCode() << " url: " << resourceUrl_ << endl;
            }
            catch (nlohmann::detail::parse_error e) {
                EV << e.what() << endl;
                // body is not correctly formatted in JSON, manage it
                return;
            }

        }

        // some error occured, show the HTTP code for now
        else {
            EV << "MecFLLocalManagerApp::handleServiceMessage - response with HTTP code:  " << rspMsg->getCode() << endl;
        }
    }
    // it is a request. It should not arrive, for now (think about sub-not)
    else {
        HttpRequestMessage *reqMsg = dynamic_cast<HttpRequestMessage*>(serviceHttpMessage);
        EV << "MecFLLocalManagerApp::handleServiceMessage - response with HTTP code:  " << reqMsg->getMethod() << " discarded" << endl;
    }
}

MecAppInstanceInfo* MecFLLocalManagerApp::instantiateFLLearnerOnUe(inet::Ptr<LearnerDeploymentPacket> learnerDep)
{
    std::string ueModuleName = learnerDep->getUeModuleName();
    cModule* ueModule = getModuleByPath(ueModuleName.c_str());

    cModuleType *moduleType = cModuleType::get(learnerDep->getLearnerModuleName());         //MEAPP module package (i.e. path!)
//    std::string learnerModuleName = learnerDep->getLearnerModuleName()
//    std::string meModuleName = std::string(learnerDep->getLearnerModuleName()).substr(__pos, __n)

    cModule *module = moduleType->create("learnerApp", ueModule);       //MEAPP module-name & its Parent Module
    std::stringstream appName;
    appName << "learnerApp_" << module->getId();
    module->setName(appName.str().c_str());
    EV << "MecFLLocalManagerApp::instantiateFLLearnerOnUe - meModuleName: " << appName.str() << endl;

//    displaying ME App dynamically created (after 70 they will overlap..)
    std::stringstream display;
    display << "p=" << 300 << "," << 50 << ";is=vs";
    module->setDisplayString(display.str().c_str());

    //initialize IMECApp Parameters THEY ARE NOT NEED HERE
    module->par("mecAppIndex") = 0;
    module->par("mecAppId") = 0;
    module->par("requiredRam") = 0.;
    module->par("requiredDisk") = 0.;
    module->par("requiredCpu") = 0.;
    module->par("localUePort") = learnerDep->getPort();
    module->par("mp1Address") = "";
    module->par("mp1Port") = 0;
    module->par("parentModule") = "UE";

    module->finalizeParameters();

    MecAppInstanceInfo* instanceInfo = new MecAppInstanceInfo();

    instanceInfo->status = true;
    instanceInfo->instanceId = appName.str();
    instanceInfo->endPoint.addr = inet::L3Address(inet::L3AddressResolver().resolve(learnerDep->getIpAddress(), inet::L3AddressResolver::ADDR_IPv4).toIpv4());
    instanceInfo->endPoint.port = learnerDep->getPort();
    instanceInfo->module = module;



    EV << "VirtualisationInfrastructureManager::instantiateMEApp port"<< instanceInfo->endPoint.port << endl;


    //connecting VirtualisationInfrastructure gates to the MEApp gates

    // add gates to the 'at' layer and connect them to the virtualisationInfr gates
    cModule *at = ueModule->getSubmodule("at");
    if(at == nullptr)
        throw cRuntimeError("at module, i.e. message dispatcher for SAP between application and transport layer non found");


    cGate* newAtInGate = at->getOrCreateFirstUnconnectedGate("in", 0, false, true);
    cGate* newAtOutGate = at->getOrCreateFirstUnconnectedGate("out", 0, false, true);

    newAtOutGate->connectTo(module->gate("socketIn"));
    module->gate("socketOut")->connectTo(newAtInGate);

    module->buildInside();
    module->scheduleStart(simTime());
    module->callInitialize();

    return instanceInfo;
}

MecAppInstanceInfo* MecFLLocalManagerApp::instantiateFLLearner()
{
    // get MEC platform manager and require mec app instantiation
    cModule* mecPlatformManagerModule = this->getModuleByPath("^.mecPlatformManager");
    MecPlatformManager* mecpm = check_and_cast<MecPlatformManager*>(mecPlatformManagerModule);

    MecAppInstanceInfo* appInfo = nullptr;
    CreateAppMessage * createAppMsg = new CreateAppMessage();

    std::string contName = par("FLLearner"); // is the path!

    //get the name of the app
    std::string appName = contName.substr(contName.rfind(".")+1, contName.length());

    createAppMsg->setUeAppID(getId()); // TODO choose id
    createAppMsg->setMEModuleName(appName.c_str());
    createAppMsg->setMEModuleType(contName.c_str()); //path

    createAppMsg->setRequiredCpu(5000.);
    createAppMsg->setRequiredRam(10.);
    createAppMsg->setRequiredDisk(10.);
    createAppMsg->setRequiredService("NULL"); // insert OMNeT like services, NULL if not

    appInfo = mecpm->instantiateMEApp(createAppMsg);

    if(!appInfo->status)
    {
        EV << "FLControllerApp::instantiateFLLearner - something went wrong during MEC app instantiation"<< endl;
    }

    else
    {
        EV << "FLControllerApp::instantiateFLLearner - successful MEC app instantiation"<< endl;
    }

    return appInfo;
}



