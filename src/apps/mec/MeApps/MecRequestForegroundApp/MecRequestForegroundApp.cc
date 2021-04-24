/*
 * MecRequestForegroundApp.cc
 *
 *  Created on: Dec 6, 2020
 *      Author: linofex
 */


#include "apps/mec/MeApps/MecRequestForegroundApp/MecRequestForegroundApp.h"
#include "nodes/mec/MEPlatform/MeServices/httpUtils/httpUtils.h"
#include "common/utils/utils.h"
#include <string>
#include "nodes/mec/MEPlatform/MeServices/packets/HttpRequestMessage/HttpRequestMessage.h"
#include "nodes/mec/MEPlatform/MeServices/packets/HttpResponseMessage/HttpResponseMessage.h"
#include "inet/common/TimeTag_m.h"

#include "inet/common/packet/chunk/BytesChunk.h"


using namespace inet;
Define_Module(MecRequestForegroundApp);

MecRequestForegroundApp::~MecRequestForegroundApp(){
    cancelAndDelete(sendFGRequest);
}


void MecRequestForegroundApp::handleServiceMessage()
{
    EV << "payload: " <<  currentHttpMessage->getBody() << endl;
    scheduleAt(simTime() + 0.01, sendFGRequest);
}

void MecRequestForegroundApp::established(int connId)
{
    EV << "Established " << endl;
    scheduleAt(simTime() + 0.01, sendFGRequest);
}

void MecRequestForegroundApp::handleSelfMessage(cMessage *msg){
    if(strcmp(msg->getName(), "connect") == 0)
    {
        connect();
        delete msg;
        return;
    }
    if(strcmp(msg->getName(), "sendFGRequest") == 0)
    {
        sendMsg();
    }
}

void MecRequestForegroundApp::initialize(int stage){
    MeAppBase::initialize(stage);
    if(stage == inet::INITSTAGE_APPLICATION_LAYER)
    {
            cMessage *m = new cMessage("connect");
            sendFGRequest = new cMessage("sendFGRequest");
            scheduleAt(simTime() + 0.000012, m);
    }
}

void MecRequestForegroundApp::sendMsg(){
    const char * body = "";
    const char *uri = "/example/location/v2/queries/users";
    std::string host = socket.getRemoteAddress().str()+":"+std::to_string(socket.getRemotePort());

    Http::sendGetRequest(&socket, "GET", uri, host.c_str());
    return;
}


