/*
 * DeviceApp.cc
 *
 *  Created on: Dec 6, 2020
 *      Author: linofex
 */


#include "apps/mec/DeviceApp/DeviceApp.h"
#include "nodes/mec/MEPlatform/MeServices/httpUtils/httpUtils.h"
#include "common/utils/utils.h"
#include <string>
#include "nodes/mec/MEPlatform/MeServices/packets/HttpRequestMessage/HttpRequestMessage.h"
#include "nodes/mec/MEPlatform/MeServices/packets/HttpResponseMessage/HttpResponseMessage.h"
#include "inet/common/TimeTag_m.h"

#include "inet/common/packet/chunk/BytesChunk.h"
#include "nodes/mec/MEPlatform/MeServices/httpUtils/json.hpp"



using namespace inet;
Define_Module(DeviceApp);

DeviceApp::~DeviceApp(){}


void DeviceApp::handleServiceMessage()
{
    EV << "payload: " <<  currentHttpMessage->getBody() << endl;
    //emit(responseTime_, simTime() - sendTimestamp);
//    sendMsg();


    if(flag)
    {
//        const char *uri = "/example/dev_app/v1/app_contexts/0";
//        std::string host = socket.getRemoteAddress().str()+":"+std::to_string(socket.getRemotePort());
//        Http::sendDeleteRequest(&socket, host.c_str(), uri);
//        flag = false;
    }
}

void DeviceApp::sendMsg(){

    nlohmann::json jsonBody;

    jsonBody["associateDevAppId"] = std::to_string(getId());

    jsonBody["appInfo"]["appDId"] = "WAMECAPP";
    jsonBody["appInfo"]["appName"] = "MEWarningAlertApp_rest";
    jsonBody["appInfo"]["appProvider"] = "lte.apps.mec.warningAlert_rest.MEWarningAlertApp_rest";

    const char *uri = "/example/dev_app/v1/app_list";


    std::string host = socket.getRemoteAddress().str()+":"+std::to_string(socket.getRemotePort());

    Http::sendGetRequest(&socket,host.c_str(), uri);
    return;

}
void DeviceApp::established(int connId)
{
    sendMsg();
//    close();

}

void DeviceApp::handleSelfMessage(cMessage *msg){
    if(strcmp(msg->getName(), "connect") == 0)
    {
        connect();
    }
    delete msg;

}

void DeviceApp::initialize(int stage){
    TcpAppBase::initialize(stage);


    if(stage == inet::INITSTAGE_APPLICATION_LAYER)
    {
        cMessage *m = new cMessage("connect");
        scheduleAt(simTime()+1, m);
flag = true;
        processedServiceResponse = new cMessage("processedServiceResponse");

        responseTime_ = registerSignal("responseTime");
    }
}


