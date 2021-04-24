/*
 * MeAppGet.cc
 *
 *  Created on: Dec 6, 2020
 *      Author: linofex
 */


#include "apps/mec/MeApps/MeAppGet.h"
#include "nodes/mec/MEPlatform/MeServices/httpUtils/httpUtils.h"
#include "common/utils/utils.h"
#include <string>
#include "nodes/mec/MEPlatform/MeServices/packets/HttpRequestMessage/HttpRequestMessage.h"
#include "nodes/mec/MEPlatform/MeServices/packets/HttpResponseMessage/HttpResponseMessage.h"
#include "inet/common/TimeTag_m.h"

#include "inet/common/packet/chunk/BytesChunk.h"


using namespace inet;
Define_Module(MeAppGet);

MeAppGet::~MeAppGet(){}


void MeAppGet::handleServiceMessage()
{
    EV << "payload: " <<  currentHttpMessage->getBody() << endl;
    //emit(responseTime_, simTime() - sendTimestamp);
//    sendMsg();
}

void MeAppGet::sendMsg(){
    const char * body = "";
    const char *uri = par("uri");
    std::string host = socket.getRemoteAddress().str()+":"+std::to_string(socket.getRemotePort());

    Http::sendGetRequest(&socket, "GET", uri, host.c_str());
    return;

}
void MeAppGet::established(int connId)
{
    sendMsg();
//    close();

}

void MeAppGet::handleSelfMessage(cMessage *msg){
    if(strcmp(msg->getName(), "connect") == 0)
    {
        connect();
    }
    delete msg;

}

void MeAppGet::initialize(int stage){
    MeAppBase::initialize(stage);
    if(stage == inet::INITSTAGE_APPLICATION_LAYER)
    {
        cMessage *m = new cMessage("connect");
        scheduleAt(simTime()+1, m);


        responseTime_ = registerSignal("responseTime");
    }
}


