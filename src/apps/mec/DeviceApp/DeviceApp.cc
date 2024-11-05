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

#include "apps/mec/DeviceApp/DeviceApp.h"

#include <string>

#include <inet/common/TimeTag_m.h>
#include <inet/common/packet/chunk/BytesChunk.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/networklayer/common/L3AddressTag_m.h>
#include <inet/transportlayer/common/L4PortTag_m.h>

#include "DeviceAppMessages/DeviceAppPacket_Types.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpRequestMessage/HttpRequestMessage.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/utils/httpUtils/json.hpp"

namespace simu5g {

using namespace inet;
using namespace omnetpp;
Define_Module(DeviceApp);


DeviceApp::~DeviceApp()
{
    cancelAndDelete(processedUALCMPMessage);
}

void DeviceApp::handleUALCMPMessage()
{
    EV << "DeviceApp::handleUALCMPMessage: " << UALCMPMessage->getBody() << endl;

    if (UALCMPMessage->getType() == RESPONSE) {
        HttpResponseMessage *response = dynamic_cast<HttpResponseMessage *>(UALCMPMessage);

        switch (appState) {
            case START: {
                EV << "DeviceApp::handleUALCMPMessage - START" << endl;
                if (response->getCode() == 200) { // Successful response of the get
                    nlohmann::json jsonResponseBody = nlohmann::json::parse(response->getBody());
                    nlohmann::json jsonRequestBody;

                    bool found = false;
                    int size = jsonResponseBody["appList"].size();
                    for (int i = 0; i < size; ++i) {
                        nlohmann::json appInfo = jsonResponseBody["appList"];
                        if (appName == appInfo.at(i)["appName"]) {
                            // search fo the app name in the list of shared apps. if found, use the stored dev app id
                            auto sharedDevAppId = devAppIds.find(std::string(appName));
                            int associateDevAppId;
                            if (sharedDevAppId != devAppIds.end())
                                associateDevAppId = sharedDevAppId->second;
                            else
                                associateDevAppId = getId();
                            jsonRequestBody["associateDevAppId"] = std::to_string(associateDevAppId);
                            jsonRequestBody["appInfo"]["appDId"] = appInfo.at(i)["appDId"];// "WAMECAPP_External"; //startPk->getMecAppDId()
                            jsonRequestBody["appInfo"]["appName"] = appName;//"MEWarningAlertApp_rest";
                            jsonRequestBody["appInfo"]["appProvider"] = appInfo.at(i)["appProvider"];//startPk->getMecAppProvider();//"lte.apps.mec.warningAlert_rest.MEWarningAlertApp_rest";
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        EV << "DeviceApp::handleUALCMPMessage: application descriptor for appName: " << appName << " not found." << endl;
                        // search fo the app name in the list of shared apps. if found, use the stored dev app id
                        auto sharedDevAppId = devAppIds.find(std::string(appName));
                        int associateDevAppId;
                        if (sharedDevAppId != devAppIds.end())
                            associateDevAppId = sharedDevAppId->second;
                        else
                            associateDevAppId = getId();
                        jsonRequestBody["associateDevAppId"] = std::to_string(associateDevAppId);
                        jsonRequestBody["appInfo"]["appPackageSource"] = appPackageSource; //"ApplicationDescriptors/WarningAlertApp.json";

                        jsonRequestBody["appInfo"]["appName"] = appName;//"MEWarningAlertApp_rest";
                        // do not add the appProvider field (even if it is mandatory in ETSI specs. The MEC orchestrator we implemented does not use it
                        // jsonRequestBody["appInfo"]["appProvider"] = appProvider;//startPk->getMecAppProvider();//"lte.apps.mec.warningAlert_rest.MEWarningAlertApp_rest";
                    }

                    const char *uri = "/example/dev_app/v1/app_contexts";

                    std::string host = UALCMPSocket_.getRemoteAddress().str() + ":" + std::to_string(UALCMPSocket_.getRemotePort());

                    Http::sendPostRequest(&UALCMPSocket_, jsonRequestBody.dump().c_str(), host.c_str(), uri);

                    //send request
                    appState = CREATING;
                    return;
                }
                else {
                    // this should not happen. This device app always sends well formed requests to the UALCMP
                    throw cRuntimeError("DeviceApp::handleUALCMPMessage() - this should not happen. This device app sends well formed requests to the UALCMP.");
                }
                break;
            }

            case CREATING: {
                EV << "DeviceApp::handleUALCMPMessage - CREATING" << endl;
                if (response->getCode() == 201) { // Successful response of the post
                    nlohmann::json jsonBody = nlohmann::json::parse(UALCMPMessage->getBody());

                    inet::Packet *packet = new inet::Packet("DeviceAppStartAckPacket");

                    std::string contextUri = response->getHeaderField("Location");

                    if (contextUri.empty()) {
                        //ERROR
                        EV << "DeviceApp::handleUALCMPMessage - ERROR (on CREATE 201) - Mec Application Context not created, i.e. the MEC app has not been instantiated" << endl;

                        auto nack = inet::makeShared<DeviceAppStartAckPacket>();

                        //instantiation requirements and info
                        nack->setType(ACK_START_MECAPP);

                        //connection info
                        nack->setResult(false);
                        // TODO add reason?
                        throw cRuntimeError("201 empty");
                        nack->setChunkLength(inet::B(2)); //just code and data length = 0
                        nack->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

                        packet->insertAtBack(nack);
                    }
                    else {
                        appContextUri = contextUri;
                        mecAppEndPoint = jsonBody["appInfo"]["userAppInstanceInfo"]["referenceURI"];

                        EV << "DeviceApp::handleUALCMPMessage - reference URI of the application instance context is: " << appContextUri << endl;
                        EV << "DeviceApp::handleUALCMPMessage - endPOint of the mec application instance is: " << mecAppEndPoint << endl;

                        std::vector<std::string> endPoint = cStringTokenizer(mecAppEndPoint.c_str(), ":").asVector();
                        EV << "vectore size: " << endPoint[0] << " e " << atoi(endPoint[1].c_str()) << endl;

                        auto ack = inet::makeShared<DeviceAppStartAckPacket>();

                        //instantiation requirements and info
                        ack->setType(ACK_START_MECAPP);

                        //connection info
                        std::string contextId = jsonBody["contextId"];
                        ack->setContextId(contextId.c_str());
                        ack->setResult(true);
                        ack->setIpAddress(endPoint[0].c_str());
                        ack->setPort(atoi(endPoint[1].c_str()));

                        ack->setChunkLength(inet::B(2 + mecAppEndPoint.size() + contextId.size() + 1));
                        ack->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

                        packet->insertAtBack(ack);
                    }

                    ueAppSocket_.sendTo(packet, ueAppAddress, ueAppPort);

                    appState = APPCREATED;
                    return;
                }
                else if (response->getCode() == 500) {
                    //ERROR
                    EV << "DeviceApp::handleUALCMPMessage - ERROR (on CREATE " << response->getCode() << ") - Mec Application Context not created, i.e. the MEC app has not been instantiated" << endl;
                    auto nack = inet::makeShared<DeviceAppStartAckPacket>();

                    //instantiation requirements and info
                    nack->setType(ACK_START_MECAPP);

                    //connection info
                    nack->setResult(false);
                    nack->setReason(response->getPayload().c_str());
                    if (strlen(nack->getReason())) {
                        nack->setChunkLength(inet::B(2 + strlen(nack->getReason()))); //just code and data length = 0
                    }
                    else {
                        nack->setChunkLength(inet::B(2)); //just code and data length = 0
                    }
                    nack->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

                    inet::Packet *packet = new inet::Packet("DeviceAppStartAckPacket");
                    packet->insertAtBack(nack);
                    //throw cRuntimeError("LCM proxy responded 500");
                    ueAppSocket_.sendTo(packet, ueAppAddress, ueAppPort);

                    appState = IDLE;
                    return;
                }
                else {
                    // in state create only 201 and 500 code are allowed, if other code arrives, something went wrong...
                    EV << "DeviceApp::handleUALCMPMessage - HTTP code " << response->getCode() << " not allowe in CREATE state" << endl;
                }
                break;
            }

            case DELETING: {
                inet::Packet *packet = new inet::Packet("DeviceAppStopAckPacket");
                auto ack = inet::makeShared<DeviceAppStopAckPacket>();
                //instantiation requirements and info
                ack->setType(ACK_STOP_MECAPP);

                if (response->getCode() == 204) { // Successful response of the delete
                    ack->setResult(true);
                    ack->setChunkLength(inet::B(2));
                    ack->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
                    packet->insertAtBack(ack);
                    appState = IDLE;
                }
                else if (response->getCode() == 404) {
                    ack->setResult(false);
                    ack->setReason("ContextId not found, maybe it has been already deleted");
                    if (strlen(ack->getReason())) {
                        ack->setChunkLength(inet::B(2 + strlen(ack->getReason()))); //just code and data length = 0
                    }
                    else {
                        ack->setChunkLength(inet::B(2)); //just code and data length = 0
                    }
                    ack->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
                    packet->insertAtBack(ack);
                    appState = IDLE;
                }
                else if (response->getCode() == 500) {
                    ack->setResult(false);
                    ack->setReason("MEC app termination did not success");
                    if (strlen(ack->getReason())) {
                        ack->setChunkLength(inet::B(2 + strlen(ack->getReason()))); //just code and data length = 0
                    }
                    else {
                        ack->setChunkLength(inet::B(2)); //just code and data length = 0
                    }
                    ack->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
                    packet->insertAtBack(ack);

                    appState = APPCREATED;
                    return;
                }

                // send nack/ack
                ueAppSocket_.sendTo(packet, ueAppAddress, ueAppPort);

                break;
            }

            case IDLE:
            default:
                throw cRuntimeError("DeviceApp::handleUALCMPMessage() - appstate IDLE. No messages should arrive from the UALCMP");
        }
    }
    else {
        // TODO implement subscriptions
        return;
    }
}

void DeviceApp::handleSelfMessage(cMessage *msg) {
    if (strcmp(msg->getName(), "connect") == 0) {
        connectToUALCMP();
        delete msg;
    }
    else if (strcmp(msg->getName(), "processedUALCMPMessage") == 0) {
        handleUALCMPMessage();
        if (UALCMPMessage != nullptr)
            delete UALCMPMessage;
        UALCMPMessage = nullptr;
    }
}

void DeviceApp::initialize(int stage) {

    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;

    const char *localAddressStr = par("localAddress");
    L3Address localAddress = *localAddressStr ? L3AddressResolver().resolve(localAddressStr) : L3Address();

    localPort = par("localPort");

    ueAppSocket_.setOutputGate(gate("socketOut"));
    UALCMPSocket_.setOutputGate(gate("socketOut"));

    ueAppSocket_.bind(localAddress, localPort); // bind ueSocket to listen on local port
    UALCMPSocket_.bind(localAddress, par("ualcmpLocalPort")); // bind UALCMP Socket to listen on ualcmpLocalPort

    ueAppSocket_.setCallback(this);
    UALCMPSocket_.setCallback(this);

    int timeToLive = par("timeToLive"); // TODO split to 2 parameters, when need different value for ue and UALCMP socket
    if (timeToLive != -1) {
        ueAppSocket_.setTimeToLive(timeToLive);
        UALCMPSocket_.setTimeToLive(timeToLive);
    }

    int dscp = par("dscp"); // TODO split to 2 parameters, when need different value for ue and UALCMP socket
    if (dscp != -1) {
        ueAppSocket_.setDscp(dscp);
        UALCMPSocket_.setDscp(dscp);
    }

    int tos = par("tos"); // TODO split to 2 parameters, when need different value for ue and UALCMP socket
    if (tos != -1) {
        ueAppSocket_.setTos(tos);
        UALCMPSocket_.setTos(tos);
    }

    const char *lcmAddress = par("UALCMPAddress").stringValue();
    UALCMPAddress = L3AddressResolver().resolve(lcmAddress);
    EV << "DeviceApp::initialize - UALCMPAddress: " << UALCMPAddress.str() << endl;
    UALCMPPort = par("UALCMPPort");

    processedUALCMPMessage = new cMessage("processedUALCMPMessage");

    appPackageSource = par("appPackageSource").stringValue();

    appState = IDLE;

    /* directly connect to the LCM proxy
     * instead of waiting for a requeste from the UE, it is
     * easier to manage
     */

    cMessage *msg = new cMessage("connect");
    scheduleAt(simTime() + 0.0, msg);
}

void DeviceApp::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        handleSelfMessage(msg);
        return;
    }
    else if (ueAppSocket_.belongsToSocket(msg)) {
        ueAppSocket_.processMessage(msg);
        delete msg;
    }
    else if (UALCMPSocket_.belongsToSocket(msg)) {
        UALCMPSocket_.processMessage(msg);
    }
}

