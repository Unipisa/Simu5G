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

#include "apps/mec/PlatooningApp/MECPlatooningConsumerApp.h"
#include "apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket_Types.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/utils/httpUtils/json.hpp"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.h"

#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/Packet_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"

#include <fstream>

Define_Module(MECPlatooningConsumerApp);

using namespace inet;
using namespace omnetpp;

MECPlatooningConsumerApp::MECPlatooningConsumerApp(): MecAppBase()
{
    lastXposition = 0.;
    lastYposition = 0.;
    lastZposition = 0.;

    state_ = IDLE;
}

MECPlatooningConsumerApp::~MECPlatooningConsumerApp()
{
}


void MECPlatooningConsumerApp::initialize(int stage)
{
    MecAppBase::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    // set UE UDP Socket
    localUePort = par("localUePort");
    ueAppSocket.setOutputGate(gate("socketOut"));
    ueAppSocket.bind(localUePort);


    // TODO add a new socket to handle communication with the provider app
    localPlatooningProviderPort = par("localPlatooningProviderPort");
    platooningProviderAppSocket.setOutputGate(gate("socketOut"));
    platooningProviderAppSocket.bind(localPlatooningProviderPort+this->getIndex()); // base+index vector;

    localPlatooningControllerPort = par("localPlatooningControllerPort");
    platooningControllerAppSocket.setOutputGate(gate("socketOut"));
    platooningControllerAppSocket.bind(localPlatooningControllerPort+this->getIndex()); // base+index vector;

    EV << "MECPlatooningConsumerApp::initialize - Mec application "<< getClassName() << " with mecAppId["<< mecAppId << "] has started!" << endl;

    // get platooning provider info
    platooningProviderAddress_ = L3Address(par("platooningProviderAddress").stringValue());
    platooningProviderPort_ = par("platooningProviderPort");

    // send registration request to the provider app
//    registerToPlatooningProviderApp();

    // initially, not connected to any controller
    controllerId_ = -1;

    // connect with the service registry
//    EV << "MECPlatooningConsumerApp::initialize - Initialize connection with the service registry via Mp1" << endl;
//    connect(&mp1Socket_, mp1Address, mp1Port);
}

void MECPlatooningConsumerApp::handleMessage(cMessage *msg)
{
    if (!msg->isSelfMessage())
    {
        auto pk = check_and_cast<Packet *>(msg);
        auto platooningPkt = pk->peekAtFront<PlatooningAppPacket>();

        if(ueAppSocket.belongsToSocket(msg))
        {
            handleUeMessage(msg);
            return;
        }
        else if (platooningProviderAppSocket.belongsToSocket(msg))
        {
            if (platooningPkt->getType() == DISCOVER_ASSOCIATE_PLATOON_RESPONSE)  // from mec provider app
                handleDiscoverAndAssociatePlatoonResponse(msg);
            else if (platooningPkt->getType() == DISCOVER_PLATOONS_RESPONSE)  // from mec provider app
                handleDiscoverPlatoonResponse(msg);
            else if (platooningPkt->getType() == ASSOCIATE_PLATOON_RESPONSE)  // from mec provider app
                handleAssociatePlatoonResponse(msg);
            return;
        }
        else if (platooningControllerAppSocket.belongsToSocket(msg))
        {
            if (platooningPkt->getType() == JOIN_RESPONSE)  // from mec provider app
                handleJoinPlatoonResponse(msg);

            else if (platooningPkt->getType() == LEAVE_RESPONSE) // from mec provider app
                handleLeavePlatoonResponse(msg);

            else if (platooningPkt->getType() == PLATOON_CMD)  // from mec provider app
                handlePlatoonCommand(msg);
            else if (platooningPkt->getType() == MANOEUVRE_NOTIFICATION)  // from mec provider app
                handleManoeuvreNotification(msg);
            else if (platooningPkt->getType() == QUEUED_JOIN_NOTIFICATION)  // from mec provider app
                handleQueuedJoinNotification(msg);

            return;
        }
    }
    MecAppBase::handleMessage(msg);
}

void MECPlatooningConsumerApp::finish()
{
    MecAppBase::finish();
    EV << "MECPlatooningConsumerApp::finish()" << endl;
    if(gate("socketOut")->isConnected())
    {

    }
}

void MECPlatooningConsumerApp::handleSelfMessage(cMessage *msg)
{
    // nothing to do here
    delete msg;
}

