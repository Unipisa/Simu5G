/*
 * DeviceApp.cc
 *
 *  Created on: Dec 6, 2020
 *      Author: linofex
 */


#include "apps/mec/DeviceApp/DeviceApp.h"
#include "nodes/mec/MEPlatform/MeServices/httpUtils/httpUtils.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "common/utils/utils.h"
#include <string>
#include "nodes/mec/MEPlatform/MeServices/packets/HttpRequestMessage/HttpRequestMessage.h"
#include "nodes/mec/MEPlatform/MeServices/packets/HttpResponseMessage/HttpResponseMessage.h"
#include "inet/common/TimeTag_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"

#include "inet/common/packet/chunk/BytesChunk.h"
#include "nodes/mec/MEPlatform/MeServices/httpUtils/json.hpp"

#include "nodes/mec/LCMProxy/DeviceAppMessages/DeviceAppPacket_m.h"
#include "nodes/mec/LCMProxy/DeviceAppMessages/DeviceAppPacket_Types.h"




using namespace inet;
using namespace omnetpp;
Define_Module(DeviceApp);

DeviceApp::~DeviceApp(){}


void DeviceApp::handleLcmProxyMessage()
{
    EV << "DeviceApp::handleLcmProxyMessage: " <<  lcmProxyMessage->getBody() << endl;

    if(lcmProxyMessage->getType() == RESPONSE)
    {
        HttpResponseMessage * response = dynamic_cast<HttpResponseMessage*>(lcmProxyMessage);
        nlohmann::json jsonBody =  nlohmann::json::parse(lcmProxyMessage->getBody());
        if(strcmp(response->getCode(), "201") ==0)
        {
            /*
             * TODO manage cases with no responses
             */
            std::string contextUri = response->getHeaderField("Location");
            if(contextUri.empty())
            {
                //ERROR
                EV << "DeviceApp::handleLcmProxyMessage - ERROR"<< endl;
                return;
            }

            appContextUri = contextUri;
            mecAppEndPoint = jsonBody["appInfo"]["userAppInstanceInfo"]["referenceURI"];

            EV << "DeviceApp::handleLcmProxyMessage - reference URI of the application instance context is: " << appContextUri << endl;
            EV << "DeviceApp::handleLcmProxyMessage - endPOint of the mec application instance is: " << mecAppEndPoint << endl;


            std::vector<std::string> endPoint =  cStringTokenizer(mecAppEndPoint.c_str(), ":").asVector();
            EV << "vectore size: " << endPoint[0] << " e " << atoi(endPoint[1].c_str()) << endl;

            inet::Packet* packet = new inet::Packet("WarningAlertPacketStart");
            auto ack = inet::makeShared<DeviceAppStartAckPacket>();

            //instantiation requirements and info
            ack->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
            ack->setType(ACK_START_MECAPP);

            //connection info
            ack->setResult("ACK");
            ack->setIpAddress(endPoint[0].c_str());
            ack->setPort(atoi(endPoint[1].c_str()));

            ack->setChunkLength(inet::B(20));

            packet->insertAtBack(ack);

            ueAppSocket_.sendTo(packet, ueAppAddress, ueAppPort);
        }
    }
}

void DeviceApp::established(int connId)
{

//    close();

}

void DeviceApp::handleUeMessage(){}

void DeviceApp::handleSelfMessage(cMessage *msg){
    if(strcmp(msg->getName(), "connect") == 0)
    {
        connectToLcmProxy();
        delete msg;
    }
    else if(strcmp(msg->getName(), "processedLcmProxyMessage") == 0)
    {
        handleLcmProxyMessage();
    }
}

