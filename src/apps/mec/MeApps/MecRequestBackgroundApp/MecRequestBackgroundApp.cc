/*
 * MecRequestBackgroundApp.cc
 *
 *  Created on: Dec 6, 2020
 *      Author: linofex
 */


#include "apps/mec/MeApps/MecRequestBackgroundApp/MecRequestBackgroundApp.h"
#include "nodes/mec/MEPlatform/MeServices/httpUtils/httpUtils.h"
#include "common/utils/utils.h"
#include <string>
#include "nodes/mec/MEPlatform/MeServices/packets/HttpRequestMessage/HttpRequestMessage.h"
#include "nodes/mec/MEPlatform/MeServices/packets/HttpResponseMessage/HttpResponseMessage.h"
#include "inet/common/TimeTag_m.h"

#include "inet/common/packet/chunk/BytesChunk.h"


using namespace inet;
Define_Module(MecRequestBackgroundApp);

MecRequestBackgroundApp::~MecRequestBackgroundApp(){
    cancelAndDelete(sendBurst);
    cancelAndDelete(burstPeriod);
    cancelAndDelete(burstTimer);
}


void MecRequestBackgroundApp::handleServiceMessage()
{
    EV << "payload: " <<  currentHttpMessage->getBody() << endl;
    if(burstFlag)
        scheduleAt(simTime() +exponential(0.01, 2),sendBurst);

}

void MecRequestBackgroundApp::established(int connId)
{
    EV << "Established " << endl;
    scheduleAt(simTime() + 5, burstTimer);

}

void MecRequestBackgroundApp::handleSelfMessage(cMessage *msg){
    if(strcmp(msg->getName(), "connect") == 0)
    {
        connect();
        delete msg;
        return;
    }
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


void MecRequestBackgroundApp::initialize(int stage){
    MeAppBase::initialize(stage);
    if(stage == inet::INITSTAGE_APPLICATION_LAYER)
    {
        numberOfApplications_ = par("numberOfApplications");

        if(numberOfApplications_ != 0)
        {
            cMessage *m = new cMessage("connect");
            sendBurst = new cMessage("sendBurst");
            burstPeriod = new cMessage("burstPeriod");
            burstTimer = new cMessage("burstTimer");
            burstFlag = false;
            scheduleAt(simTime() + 0.000012, m);
        }
    }
}

void MecRequestBackgroundApp::sendBulkRequest(){

    int numRequests = numberOfApplications_;
    std::string payload = "BulkRequest: 1";// + std::to_string(numberOfApplications_);

    Http::sendPacket(payload.c_str(), &socket);

    EV << "sent " << numRequests << " requests to the server"<<endl;
}


