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


void MeAppGet::handleTcpMsg()
{
    EV << "payload: " <<  currentHttpMessage->getBody() << endl;
    //emit(responseTime_, simTime() - sendTimestamp);
//    sendMsg();
}

void MeAppGet::sendMsg(){
    std::string body = "";
    std::string uri = "/example/location/v2/queries/users?accessPointId=2";
    std::string host = socket.getRemoteAddress().str()+":"+std::to_string(socket.getRemotePort());
//    Http::sendPostRequest(&socket, body.c_str(), host.c_str(), uri.c_str());

    inet::Packet* packet = new inet::Packet("MecRequestPacket");
    auto reqPkt = inet::makeShared<HttpRequestMessage>();

    reqPkt->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    reqPkt->setMethod("GET");
    reqPkt->setBody(body.c_str());
    reqPkt->setUri(uri.c_str());
    reqPkt->setHost(host.c_str());
    reqPkt->setContentLength(body.length());
    reqPkt->setChunkLength(B(reqPkt->getPayload().size()));
    packet->insertAtBack(reqPkt);

//    Ptr<Chunk> chunkPayload;
//    const auto& bytesChunk = inet::makeShared<BytesChunk>();
//    std::string payload = reqPkt->getPayload();
//    std::vector<uint8_t> vec(payload.c_str(), payload.c_str() + payload.length());
//    bytesChunk->setBytes(vec);
//    chunkPayload = bytesChunk;
//    packet->insertAtBack(chunkPayload);


    socket.send(packet);
    EV << "SENT" << endl;

    sendTimestamp = simTime();
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