void MECPlatooningConsumerApp::handleMp1Message(int connId)
{
    EV << "MECPlatooningConsumerApp::handleMp1Message - payload: " << mp1HttpMessage->getBody() << endl;

    try
    {
        nlohmann::json jsonBody = nlohmann::json::parse(mp1HttpMessage->getBody()); // get the JSON structure
        if(!jsonBody.empty())
        {
            jsonBody = jsonBody[0];
            std::string serName = jsonBody["serName"];
            if(serName.compare("LocationService") == 0)
            {
                if(jsonBody.contains("transportInfo"))
                {
                    nlohmann::json endPoint = jsonBody["transportInfo"]["endPoint"]["addresses"];
                    EV << "address: " << endPoint["host"] << " port: " <<  endPoint["port"] << endl;
                    std::string address = endPoint["host"];
                    locationServiceAddress_ = L3AddressResolver().resolve(address.c_str());;
                    locationServicePort_ = endPoint["port"];

                    // once we obtained the endpoint of the Location Service, establish a connection with it
                    //connect(&serviceSocket_, locationServiceAddress_, locationServicePort_);
                }
            }
            else
            {
                EV << "MECPlatooningConsumerApp::handleMp1Message - LocationService not found"<< endl;
                locationServiceAddress_ = L3Address();
            }
        }

    }
    catch(nlohmann::detail::parse_error e)
    {
        EV <<  e.what() << std::endl;
        // body is not correctly formatted in JSON, manage it
        return;
    }

}

void MECPlatooningConsumerApp::handleHttpMessage(int connId)
{
}

void MECPlatooningConsumerApp::handleServiceMessage(int connId)
{
    if(serviceHttpMessage->getType() == RESPONSE)
    {
        HttpResponseMessage *rspMsg = dynamic_cast<HttpResponseMessage*>(serviceHttpMessage);
        if(rspMsg->getCode() == 200) // in response to a successful GET request
        {
            nlohmann::json jsonBody;
//            EV << "MECPlatooningConsumerApp::handleServiceMessage - response 200 " << rspMsg->getBody()<< endl;
            EV << "MECPlatooningConsumerApp::handleServiceMessage - response 200 " << endl;

            try
            {
              jsonBody = nlohmann::json::parse(rspMsg->getBody()); // get the JSON structure
              lastXposition = jsonBody["userInfo"]["locationInfo"]["x"];
              lastYposition = jsonBody["userInfo"]["locationInfo"]["y"];
              lastZposition = jsonBody["userInfo"]["locationInfo"]["z"];

              EV << "MECPlatooningConsumerApp::handleServiceMessage - lastXposition: " << lastXposition << " lastYposition: " << lastYposition << endl;

            }
            catch(nlohmann::detail::parse_error e)
            {
              EV <<  e.what() << endl;
              // body is not correctly formatted in JSON, manage it
              return;
            }

        }

        // some error occured, show the HTTP code for now
        else
        {
            EV << "MECPlatooningConsumerApp::handleServiceMessage - response with HTTP code:  " << rspMsg->getCode() << endl;
        }
    }
    // it is a request. It should not arrive, for now (think about sub-not)
    else
    {
        HttpRequestMessage *reqMsg = dynamic_cast<HttpRequestMessage*>(serviceHttpMessage);
        EV << "MECPlatooningConsumerApp::handleServiceMessage - response with HTTP code:  " << reqMsg->getMethod() << " discarded"  << endl;
    }

}

void MECPlatooningConsumerApp::registerToPlatooningProviderApp()
{
    inet::Packet* packet = new Packet ("PlatooningRegistrationPacket");
    auto registration = inet::makeShared<PlatooningRegistrationPacket>();
    registration->setType(REGISTRATION_REQUEST);
    registration->setConsumerAppId(getId());
    int chunkLen = sizeof(getId())+sizeof(localUePort);
    registration->setChunkLength(inet::B(chunkLen));

    packet->insertAtFront(registration);
    packet->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    platooningProviderAppSocket.sendTo(packet, platooningProviderAddress_, platooningProviderPort_);
}


void MECPlatooningConsumerApp::handleUeMessage(omnetpp::cMessage *msg)
{
    auto pk = check_and_cast<Packet *>(msg);
    auto platooningPkt = pk->peekAtFront<PlatooningAppPacket>();

    if (platooningPkt->getType() == DISCOVER_ASSOCIATE_PLATOON_REQUEST)
       handleDiscoverAndAssociatePlatoonRequest(msg);
    else if (platooningPkt->getType() == DISCOVER_PLATOONS_REQUEST)
       handleDiscoverPlatoonRequest(msg);
    else if (platooningPkt->getType() == ASSOCIATE_PLATOON_REQUEST)
       handleAssociatePlatoonRequest(msg);
    else if (platooningPkt->getType() == LEAVE_REQUEST)
        handleLeavePlatoonRequest(msg);
}


