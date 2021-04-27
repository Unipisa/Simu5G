//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "apps/mec/RNIDelay/MECRniDelayApp.h"
#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/Packet_m.h"

#include "nodes/mec/MEPlatform/MeServices/httpUtils/httpUtils.h"
#include "nodes/mec/MEPlatform/MeServices/httpUtils/json.hpp"
#include "nodes/mec/MEPlatform/MeServices/packets/HttpResponseMessage/HttpResponseMessage.h"

#include "nodes/mec/MEPlatform/MEAppPacket_Types.h"
#include "nodes/mec/MEPlatform/MEAppPacket_m.h"

Define_Module(MECRniDelayApp);

using namespace inet;
using namespace omnetpp;

MECRniDelayApp::MECRniDelayApp()
{
    meHost = nullptr;
    mePlatform = nullptr;;
    serviceRegistry = nullptr;;

}
MECRniDelayApp::~MECRniDelayApp()
{
    cancelAndDelete(sendGet);
    cancelAndDelete(sendUe);

}


void MECRniDelayApp::initialize(int stage)
{
    MeAppBase::initialize(stage);
    EV << "MECRniDelayApp::initialize - stage " << stage << endl;
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    //retrieving parameters
    size_ = par("packetSize");
    ueSimbolicAddress = (char*)par("ueSimbolicAddress").stringValue();
    meHostSimbolicAddress = (char*)par("localAddress").stringValue();
    destAddress_ = inet::L3AddressResolver().resolve(ueSimbolicAddress);
    destPort_ = par("uePort");
    localPort_ = par("localPort");

    // set Udp Socket
    ueSocket.setOutputGate(gate("socketOut"));
    ueSocket.bind(localPort_);

//    // set Tcp Socket
//    socket.bind(L3AddressResolver().resolve(meHostSimbolicAddress), localPort_);
//
//    socket.setCallback(this);
//    socket.setOutputGate(gate("socketOut"));

    l2Delay = registerSignal("RNIL2Delay");

    cMessage *m = new cMessage("connect");
    scheduleAt(simTime()+0.01, m);

    sendGet = new cMessage("sendGet");
    sendUe = new cMessage("sendUe");
    getPeriod = par("getPeriod");
    sendUePerdiod = par("period");
    threshold = par("threshold");
    thresholdCount = par("thresholdCount");


    //testing
    EV << "MECRniDelayApp::initialize - MECRniDelayApp Symbolic Address: " << meHostSimbolicAddress << endl;
    EV << "MECRniDelayApp::initialize - UEWarningAlertApp Symbolic Address: " << ueSimbolicAddress <<  " [" << destAddress_.str() << "]" << endl;

}


void MECRniDelayApp::finish(){

    EV << "MECRniDelayApp::finish - Sending " << STOP_MEAPP << " to the MEIceAlertService" << endl;

    if(gate("socketOut")->isConnected()){
        // TODO
//        WarningAlertPacket* packet = new WarningAlertPacket();
//        packet->setType(STOP_MEAPP);
//
//        send((cMessage*)packet, "socketOut");
    }
}

void MECRniDelayApp::handleServiceMessage()
{
    EV << "MECRniDelayApp::handleServiceMessage()" << endl;
//    if(currentHttpMessage->getType() == REQUEST)
//    {
//        Http::send204Response(&socket);
//        nlohmann::json jsonBody;
//        EV << "MEClusterizeService::handleTcpMsg - REQUEST " << currentHttpMessage->getBody()<< endl;
//        try
//        {
//
//           jsonBody = nlohmann::json::parse(currentHttpMessage->getBody()); // get the JSON structure
//        }
//        catch(nlohmann::detail::parse_error e)
//        {
//           EV <<  e.what() << endl;
//           // body is not correctly formatted in JSON, manage it
//           return;
//        }
//    }
    if(currentHttpMessage->getType() == RESPONSE)
    {
        HttpResponseMessage *rspMsg = dynamic_cast<HttpResponseMessage*>(currentHttpMessage);

        nlohmann::json jsonBody;
        EV << "MEClusterizeService::handleServiceMessage - RESPONSE " << endl;
        try
        {

           jsonBody = nlohmann::json::parse(currentHttpMessage->getBody()); // get the JSON structure
        }
        catch(nlohmann::detail::parse_error e)
        {
           EV <<  e.what() << endl;
           // body is not correctly formatted in JSON, manage it
           return;
        }
        if(jsonBody.contains("cellUEInfo"))
        {
            auto address = jsonBody["cellUEInfo"]["associatedId"]["value"];
            int delay = jsonBody["cellUEInfo"]["dl_nongbr_delay_ue"];
            EV <<"MECRniDelayApp::handleServiceMessage() - Address: "<< address << endl;
            EV <<"MECRniDelayApp::handleServiceMessage() - Delay: "<< delay << endl;
            emit(l2Delay, delay);
        }
        scheduleAt(simTime()+getPeriod, sendGet);
    }
}

void MECRniDelayApp::connect()
{
//    socket.renewSocket();
    EV << "MECRniDelayApp::connect" << endl;

    // use the service Registry to get ip and port of the service
    RniServiceSockAddress = serviceRegistry->retrieveMeService("RNIService");
    if(RniServiceSockAddress.addr.getType() == L3Address::NONE)
        throw cRuntimeError("MECRniDelayApp::connect - Mec Service RNIService not found!");
    EV << "MECRniDelayApp::connect to:  " << RniServiceSockAddress.str() << endl;
    socket.connect(RniServiceSockAddress.addr, RniServiceSockAddress.port);
}

void MECRniDelayApp::established(int connId)
{
    EV << "MECRniDelayApp::established" << endl;
    scheduleAt(simTime()+sendUePerdiod, sendUe);
    scheduleAt(simTime()+getPeriod, sendGet);
}

void MECRniDelayApp::sendRNISRequest()
{
    const string uri = "/example/rni/v2/queries/layer2_meas?ue_ipv4_address=acr:"+L3AddressResolver().resolve(ueSimbolicAddress).str();
    EV << "MECRniDelayApp::sendRNISRequest - get "<< uri << endl;
    std::string host = socket.getRemoteAddress().str()+":"+std::to_string(socket.getRemotePort());
    Http::sendGetRequest(&socket, "GET", uri.c_str(), host.c_str());
}

void MECRniDelayApp::sendDataToUe()
{
    EV << "MECRniDelayApp::sendDataToUe - sendData to UE" << endl;
    // generate and send a packet
    Packet *pkt = new Packet("DataPkt");
    const auto& payload = makeShared<MEAppPacket>();
    payload->addTag<CreationTimeTag>()->setCreationTime(simTime());
    payload->setType(INFO_MEAPP);
    payload->setChunkLength(inet::B(size_));
    pkt->insertAtBack(payload);

    ueSocket.sendTo(pkt, destAddress_, destPort_);
    scheduleAt(simTime()+sendUePerdiod, sendUe);
}

void MECRniDelayApp::handleSelfMessage(cMessage *msg)
{

    if(strcmp(msg->getName(), "connect") == 0)
    {
        connect();
        delete msg;
    }
    else if(strcmp(msg->getName(), "sendUe") == 0)
    {
        sendDataToUe();
    }
    else if(strcmp(msg->getName(), "sendGet") == 0)
    {
        sendRNISRequest();
    }

}