void DeviceApp::initialize(int stage){

    if(stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;
    localPort = par("localPort");

    ueAppSocket_.setOutputGate(gate("socketOut"));
    lcmProxySocket_.setOutputGate(gate("socketOut"));

    ueAppSocket_.bind(localPort); // bind ueSocket to listen on local port
    lcmProxySocket_.bind(8740); // bind ueSocket to listen on local port


    ueAppSocket_.setCallback(this);

    lcmProxySocket_.setCallback(this);

    const char *lcmAddress = par("lcmProxyAddress").stringValue();
    lcmProxyAddress = L3AddressResolver().resolve(lcmAddress);
    lcmProxyPort = par("lcmProxyPort");

    processedLcmProxyMessage = new cMessage("processedLcmProxyMessage");

    cMessage *msg = new cMessage("connect");
    scheduleAt(simTime()+0.05 , msg);
}


void DeviceApp::handleMessage(omnetpp::cMessage *msg)
{
    if(msg->isSelfMessage())
    {
        handleSelfMessage(msg);
        return;
    }

    else if(ueAppSocket_.belongsToSocket(msg))
    {
        ueAppSocket_.processMessage(msg);
    }
    else if(lcmProxySocket_.belongsToSocket(msg))
    {
        lcmProxySocket_.processMessage(msg);
    }

//    delete msg;

}


void DeviceApp::connectToLcmProxy()
{
    // we need a new connId if this is not the first connection
    lcmProxySocket_.renewSocket();

    if (lcmProxyAddress.isUnspecified()) {
        EV_ERROR << "Connecting to " << lcmProxyAddress << " port=" << lcmProxyPort << ": cannot resolve destination address\n";
    }
    else {
        EV_INFO << "Connecting to " << lcmProxyAddress << " port=" << lcmProxyPort << endl;
        lcmProxySocket_.connect(lcmProxyAddress, lcmProxyPort);
    }
}

void DeviceApp::sendStartAppContext(Packet *pk)
{
    EV << "DeviceApp::sendStartAppContext" << endl;

    DeviceAppStartPacket* startPk = check_and_cast<DeviceAppStartPacket*>(pk);

    nlohmann::json jsonBody;

    jsonBody["associateDevAppId"] = std::to_string(getId());

    jsonBody["appInfo"]["appDId"] = startPk->getMecAppDId();//"WAMECAPP";
    //    jsonBody["appInfo"]["appPackageSource"] = "ApplicationDescriptors/WarningAlertApp.json";

    jsonBody["appInfo"]["appName"] = startPk->getMecAppName();//"MEWarningAlertApp_rest";
    jsonBody["appInfo"]["appProvider"] = startPk->getMecAppProvider();//"lte.apps.mec.warningAlert_rest.MEWarningAlertApp_rest";

    const char *uri = "/example/dev_app/v1/app_contexts";


    std::string host = lcmProxySocket_.getRemoteAddress().str()+":"+std::to_string(lcmProxySocket_.getRemotePort());

    Http::sendPostRequest(&lcmProxySocket_, jsonBody.dump().c_str(), host.c_str(), uri);
    return;
}

void DeviceApp::sendStopAppContext(Packet *pk)
{
    std::string host = lcmProxySocket_.getRemoteAddress().str()+":"+std::to_string(lcmProxySocket_.getRemotePort());
    Http::sendDeleteRequest(&lcmProxySocket_, host.c_str(), appContextUri.c_str());
}


// ------ UDP socket Callback implementation

void DeviceApp::socketDataArrived(UdpSocket *socket, Packet *pk)
{
    // determine its source address/port
    ueAppAddress = pk->getTag<L3AddressInd>()->getSrcAddress();
    ueAppPort = pk->getTag<L4PortInd>()->getSrcPort();

//    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto pkt = pk->peekAtFront<DeviceAppPacket>();
    if(strcmp(pkt->getType(), START_MECAPP) == 0)
    {
        sendStartAppContext(pk);
    }
    else if(strcmp(pkt->getType(), STOP_MECAPP) == 0 == 0)
    {
        sendStopAppContext(pk);
    }

//    pk->clearTags();
//    pk->trim();
//
//    std::vector<uint8_t> bytes =  pk->peekDataAsBytes()->getBytes();
//    std::string packet(bytes.begin(), bytes.end());
//
//    if(packet.compare("START") == 0)
//    {
//        sendStartAppContext();
//    }
//    else if(packet.compare("STOP") == 0)
//    {
//        sendStopAppContext();
//    }
//    else
//    {
//        throw cRuntimeError("Message from ue: %s non recognised", packet.c_str());
//    }
}

void DeviceApp::socketErrorArrived(UdpSocket *socket, Indication *indication)
{
    EV_WARN << "Ignoring UDP error report " << indication->getName() << endl;
    delete indication;
}

void DeviceApp::socketClosed(UdpSocket *socket)
{
    EV << "DeviceApp::socketClosed" << endl;
}


// ------ TCP socket Callback implementation
void DeviceApp::socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool urgent)
{
    EV << "DeviceApp::socketDataArrived" << endl;
    std::vector<uint8_t> bytes =  msg->peekDataAsBytes()->getBytes();
    std::string packet(bytes.begin(), bytes.end());
//    EV << packet << endl;

    bool res = Http::parseReceivedMsg(packet, &lcmProxyMessageBuffer, &lcmProxyMessage);
    if(res)
    {
        lcmProxyMessage->setSockId(lcmProxySocket_.getSocketId());
        double time = 0.005;
        scheduleAt(simTime()+time, processedLcmProxyMessage);
    }
}
void DeviceApp::socketEstablished(inet::TcpSocket *socket)
{
    EV <<"DeviceApp::socketEstablished - lcmProxySocket established " << endl;
}
void DeviceApp::socketPeerClosed(inet::TcpSocket *socket)
{
    EV << "DeviceApp::socketPeerClosed"<< endl;
    if (lcmProxySocket_.getState() == TcpSocket::PEER_CLOSED) {
           EV_INFO << "remote TCP closed, closing here as well\n";
           lcmProxySocket_.close();
       }

}
void DeviceApp::socketClosed(inet::TcpSocket *socket) {}
void DeviceApp::socketFailure(inet::TcpSocket *socket, int code) {}



void DeviceApp::finish()
{}
