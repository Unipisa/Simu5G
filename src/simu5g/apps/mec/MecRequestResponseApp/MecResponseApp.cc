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

#include "simu5g/apps/mec/MecRequestResponseApp/MecResponseApp.h"

#include <fstream>

#include <inet/common/TimeTag_m.h>
#include <inet/common/packet/Packet_m.h>
#include <inet/networklayer/common/L3AddressTag_m.h>
#include <inet/transportlayer/common/L4PortTag_m.h>

#include "simu5g/apps/mec/MecRequestResponseApp/packets/RequestResponsePacket_m.h"
#include "simu5g/mec/utils/httpUtils/httpUtils.h"
#include "simu5g/mec/utils/httpUtils/json.hpp"
#include "simu5g/mec/platform/services/messages/HttpResponseMessage.h"

namespace simu5g {

Define_Module(MecResponseApp);

using namespace inet;
using namespace omnetpp;

MecResponseApp::~MecResponseApp()
{
    delete currentRequestfMsg_;
    cancelAndDelete(processingTimer_);
}

void MecResponseApp::initialize(int stage)
{
    MecAppBase::initialize(stage);

    // avoid multiple initializations
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;

    EV << "MecResponseApp::initialize - MEC application " << getClassName() << " with mecAppId[" << mecAppId << "] has started!" << endl;

    localUePort_ = par("localUePort");
    ueAppSocket_.setOutputGate(gate("socketOut"));
    ueAppSocket_.bind(localUePort_);

    packetSize_ = B(par("responsePacketSize"));

    processingTimer_ = new cMessage("computeMsg");

    minInstructions_ = par("minInstructions");
    maxInstructions_ = par("maxInstructions");

    // connect with the service registry
    EV << "MecResponseApp::initialize - Initialize connection with the Service Registry via Mp1" << endl;
    mp1Socket_ = addNewSocket();

    connect(mp1Socket_, mp1Address, mp1Port);
}

void MecResponseApp::handleProcessedMessage(cMessage *msg)
{
    if (!msg->isSelfMessage()) {
        if (ueAppSocket_.belongsToSocket(msg)) {
            EV << "MecResponseApp::handleProcessedMessage: received message from UE" << endl;
            inet::Packet *packet = check_and_cast<inet::Packet *>(msg);
            auto req = packet->peekAtFront<RequestResponseAppPacket>();
            if (req->getType() == UEAPP_REQUEST)
                handleRequest(msg);
            else if (req->getType() == UEAPP_STOP)
                handleStopRequest(msg);
            else
                throw cRuntimeError("MecResponseApp::handleProcessedMessage - Type not recognized!");
            return;
        }
    }
    MecAppBase::handleProcessedMessage(msg);
}

void MecResponseApp::finish()
{
    MecAppBase::finish();
    EV << "MecResponseApp::finish()" << endl;
    if (gate("socketOut")->isConnected()) {
    }
}

double MecResponseApp::scheduleNextMsg(cMessage *msg)
{
    return MecAppBase::scheduleNextMsg(msg);
}

void MecResponseApp::handleSelfMessage(cMessage *msg)
{
    if (!strcmp(msg->getName(), "computeMsg")) {
        sendResponse();
    }
}

void MecResponseApp::handleRequest(cMessage *msg)
{
    EV << "MecResponseApp::handleRequest" << endl;
    // this method pretends to perform some computation after having
    //.request some info to the RNI
    if (currentRequestfMsg_ != nullptr)
        throw cRuntimeError("MecResponseApp::handleRequest - currentRequestfMsg_ not null!");

    msgArrived_ = simTime();
    currentRequestfMsg_ = msg;
    sendGetRequest();
    getRequestSent_ = simTime();
}

void MecResponseApp::handleStopRequest(cMessage *msg)
{
    EV << "MecResponseApp::handleStopRequest" << endl;
    serviceSocket_->close();
}

void MecResponseApp::sendResponse()
{
    inet::Packet *packet = check_and_cast<inet::Packet *>(currentRequestfMsg_);
    ueAppAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
    ueAppPort = packet->getTag<L4PortInd>()->getSrcPort();

    auto req = packet->removeAtFront<RequestResponseAppPacket>();
    req->setType(MECAPP_RESPONSE);
    req->setRequestArrivedTimestamp(msgArrived_);
    req->setServiceResponseTime(getRequestArrived_ - getRequestSent_);
    req->setResponseSentTimestamp(simTime());
    req->setProcessingTime(processingTime_);
    req->setChunkLength(packetSize_);
    inet::Packet *pkt = new inet::Packet("ResponseAppPacket");
    pkt->insertAtBack(req);

    ueAppSocket_.sendTo(pkt, ueAppAddress, ueAppPort);

    //clean current request
    delete packet;
    currentRequestfMsg_ = nullptr;
    msgArrived_ = 0;
    processingTime_ = 0;
    getRequestArrived_ = 0;
    getRequestSent_ = 0;
}

void MecResponseApp::handleHttpMessage(int connId)
{
    if (mp1Socket_ != nullptr && connId == mp1Socket_->getSocketId()) {
        handleMp1Message(connId);
    }
    else {
        handleServiceMessage(connId);
    }
}

void MecResponseApp::handleMp1Message(int connId)
{
    // for now I only have just one Service Registry
    HttpMessageStatus *msgStatus = static_cast<HttpMessageStatus *>(mp1Socket_->getUserData());
    mp1HttpMessage = check_and_cast_nullable<HttpBaseMessage *>(msgStatus->httpMessageQueue.front());
    EV << "MecPlatooningApp::handleMp1Message - payload: " << mp1HttpMessage->getBody() << endl;

    try {
        nlohmann::json jsonBody = nlohmann::json::parse(mp1HttpMessage->getBody()); // get the JSON structure
        if (!jsonBody.empty()) {
            jsonBody = jsonBody[0];
            std::string serName = jsonBody["serName"];
            if (serName == "RNIService") {
                if (jsonBody.contains("transportInfo")) {
                    nlohmann::json endPoint = jsonBody["transportInfo"]["endPoint"]["addresses"];
                    EV << "address: " << endPoint["host"] << " port: " << endPoint["port"] << endl;
                    std::string address = endPoint["host"];
                    serviceAddress_ = L3AddressResolver().resolve(address.c_str());
                    servicePort_ = endPoint["port"];
                    serviceSocket_ = addNewSocket();
                    connect(serviceSocket_, serviceAddress_, servicePort_);
                }
            }
            else {
                EV << "MecPlatooningApp::handleMp1Message - RNIService not found" << endl;
                serviceAddress_ = L3Address();
            }
        }

        //close service registry socket
        mp1Socket_->close();
    }
    catch (nlohmann::detail::parse_error e) {
        EV << e.what() << std::endl;
        // body is not correctly formatted in JSON, manage it
        return;
    }
}

void MecResponseApp::handleServiceMessage(int connId)
{
    HttpMessageStatus *msgStatus = static_cast<HttpMessageStatus *>(serviceSocket_->getUserData());
    HttpBaseMessage *httpMessage = check_and_cast<HttpBaseMessage *>(msgStatus->httpMessageQueue.front());

    if (httpMessage->getType() == RESPONSE) {
        HttpResponseMessage *rspMsg = dynamic_cast<HttpResponseMessage *>(httpMessage);
        if (rspMsg->getCode() == 200) { // in response to a successful GET request
            EV << "MecResponseApp::handleServiceMessage - response 200 from Socket with Id [" << connId << "]" << endl;
            getRequestArrived_ = simTime();
            EV << "response time " << getRequestArrived_ - getRequestSent_ << endl;
            doComputation();
        }
        // some error occured, show the HTTP code for now
        else {
            EV << "MecResponseApp::handleServiceMessage - response with HTTP code:  " << rspMsg->getCode() << endl;
        }
    }
}

void MecResponseApp::doComputation()
{
    processingTime_ = vim->calculateProcessingTime(mecAppId, uniform(minInstructions_, maxInstructions_));
    EV << "time " << processingTime_ << endl;
    scheduleAt(simTime() + processingTime_, processingTimer_);
}

void MecResponseApp::sendGetRequest()
{
    //check if the ueAppAddress is specified
    if (serviceSocket_->getState() == inet::TcpSocket::CONNECTED) {
        EV << "MecResponseApp::sendGetRequest(): send request to the Location Service" << endl;
        std::stringstream uri;
        uri << "/example/rni/v2/queries/layer2_meas"; //TODO filter the request to get less data
        EV << "MecResponseApp::requestLocation(): uri: " << uri.str() << endl;
        std::string host = serviceSocket_->getRemoteAddress().str() + ":" + std::to_string(serviceSocket_->getRemotePort());
        Http::sendGetRequest(serviceSocket_, host.c_str(), uri.str().c_str());
    }
    else {
        EV << "MecResponseApp::sendGetRequest(): Location Service not connected" << endl;
    }
}

void MecResponseApp::established(int connId)
{
    EV << "MecResponseApp::established - connId [" << connId << "]" << endl;

    if (mp1Socket_ != nullptr && connId == mp1Socket_->getSocketId()) {
        EV << "MecResponseApp::established - Mp1Socket" << endl;

        // once the connection with the Service Registry has been established, obtain the
        // endPoint (address+port) of the Location Service
        const char *uri = "/example/mec_service_mgmt/v1/services?ser_name=RNIService";
        std::string host = mp1Socket_->getRemoteAddress().str() + ":" + std::to_string(mp1Socket_->getRemotePort());

        Http::sendGetRequest(mp1Socket_, host.c_str(), uri);
    }
}

void MecResponseApp::socketClosed(inet::TcpSocket *sock)
{
    EV << "MecResponseApp::socketClosed" << endl;
    std::cout << "MecResponseApp::socketClosed with sockId " << sock->getSocketId() << std::endl;
    if (mp1Socket_ != nullptr && sock->getSocketId() == mp1Socket_->getSocketId()) {
        removeSocket(sock);
        mp1Socket_ = nullptr;
    }
    else {
        EV << "Service socket closed" << endl;
        removeSocket(sock);
        sendStopAck();
    }
}

void MecResponseApp::sendStopAck()
{
    inet::Packet *pkt = new inet::Packet("RequestResponseAppPacket");
    auto req = inet::makeShared<RequestResponseAppPacket>();
    req->setType(UEAPP_ACK_STOP);
    req->setChunkLength(packetSize_);
    pkt->insertAtBack(req);

    ueAppSocket_.sendTo(pkt, ueAppAddress, ueAppPort);
}

} //namespace