void MECPlatooningConsumerApp::handleDiscoverAndAssociatePlatoonRequest(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto joinReq = packet->removeAtFront<PlatooningDiscoverAndAssociatePlatoonPacket>();


    ueAppAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
    ueAppPort  = packet->getTag<L4PortInd>()->getSrcPort();

    // save direction info (needed for join request)
    direction_ = joinReq->getDirection();
    lastPosition_ = joinReq->getLastPosition();

    EV << "MECPlatooningConsumerApp::handleJoinPlatoonRequest - Forward join request to the MECPlatooningProviderApp" << endl;

    inet::Packet* fwPacket = new Packet (packet->getName());
    joinReq->setConsumerAppId(getId());
//    joinReq->setUeAddress(ueAppAddress);
    fwPacket->insertAtFront(joinReq);
    fwPacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    platooningProviderAppSocket.sendTo(fwPacket, platooningProviderAddress_, platooningProviderPort_);

    state_ = DISCOVERY_AND_ASSOCIATE;
    delete packet;
}



void MECPlatooningConsumerApp::handleDiscoverAndAssociatePlatoonResponse(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto discResp = packet->removeAtFront<PlatooningDiscoverAndAssociatePlatoonPacket>();

    if(state_ != DISCOVERY_AND_ASSOCIATE)
    {
        EV << "MECPlatooningConsumerApp::handleDiscoverAndAssociatePlatoonResponse - The status is not DISCOVERY_AND_ASSOCIATE. This message will be discarded." << endl;
        state_ = IDLE;
        // TODO send notification to UE?
    }

    if(discResp->getResult())
    {
        // require to join the platoon
        platooningControllerAddress_ = discResp->getControllerAddress();
        platooningControllerPort_ = discResp->getControllerPort();
        mecPlatoonProducerAppId_ = discResp->getProducerAppId();
        int platoonId = discResp->getControllerId();

        inet::Packet* pkt = new inet::Packet("PlatooningJoinPacket");
        auto joinReq = inet::makeShared<PlatooningJoinPacket>();

        joinReq->setType(JOIN_REQUEST);
        joinReq->setUeAddress(ueAppAddress);
        joinReq->setProducerAppId(mecPlatoonProducerAppId_);
        joinReq->setDirection(direction_);
        joinReq->setLastPosition(lastPosition_); // TODO a new position must be retrieved!!
        joinReq->setControllerId(platoonId);
        joinReq->setChunkLength(B(16));
        joinReq->setConsumerAppId(getId());

        pkt->insertAtFront(joinReq);
        pkt->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

        platooningControllerAppSocket.sendTo(pkt, platooningControllerAddress_, platooningControllerPort_);
        state_ = JOIN;
    }

    else
    {
        EV << "MECPlatooningConsumerApp::handleDiscoverAndAssociatePlatoonResponse - No platoon found, turn in IDLE state." << endl;
        state_ = IDLE;
        // TODO send notification to UE?
    }

    delete packet;
}


void MECPlatooningConsumerApp::handleProviderRegistrationResponse(cMessage* msg)
{
    EV << "MECPlatooningConsumerApp::handleProviderRegistrationResponse - Received registration response from the MECPlatooningProviderApp" << endl;

    // TODO do stuff

    delete msg;
}


void MECPlatooningConsumerApp::handleJoinPlatoonRequest(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto joinReq = packet->removeAtFront<PlatooningJoinPacket>();

    inet::L3Address ueAddress = packet->getTag<L3AddressInd>()->getSrcAddress();

    EV << "MECPlatooningConsumerApp::handleJoinPlatoonRequest - Forward join request to the MECPlatooningProviderApp" << endl;

    /*
     * TODO
     * choose here if just DISCOVER or DISCOVER and ASSOCIATE.
     * For now is always DISCOVER and ASSCOI
     */

    inet::Packet* fwPacket = new Packet (packet->getName());
    joinReq->setConsumerAppId(getId());

    joinReq->setUeAddress(ueAddress);
    fwPacket->insertAtFront(joinReq);
    fwPacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    platooningProviderAppSocket.sendTo(fwPacket, platooningProviderAddress_, platooningProviderPort_);

    state_ = DISCOVERY_AND_ASSOCIATE;

    delete packet;
}

