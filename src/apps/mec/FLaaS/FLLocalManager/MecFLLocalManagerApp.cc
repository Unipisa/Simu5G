/*
 * MecFLLocalManagerApp.cc
 *
 *  Created on: Oct 18, 2022
 *      Author: alessandro
 */

#include "MecFLLocalManagerApp.h"

Define_Module(MecFLLocalManagerApp);


MecFLLocalManagerApp::MecFLLocalManagerApp()
{
    serviceSocket_ = nullptr;
    mp1Socket_ = nullptr;
    flControllerSocket_ = nullptr;
}



void MecFLLocalManagerApp::initialize(int stage)
{
    MecAppBase::initialize(stage);

    // avoid multiple initializations
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;

    EV << "MecFLLocalManagerApp::initialize - MEC application " << getClassName() << " with mecAppId[" << mecAppId << "] has started!" << endl;

    learner_ = par("hasLearner").boolValue();

    if(learner_)
    {
        //instantiate learner App
        // save it end point necessary for the FL comp engine
    }

    fLServiceName_ = par("fLServiceName").str();

    ueAppPort = par("localUePort");
    ueSocket.setOutputGate(gate("socketOut"));
    ueSocket.bind(ueAppPort);

    // connect with the service registry
    EV << "MecFLLocalManagerApp::initialize - Initialize connection with the Service Registry via Mp1" << endl;

    // retrieve global PlatoonProducerApp endpoints

    mp1Socket_ = addNewSocket();

    connect(mp1Socket_, mp1Address, mp1Port);

    state_ = IDLE;
    // fill the map with the number of instructions base on the msg type
}

void MecFLLocalManagerApp::established(int connId)
{
    EV << "MECPlatooningControllerApp::established - connId [" << connId << "]" << endl;

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
        EV << "MECPlatooningControllerApp::established - conection Id: "<< connId << endl;
        // request the list of the active processes with category XAI
        std::stringstream uri;
        uri << "/example/flaas/v1/queries/activeProcesses";
        EV << "MECPlatooningControllerApp::request active processes: uri: " << uri.str() << endl;
        std::string host = serviceSocket_->getRemoteAddress().str() + ":" + std::to_string(serviceSocket_->getRemotePort());
        Http::sendGetRequest(serviceSocket_, host.c_str(), uri.str().c_str());
        state_ = REQ_PROCESS;
    }
    else if (flControllerSocket_ != nullptr && connId == flControllerSocket_->getSocketId())
    {
        // request to be in a training
        EV << "MECPlatooningControllerApp::established - conection Id: "<< connId << endl;
        // request the list of the active processes with category XAI
        std::stringstream uri;
        uri << "/example/flController/operations/trainModel";
        EV << "MECPlatooningControllerApp::request active processes: uri: " << uri.str() << endl;
        std::string host = serviceSocket_->getRemoteAddress().str() + ":" + std::to_string(serviceSocket_->getRemotePort());
        nlohmann::json jsonBody;
        jsonBody["learnerAddress"]  = 3;
        jsonBody["learnerPort"] = 5;

        Http::sendPostRequest(serviceSocket_, jsonBody.dump(0).c_str(), host.c_str(), uri.str().c_str());
        state_ = REQ_TRAIN;
    }

//    else
//    {
//        throw cRuntimeError("MecAppBase::socketEstablished - Socket %d not recognized", connId);
//    }
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
    HttpMessageStatus *msgStatus = (HttpMessageStatus*) serviceSocket_->getUserData();
    serviceHttpMessage = (HttpBaseMessage*) msgStatus->httpMessageQueue.front();
    EV << "MECPlatooningApp::handleServiceMessage - payload: " << serviceHttpMessage->getBody() << endl;

    if (serviceHttpMessage->getType() == RESPONSE) {
        HttpResponseMessage *rspMsg = dynamic_cast<HttpResponseMessage*>(serviceHttpMessage);
        if (rspMsg->getCode() == 200) // in response to a successful GET request
        {
            nlohmann::json jsonBody;
            EV << "MECPlatooningControllerApp::handleServiceMessage - response 200 from Socket with Id [" << connId << "]" << endl;

//            received_++;

            try {
                jsonBody = nlohmann::json::parse(rspMsg->getBody()); // get the JSON structure
                //based on the state of the manager parse the response differently
                if(state_ == REQ_PROCESS)
                {
                // then ask for the controllers ep
                    bool found = false;
                    nlohmann::json flProcessList = jsonBody["flProcessList"];
                    for(int i = 0; i < flProcessList.size(); ++i)
                    {
                        nlohmann::json process = flProcessList.at(i);
                        if(process["name"] == fLServiceName_)
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
                        EV << "MECPlatooningControllerApp::request process endpoint!: uri: " << uri.str() << endl;
                        std::string host = serviceSocket_->getRemoteAddress().str() + ":" + std::to_string(serviceSocket_->getRemotePort());
                        Http::sendGetRequest(serviceSocket_, host.c_str(), uri.str().c_str());
                        state_ = REQ_CONTRLLER;
                    }
                }
                else if (state_ == REQ_CONTRLLER)
                {
                // connect to controller to post the training mode
                    nlohmann::json controllerEndpoint = jsonBody["FLControllerEndpoint"];
                    inet::L3Address add = inet::L3Address(controllerEndpoint["address"]["host"]);
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

        // some error occured, show the HTTP code for now
        else {
            EV << "MECPlatooningControllerApp::handleServiceMessage - response with HTTP code:  " << rspMsg->getCode() << endl;
        }
    }
    // it is a request. It should not arrive, for now (think about sub-not)
    else {
        HttpRequestMessage *reqMsg = dynamic_cast<HttpRequestMessage*>(serviceHttpMessage);
        EV << "MECPlatooningControllerApp::handleServiceMessage - response with HTTP code:  " << reqMsg->getMethod() << " discarded" << endl;
    }
}




