//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "apps/mec/MecApps/MecRequestBackgroundGeneratorApp/MecRequestBackgroundGeneratorApp.h"

#include <string>

#include <inet/common/TimeTag_m.h>
#include <inet/common/packet/chunk/BytesChunk.h>

#include "nodes/mec/MECPlatform/MECServices/packets/HttpRequestMessage/HttpRequestMessage.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"

namespace simu5g {

using namespace inet;

Define_Module(MecRequestBackgroundGeneratorApp);

MecRequestBackgroundGeneratorApp::~MecRequestBackgroundGeneratorApp()
{
    cancelAndDelete(sendBurst);
    cancelAndDelete(burstPeriod);
}

void MecRequestBackgroundGeneratorApp::handleServiceMessage(int connId)
{
    HttpMessageStatus *msgStatus = static_cast<HttpMessageStatus *>(serviceSocket_->getUserData());
    serviceHttpMessage = check_and_cast<HttpBaseMessage *>(msgStatus->httpMessageQueue.front());
    EV << "payload: " << serviceHttpMessage->getBody() << endl;
    if (burstFlag)
        scheduleAt(simTime() + 0, sendBurst);
}

void MecRequestBackgroundGeneratorApp::established(int connId)
{
    if (connId == mp1Socket_->getSocketId()) {
        EV << "MecRequestBackgroundGeneratorApp::established - Mp1Socket" << endl;
        // get endPoint of the required service
        const char *uri = "/example/mec_service_mgmt/v1/services?ser_name=LocationService";
        std::string host = mp1Socket_->getRemoteAddress().str() + ":" + std::to_string(mp1Socket_->getRemotePort());

        Http::sendGetRequest(mp1Socket_, host.c_str(), uri);
        return;
    }
    else if (connId == serviceSocket_->getSocketId()) {
        EV << "MecRequestBackgroundGeneratorApp::established - serviceSocket" << endl;
        scheduleAt(simTime() + 5, burstTimer);
    }
    else {
        throw cRuntimeError("MecRequestBackgroundGeneratorApp::socketEstablished - Socket %d not recognized", connId);
    }
}

void MecRequestBackgroundGeneratorApp::handleSelfMessage(cMessage *msg) {
    if (strcmp(msg->getName(), "burstTimer") == 0) {
        burstFlag = !burstFlag;
        if (burstFlag) {
            sendBulkRequest();
            scheduleAt(simTime() + 0.5, burstPeriod);
        }
        scheduleAt(simTime() + 5, burstTimer);
    }
    else if (strcmp(msg->getName(), "burstPeriod") == 0) {
        burstFlag = false;
    }
    else if (strcmp(msg->getName(), "sendBurst") == 0) {
        sendBulkRequest();
    }
    else if (strcmp(msg->getName(), "connectMp1") == 0) {
        EV << "MecAppBase::handleMessage- " << msg->getName() << endl;
        connect(mp1Socket_, mp1Address, mp1Port);
        delete msg;
    }
    else if (strcmp(msg->getName(), "connectService") == 0) {
        EV << "MecAppBase::handleMessage- " << msg->getName() << endl;
        connect(serviceSocket_, serviceAddress, servicePort);
    }
    else {
        EV << "MecRequestBackgroundGeneratorApp::handleSelfMessage - selfMessage not recognized" << endl;
        delete msg;
    }
}

void MecRequestBackgroundGeneratorApp::handleHttpMessage(int connId)
{
    if (mp1Socket_ != nullptr && connId == mp1Socket_->getSocketId()) {
        handleMp1Message(connId);
    }
    else {
        handleServiceMessage(connId);
    }
}

void MecRequestBackgroundGeneratorApp::handleMp1Message(int connId)
{
    HttpMessageStatus *msgStatus = static_cast<HttpMessageStatus *>(mp1Socket_->getUserData());
    mp1HttpMessage = check_and_cast<HttpBaseMessage *>(msgStatus->httpMessageQueue.front());
    EV << "MEWarningAlertApp_rest::handleMp1Message - payload: " << mp1HttpMessage->getBody() << endl;

    try {
        nlohmann::json jsonBody = nlohmann::json::parse(mp1HttpMessage->getBody()); // get the JSON structure
        if (!jsonBody.empty()) {
            jsonBody = jsonBody[0];
            std::string serName = jsonBody["serName"];
            if (serName == "LocationService") {
                if (jsonBody.contains("transportInfo")) {
                    nlohmann::json endPoint = jsonBody["transportInfo"]["endPoint"]["addresses"];
                    EV << "address: " << endPoint["host"] << " port: " << endPoint["port"] << endl;
                    std::string address = endPoint["host"];
                    serviceAddress = L3AddressResolver().resolve(address.c_str());
                    servicePort = endPoint["port"];
                    serviceSocket_ = addNewSocket();
                    connect(serviceSocket_, serviceAddress, servicePort);
                }
            }
        }
    }
    catch (nlohmann::detail::parse_error e) {
        EV << e.what() << std::endl;
        // body is not correctly formatted in JSON, manage it
        return;
    }
}

void MecRequestBackgroundGeneratorApp::initialize(int stage) {
    MecAppBase::initialize(stage);

    if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        EV << "MecRequestBackgroundGeneratorApp::initialize" << endl;
        numberOfApplications_ = par("numberOfApplications");

        if (numberOfApplications_ != 0) {
            if (vim == nullptr)
                throw cRuntimeError("MecRequestBackgroundGeneratorApp::initialize - vim is null!");
            if (par("allocateHostResources").boolValue()) {
                bool res = vim->registerMecApp(getId(), requiredRam, requiredDisk, requiredCpu);
                if (!res) {
                    EV << "MecRequestBackgroundGeneratorApp::initialize - MecRequestBackgroundGeneratorApp [" << mecAppId << "]  cannot be instantiated" << endl;
                }
            }
            else {
                EV << "MecRequestBackgroundGeneratorApp::initialize - MecRequestBackgroundGeneratorApp [" << mecAppId << "] does not allocate MEC host resources" << endl;
            }

            mp1Socket_ = addNewSocket();
            cMessage *m = new cMessage("connectMp1");
            sendBurst = new cMessage("sendBurst");
            burstPeriod = new cMessage("burstPeriod");
            burstTimer = new cMessage("burstTimer");
            burstFlag = false;
            mecAppId = getId();
            scheduleAt(simTime() + 0.055, m);
        }
    }
}

void MecRequestBackgroundGeneratorApp::sendBulkRequest() {
    int numRequests = truncnormal(numberOfApplications_, 20, 2);
    std::string payload = "BulkRequest: " + std::to_string(numRequests + 1);

    Http::sendPacket(payload.c_str(), serviceSocket_);

    EV << "MecRequestBackgroundGeneratorApp: " << numRequests << " requests to the server" << endl;
}

void MecRequestBackgroundGeneratorApp::finish()
{
    if (vim == nullptr)
        throw cRuntimeError("MecRequestBackgroundGeneratorApp::initialize - vim is null!");
    if (par("allocateHostResources").boolValue()) {
        bool res = vim->deRegisterMecApp(mecAppId);
        if (!res) {
            EV << "MecRequestBackgroundGeneratorApp::finish - MEC host did not deallocate MecRequestBackgroundGeneratorApp [" << mecAppId << "] resources" << endl;
        }
    }
    else {
        EV << "MecRequestBackgroundGeneratorApp::finish - MecRequestBackgroundGeneratorApp [" << mecAppId << "] had not allocated resources" << endl;
    }
}

} //namespace