void MECPlatooningConsumerApp::handleLeavePlatoonRequest(cMessage* msg)
{

    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto leaveReq = packet->removeAtFront<PlatooningLeavePacket>();

    EV << "MECPlatooningConsumerApp::handleLeavePlatoonRequest - Forward leave request to the MECPlatooningProviderApp" << endl;

    // TODO get latest UE position (from the Location Service) and add it to the message for the provider app

    inet::Packet* fwPacket = new Packet (packet->getName());
    leaveReq->setConsumerAppId(getId());
    leaveReq->setControllerId(controllerId_);
    fwPacket->insertAtFront(leaveReq);
    fwPacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    platooningControllerAppSocket.sendTo(fwPacket, platooningControllerAddress_, platooningControllerPort_);


    delete packet;
}

void MECPlatooningConsumerApp::handleJoinPlatoonResponse(cMessage* msg)
{
    // forward to client

    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto joinResp = packet->removeAtFront<PlatooningJoinPacket>();

    // set controller index
    if (joinResp->getResponse())
        controllerId_ = joinResp->getControllerId();

    EV << "MECPlatooningConsumerApp::handleJoinPlatoonResponse - Forward join response to the UE" << endl;

    // if response is True, start to require UE location from Location service (if established)
    // test
    //requestLocation();

    inet::Packet* fwPacket = new Packet (packet->getName());
    fwPacket->insertAtFront(joinResp);
    fwPacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    ueAppSocket.sendTo(fwPacket, ueAppAddress, ueAppPort);

    state_ = JOINED_PLATOON;
    delete packet;
}


void MECPlatooningConsumerApp::handleManoeuvreNotification(cMessage* msg)
{
    // forward to client

    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto manNotification = packet->removeAtFront<PlatooningManoeuvreNotificationPacket>();

    EV << "MECPlatooningConsumerApp::handleManoeuvreNotification - Forward manoeuvre notification to the UE" << endl;

    // if response is True, start to require UE location from Location service (if established)
    // test
    //requestLocation();

    inet::Packet* fwPacket = new Packet (packet->getName());
    fwPacket->insertAtFront(manNotification);
    fwPacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    ueAppSocket.sendTo(fwPacket, ueAppAddress, ueAppPort);

    state_ = MANOEUVRING;
    delete packet;
}

void MECPlatooningConsumerApp::handleQueuedJoinNotification(cMessage* msg)
{
    // forward to client

    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto qJoinNotification = packet->removeAtFront<PlatooningQueuedJoinNotificationPacket>();

    EV << "MECPlatooningConsumerApp::handleQueuedJoinNotification - Forward queued join notification to the UE" << endl;

    // if response is True, start to require UE location from Location service (if established)
    // test
    //requestLocation();

    inet::Packet* fwPacket = new Packet (packet->getName());
    fwPacket->insertAtFront(qJoinNotification);
    fwPacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    ueAppSocket.sendTo(fwPacket, ueAppAddress, ueAppPort);

    state_ = MANOEUVRING;
    delete packet;
}

void MECPlatooningConsumerApp::handleLeavePlatoonResponse(cMessage* msg)
{
    // forward to client

    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto leaveResp = packet->removeAtFront<PlatooningLeavePacket>();

    // detach controller index
    if (leaveResp->getResponse())
        controllerId_ = -1;

    EV << "MECPlatooningConsumerApp::handleLeavePlatoonResponse - Forward leave response to the UE" << endl;

    inet::Packet* fwPacket = new Packet (packet->getName());
    fwPacket->insertAtFront(leaveResp);
    fwPacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    ueAppSocket.sendTo(fwPacket, ueAppAddress, ueAppPort);

    delete packet;
}

void MECPlatooningConsumerApp::handlePlatoonCommand(cMessage* msg)
{
    // forward to client

    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto cmd = packet->removeAtFront<PlatooningInfoPacket>();

    EV << "MECPlatooningConsumerApp::handlePlatoonCommand - Forward new command to the UE" << endl;

    inet::Packet* fwPacket = new Packet (packet->getName());
    fwPacket->insertAtFront(cmd);
    fwPacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    ueAppSocket.sendTo(fwPacket, ueAppAddress, ueAppPort);

    delete packet;
}