void DeviceApp::connectToUALCMP()
{
    // we need a new connId if this is not the first connection
    UALCMPSocket_.renewSocket();

    if (UALCMPAddress.isUnspecified()) {
        EV_ERROR << "Connecting to " << UALCMPAddress << " port=" << UALCMPPort << ": cannot resolve destination address\n";
        throw cRuntimeError("LCM proxy address is unspecified!");
    }
    else {
        EV << "Connecting to " << UALCMPAddress << " port=" << UALCMPPort << endl;
        UALCMPSocket_.connect(UALCMPAddress, UALCMPPort);
    }
}

void DeviceApp::sendStartAppContext(inet::Ptr<const DeviceAppPacket> pk)
{
    EV << "DeviceApp::sendStartAppContext" << endl;

    auto startPk = dynamicPtrCast<const DeviceAppStartPacket>(pk);
    if (startPk == nullptr)
        throw cRuntimeError("DeviceApp::sendStartAppContext - DeviceAppStartPacket is null");

    /*
     * ask the proxy for the requested app, by appName
     */
    appName = startPk->getMecAppName();
    if (startPk->getShared()) {
        // if this is a shared app, store the dev app id to be associated when sending the start request
        devAppIds[std::string(appName)] = startPk->getAssociateDevAppId();
    }

    std::string uri("/example/dev_app/v1/app_list");
    std::stringstream params;
    params << "appName=" << appName;

    std::string host = UALCMPSocket_.getRemoteAddress().str() + ":" + std::to_string(UALCMPSocket_.getRemotePort());

    if (UALCMPSocket_.getState() == inet::TcpSocket::CONNECTED && appState == IDLE) {
        Http::sendGetRequest(&UALCMPSocket_, host.c_str(), uri.c_str(), params.str().c_str());
        appState = START;
    }
    else if (UALCMPSocket_.getState() != inet::TcpSocket::CONNECTED) {
        // send nack to the UE app

        inet::Packet *packet = new inet::Packet("DeviceAppStartAckPacket");
        auto nack = inet::makeShared<DeviceAppStartAckPacket>();

        //instantiation requirements and info
        nack->setType(ACK_START_MECAPP);

        //connection info
        nack->setResult(false);

        nack->setReason("LCM proxy not connected");
        if (strlen(nack->getReason())) {
            nack->setChunkLength(inet::B(2 + strlen(nack->getReason()))); //just code and data length = 0
        }
        else {
            nack->setChunkLength(inet::B(2)); //just code and data length = 0
        }
        nack->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
        packet->insertAtBack(nack);
        ueAppSocket_.sendTo(packet, ueAppAddress, ueAppPort);

        return;
    }
    else if (appState != IDLE) {
        EV << "DeviceApp::sendStartAppContext already sent" << endl;
    }
}

