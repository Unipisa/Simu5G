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

#include "apps/mec/MecApps/MecRequestBackgroundApp/MecRequestBackgroundApp.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "common/utils/utils.h"
#include <string>
#include "nodes/mec/MECPlatform/MECServices/packets/HttpRequestMessage/HttpRequestMessage.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.h"
#include "inet/common/TimeTag_m.h"

#include "inet/common/packet/chunk/BytesChunk.h"


using namespace inet;
Define_Module(MecRequestBackgroundApp);

MecRequestBackgroundApp::~MecRequestBackgroundApp(){
    cancelAndDelete(sendBurst);
    cancelAndDelete(burstPeriod);
    cancelAndDelete(burstTimer);
}

MecRequestBackgroundApp::MecRequestBackgroundApp()
{
    burstTimer = nullptr;
    burstPeriod = nullptr;
    sendBurst = nullptr;
    serviceSocket_ = nullptr;
    mp1Socket_ = nullptr;
    mp1HttpMessage = nullptr;
    serviceHttpMessage = nullptr;
}

void MecRequestBackgroundApp::handleServiceMessage(int connId)
{
    HttpMessageStatus *msgStatus = (HttpMessageStatus*) serviceSocket_->getUserData();
    serviceHttpMessage = (HttpBaseMessage*) msgStatus->httpMessageQueue.front();
    EV << "payload: " <<  serviceHttpMessage->getBody() << endl;
//    if(burstFlag)
//    scheduleAt(simTime() + exponential(lambda, 2));

}


void MecRequestBackgroundApp::initialize(int stage){
    if(stage != inet::INITSTAGE_APPLICATION_LAYER)
           return;
    MecAppBase::initialize(stage);
    mp1Socket_ = addNewSocket();
    cMessage *m = new cMessage("connectMp1");
    sendBurst = new cMessage("sendBurst");
    burstPeriod = new cMessage("burstPeriod");
    burstTimer = new cMessage("burstTimer");
    burstFlag = false;
    lambda = par("lambda").doubleValue();
    mecAppId = getId();
    scheduleAt(simTime() + 0, m);

}

void MecRequestBackgroundApp::sendRequest(){

    /*
     * For a background mec application application, I send the request as a bulkrequest.
     * This allow the service to only count the foreground requests, since they emit
     * the signal queuedRequests.
     *
     */
    std::string payload = "BulkRequest: 1";// + std::to_string(numberOfApplications_);
    Http::sendPacket(payload.c_str(), serviceSocket_);
    EV << "sent 1 request to the server"<<endl;
}



void MecRequestBackgroundApp::established(int connId)
{
    if(connId == mp1Socket_->getSocketId())
        {
            EV << "MecRequestBackgroundApp::established - Mp1Socket"<< endl;
            // get endPoint of the required service
            const char *uri = "/example/mec_service_mgmt/v1/services?ser_name=LocationService";
            std::string host = mp1Socket_->getRemoteAddress().str()+":"+std::to_string(mp1Socket_->getRemotePort());

            Http::sendGetRequest(mp1Socket_, host.c_str(), uri);
            return;
        }
        else if (connId == serviceSocket_->getSocketId())
        {
            EV << "MecRequestBackgroundApp::established - serviceSocket"<< endl;
            scheduleAt(simTime() + exponential(lambda, 2), burstTimer);
        }
        else
        {
            throw cRuntimeError("MecRequestBackgroundApp::socketEstablished - Socket %d not recognized", connId);
        }
}

void MecRequestBackgroundApp::handleSelfMessage(cMessage *msg){
    if(strcmp(msg->getName(), "burstTimer") == 0)
    {
        sendRequest();
        scheduleAt(simTime() + exponential(lambda, 2), burstTimer);
    }
    else if(strcmp(msg->getName(), "connectMp1") == 0)
    {
        EV << "MecAppBase::handleMessage- " << msg->getName() << endl;
        connect(mp1Socket_, mp1Address, mp1Port);
    }
    else if(strcmp(msg->getName(), "connectService") == 0)
    {
        EV << "MecAppBase::handleMessage- " << msg->getName() << endl;
//        connect(serviceSocket_, serviceAddress, servicePort);
        connect(serviceSocket_, mp1Address, mp1Port);
        delete msg;
    }
    else
    {
        EV << "MecRequestBackgroundApp::handleSelfMessage - selfMessage not recognized" << endl;
        delete msg;
    }
}


void MecRequestBackgroundApp::handleHttpMessage(int connId)
{
    if (mp1Socket_ != nullptr && connId == mp1Socket_->getSocketId()) {
        handleMp1Message(connId);
    }
    else
    {
        handleServiceMessage(connId);
    }
}
void MecRequestBackgroundApp::handleMp1Message(int connId)
{
    HttpMessageStatus *msgStatus = (HttpMessageStatus*) mp1Socket_->getUserData();
    mp1HttpMessage = (HttpBaseMessage*) msgStatus->httpMessageQueue.front();
    EV << "MecRequestBackgroundApp::handleMp1Message - payload: " << mp1HttpMessage->getBody() << endl;

    try
    {
        nlohmann::json jsonBody = nlohmann::json::parse(mp1HttpMessage->getBody()); // get the JSON structure
        if(!jsonBody.empty())
        {
            jsonBody = jsonBody[0];
            std::string serName = jsonBody["serName"];
            if(serName.compare("LocationService") == 0)
            {
                if(jsonBody.contains("transportInfo"))
                {
                    nlohmann::json endPoint = jsonBody["transportInfo"]["endPoint"]["addresses"];
                    EV << "address: " << endPoint["host"] << " port: " <<  endPoint["port"] << endl;
                    std::string address = endPoint["host"];
                    serviceAddress = L3AddressResolver().resolve(address.c_str());;
                    servicePort = endPoint["port"];
                    serviceSocket_ = addNewSocket();
                    connect(serviceSocket_, serviceAddress, servicePort);
                }

            }
        }

    }
    catch(nlohmann::detail::parse_error e)
    {
        EV <<  e.what() << std::endl;
        // body is not correctly formatted in JSON, manage it
        return;
    }

}

void MecRequestBackgroundApp::finish()
{
}