//void MECPlatooningConsumerApp::modifySubscription()
//{
//    std::string body = "{  \"circleNotificationSubscription\": {"
//                       "\"callbackReference\" : {"
//                        "\"callbackData\":\"1234\","
//                        "\"notifyURL\":\"example.com/notification/1234\"},"
//                       "\"checkImmediate\": \"false\","
//                        "\"address\": \"" + ueAppAddress.str()+ "\","
//                        "\"clientCorrelator\": \"null\","
//                        "\"enteringLeavingCriteria\": \"Leaving\","
//                        "\"frequency\": 5,"
//                        "\"radius\": " + std::to_string(radius) + ","
//                        "\"trackingAccuracy\": 10,"
//                        "\"latitude\": " + std::to_string(centerPositionX) + ","           // as x
//                        "\"longitude\": " + std::to_string(centerPositionY) + ""        // as y
//                        "}"
//                        "}\r\n";
//    std::string uri = "/example/location/v2/subscriptions/area/circle/" + subId;
//    std::string host = serviceSocket_.getRemoteAddress().str()+":"+std::to_string(serviceSocket_.getRemotePort());
//    Http::sendPutRequest(&serviceSocket_, body.c_str(), host.c_str(), uri.c_str());
//}
//
//void MECPlatooningConsumerApp::sendSubscription()
//{
//    std::string body = "{  \"circleNotificationSubscription\": {"
//                           "\"callbackReference\" : {"
//                            "\"callbackData\":\"1234\","
//                            "\"notifyURL\":\"example.com/notification/1234\"},"
//                           "\"checkImmediate\": \"false\","
//                            "\"address\": \"" + ueAppAddress.str()+ "\","
//                            "\"clientCorrelator\": \"null\","
//                            "\"enteringLeavingCriteria\": \"Entering\","
//                            "\"frequency\": 5,"
//                            "\"radius\": " + std::to_string(radius) + ","
//                            "\"trackingAccuracy\": 10,"
//                            "\"latitude\": " + std::to_string(centerPositionX) + ","           // as x
//                            "\"longitude\": " + std::to_string(centerPositionY) + ""        // as y
//                            "}"
//                            "}\r\n";
//    std::string uri = "/example/location/v2/subscriptions/area/circle";
//    std::string host = serviceSocket_.getRemoteAddress().str()+":"+std::to_string(serviceSocket_.getRemotePort());
//
//    if(par("logger").boolValue())
//    {
//        ofstream myfile;
//        myfile.open ("example.txt", ios::app);
//        if(myfile.is_open())
//        {
//            myfile <<"["<< NOW << "] MEWarningAlertApp - Sent POST circleNotificationSubscription the Location Service \n";
//            myfile.close();
//        }
//    }
//
//    Http::sendPostRequest(&serviceSocket_, body.c_str(), host.c_str(), uri.c_str());
//}
//
//void MECPlatooningConsumerApp::sendDeleteSubscription()
//{
//    std::string uri = "/example/location/v2/subscriptions/area/circle/" + subId;
//    std::string host = serviceSocket_.getRemoteAddress().str()+":"+std::to_string(serviceSocket_.getRemotePort());
//    Http::sendDeleteRequest(&serviceSocket_, host.c_str(), uri.c_str());
//}

void MECPlatooningConsumerApp::established(int connId)
{
//    if(connId == mp1Socket_.getSocketId())
//    {
//        EV << "MECPlatooningConsumerApp::established - Mp1Socket"<< endl;
//
//        // once the connection with the Service Registry has been established, obtain the
//        // endPoint (address+port) of the Location Service
//        const char *uri = "/example/mec_service_mgmt/v1/services?ser_name=LocationService";
//        std::string host = mp1Socket_.getRemoteAddress().str()+":"+std::to_string(mp1Socket_.getRemotePort());
//
//        Http::sendGetRequest(&mp1Socket_, host.c_str(), uri);
//    }
//    else if (connId == serviceSocket_.getSocketId())
//    {
//        EV << "MECPlatooningConsumerApp::established - serviceSocket"<< endl;
//
//        //sendGETRequest
//
//
//        // here, the connection with the Location Service has been established
//        // TODO how to distinguish the service in case this app uses more than one MEC service?
//    }
//    else
//    {
//        throw cRuntimeError("MECPlatooningConsumerApp::socketEstablished - Socket %d not recognized", connId);
//    }
}

void MECPlatooningConsumerApp::requestLocation()
{
//    //check if the ueAppAddress is specified
//    if(ueAppAddress.isUnspecified())
//    {
//        EV << "MECPlatooningConsumerApp::requestLocation(): The IP address of the UE is unknown" << endl;
//        return;
//    }
//
//    if(serviceSocket_.getState() == inet::TcpSocket::CONNECTED)
//    {
//        EV << "MECPlatooningConsumerApp::requestLocation(): send request to the Location Service" << endl;
//        std::stringstream uri;
//        uri << "/example/location/v2/queries/users?address=acr:" << ueAppAddress.str();
//        std::string host = serviceSocket_.getRemoteAddress().str()+":"+std::to_string(serviceSocket_.getRemotePort());
//        Http::sendGetRequest(&serviceSocket_, host.c_str(), uri.str().c_str());
//    }
//    else
//    {
//        EV << "MECPlatooningConsumerApp::requestLocation(): Location Service not connected" << endl;
//    }
}