void DeviceApp::sendStopAppContext(inet::Ptr<const DeviceAppPacket> pk)
{
    auto stopPk = dynamicPtrCast<const DeviceAppStopPacket>(pk);
    if (stopPk == nullptr)
        throw cRuntimeError("DeviceApp::sendStopAppContext - DeviceAppStopPacket is null");

    /*
     * Not used, since the UE app is 1-1 with the device app, so the latter uses the
     *  appContextUri saved during startContext
     */
    std::string cId = stopPk->getContextId();

    std::string host = UALCMPSocket_.getRemoteAddress().str() + ":" + std::to_string(UALCMPSocket_.getRemotePort());

    if (UALCMPSocket_.getState() == inet::TcpSocket::CONNECTED && appState == APPCREATED) {
        EV << "DeviceApp::sendStopAppContext - send DELETE for MEC app: " << appContextUri << endl;
        Http::sendDeleteRequest(&UALCMPSocket_, host.c_str(), appContextUri.c_str());
        appState = DELETING;
    }
    else if (appState == DELETING) {
        EV << "DeviceApp::sendStopAppContext - DELETE command already sent - discarding packet" << endl;
        return;
    }
    else if (UALCMPSocket_.getState() != inet::TcpSocket::CONNECTED) {
        EV << "DeviceApp::sendStopAppContext - LCM proxy not connected" << endl;

        inet::Packet *packet = new inet::Packet("DeviceAppStopAckPacket");
        auto ack = inet::makeShared<DeviceAppStopAckPacket>();

        //instantiation requirements and info
        ack->setType(ACK_STOP_MECAPP);

        ack->setResult(false);
        ack->setReason("LCM proxy not connected");

        if (strlen(ack->getReason())) {
            ack->setChunkLength(inet::B(2 + strlen(ack->getReason()))); //just code and data length = 0
        }
        else {
            ack->setChunkLength(inet::B(2)); //just code and data length = 0
        }
        ack->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
        packet->insertAtBack(ack);

        ueAppSocket_.sendTo(packet, ueAppAddress, ueAppPort);
        return;
    }
}

