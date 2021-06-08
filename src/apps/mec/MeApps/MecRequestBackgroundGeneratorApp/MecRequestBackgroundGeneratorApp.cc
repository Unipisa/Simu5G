/*
 * MecRequestBackgroundGeneratorApp.cc
 *
 *  Created on: Dec 6, 2020
 *      Author: linofex
 */


#include "apps/mec/MeApps/MecRequestBackgroundGeneratorApp/MecRequestBackgroundGeneratorApp.h"
#include "nodes/mec/MEPlatform/MeServices/httpUtils/httpUtils.h"
#include "common/utils/utils.h"
#include <string>
#include "nodes/mec/MEPlatform/MeServices/packets/HttpRequestMessage/HttpRequestMessage.h"
#include "nodes/mec/MEPlatform/MeServices/packets/HttpResponseMessage/HttpResponseMessage.h"
#include "inet/common/TimeTag_m.h"

#include "inet/common/packet/chunk/BytesChunk.h"


using namespace inet;
Define_Module(MecRequestBackgroundGeneratorApp);

MecRequestBackgroundGeneratorApp::~MecRequestBackgroundGeneratorApp(){}


void MecRequestBackgroundGeneratorApp::handleServiceMessage()
{
    EV << "payload: " <<  serviceHttpMessage->getBody() << endl;
    if(burstFlag)
        scheduleAt(simTime() + 0, sendBurst);

}

void MecRequestBackgroundGeneratorApp::established(int connId)
{
    if(connId == mp1Socket_.getSocketId())
        {
            EV << "MecRequestBackgroundGeneratorApp::established - Mp1Socket"<< endl;
            // get endPoint of the required service
            const char *uri = "/example/mec_service_mgmt/v1/services?ser_name=LocationService";
            std::string host = mp1Socket_.getRemoteAddress().str()+":"+std::to_string(mp1Socket_.getRemotePort());

            Http::sendGetRequest(&mp1Socket_, host.c_str(), uri);
            return;
        }
        else if (connId == serviceSocket_.getSocketId())
        {
            EV << "MecRequestBackgroundGeneratorApp::established - serviceSocket"<< endl;
            scheduleAt(simTime() + 5, burstTimer);
        }
        else
        {
            throw cRuntimeError("MecRequestBackgroundGeneratorApp::socketEstablished - Socket %s not recognized", connId);
        }
}

void MecRequestBackgroundGeneratorApp::handleSelfMessage(cMessage *msg){
    if(strcmp(msg->getName(), "burstTimer") == 0)
    {
        burstFlag = (burstFlag == true)? false : true;
        if(burstFlag){
            sendBulkRequest();
            scheduleAt(simTime()+0.5, burstPeriod);
        }
        scheduleAt(simTime()+5,burstTimer);
    }
    if(strcmp(msg->getName(), "burstPeriod") == 0)
        {
            burstFlag = false;
//            if(burstFlag)
//                scheduleAt(simTime()+0.5, burstPeriod);
        }
    if(strcmp(msg->getName(), "sendBurst") == 0)
    {
        sendBulkRequest();
    }

}


void MecRequestBackgroundGeneratorApp::handleMp1Message()
{
//    throw cRuntimeError("QUiI");
    EV << "MEWarningAlertApp_rest::handleMp1Message - payload: " << mp1HttpMessage->getBody() << endl;

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
                    connect(&serviceSocket_, serviceAddress, servicePort);
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


void MecRequestBackgroundGeneratorApp::initialize(int stage){
    MecAppBase::initialize(stage);
    if(stage == inet::INITSTAGE_APPLICATION_LAYER)
    {
        numberOfApplications_ = par("numberOfApplications");

        if(numberOfApplications_ != 0)
        {
            if(vim != nullptr)
                throw cRuntimeError("MecRequestBackgroundGeneratorApp::initialize - vim is null!");
            bool res = vim->registerMecApp(getId(), requiredRam, requiredDisk, requiredCpu);
            if(res == false)
            {
                throw cRuntimeError("MecRequestBackgroundGeneratorApp::initialize - RequestBackgroundGeneratorApp cannot be instatiated");
            }
            cMessage *m = new cMessage("connect");
            sendBurst = new cMessage("sendBurst");
            burstPeriod = new cMessage("burstPeriod");
            burstTimer = new cMessage("burstTimer");
            burstFlag = false;
            scheduleAt(simTime() + 0.055, m);
        }
    }
}

void MecRequestBackgroundGeneratorApp::sendBulkRequest(){

    int numRequests = truncnormal(numberOfApplications_, 20, 2);
    std::string payload = "BulkRequest: " + std::to_string(numRequests+1);

    Http::sendPacket(payload.c_str(), &serviceSocket_);

    EV << "MecRequestBackgroundGeneratorApp: " << numRequests << " requests to the server"<<endl;
}