// ------ UDP socket Callback implementation

void DeviceApp::socketDataArrived(UdpSocket *socket, Packet *pk)
{
    // determine its source address/port
    ueAppAddress = pk->getTag<L3AddressInd>()->getSrcAddress();
    ueAppPort = pk->getTag<L4PortInd>()->getSrcPort();

    auto pkt = pk->peekAtFront<DeviceAppPacket>();
    EV << "DeviceAppPacket type: " << pkt->getType() << endl;
    if (strcmp(pkt->getType(), START_MECAPP) == 0) {
        sendStartAppContext(pkt);
    }
    else if (strcmp(pkt->getType(), STOP_MECAPP) == 0) {
        sendStopAppContext(pkt);
    }
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
    std::vector<uint8_t> bytes = msg->peekDataAsBytes()->getBytes();
    std::string packet(bytes.begin(), bytes.end());

    delete msg;

    bool res = Http::parseReceivedMsg(packet, UALCMPMessageBuffer, UALCMPMessage);
    if (res) {
        EV << "DeviceApp::socketDataArrived - schedule processedUALCMPMessage" << endl;
        UALCMPMessage->setSockId(UALCMPSocket_.getSocketId());
        double time = 0.005;
        scheduleAt(simTime() + time, processedUALCMPMessage);
    }
}

void DeviceApp::socketEstablished(inet::TcpSocket *socket)
{
    EV << "DeviceApp::socketEstablished - UALCMPSocket established " << endl;
}

void DeviceApp::socketPeerClosed(inet::TcpSocket *socket)
{
    EV << "DeviceApp::socketPeerClosed" << endl;
    if (UALCMPSocket_.getState() == TcpSocket::PEER_CLOSED) {
        EV_INFO << "remote TCP closed, closing here as well\n";
        UALCMPSocket_.close();
    }
}

void DeviceApp::socketClosed(inet::TcpSocket *socket) {}
void DeviceApp::socketFailure(inet::TcpSocket *socket, int code) {}

void DeviceApp::finish()
{
    if (UALCMPSocket_.getState() == inet::TcpSocket::CONNECTED)
        UALCMPSocket_.close();
}

} //namespace

