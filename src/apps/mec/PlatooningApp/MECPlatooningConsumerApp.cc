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
#include "apps/mec/PlatooningApp/PlatooningUtils.h"
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


    EV << "MECPlatooningConsumerApp::initialize - Mec application "<< getClassName() << " with mecAppId["<< mecAppId << "] has started!" << endl;

    // get platooning provider info
    platooningProviderAddress_ = L3Address(par("platooningProviderAddress").stringValue());
    platooningProviderPort_ = par("platooningProviderPort");

    // send registration request to the provider app
    registerToPlatooningProviderApp();

    // initially, not connected to any controller
    controllerIndex_ = -1;

    // connect with the service registry
//    EV << "MECPlatooningConsumerApp::initialize - Initialize connection with the service registry via Mp1" << endl;
//    connect(&mp1Socket_, mp1Address, mp1Port);
}

void MECPlatooningConsumerApp::handleMessage(cMessage *msg)
{
    if (!msg->isSelfMessage())
    {
        if(ueAppSocket.belongsToSocket(msg) || platooningProviderAppSocket.belongsToSocket(msg))
        {
            handleUeMessage(msg);
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
    registration->setMecAppId(getId());
    int chunkLen = sizeof(getId())+sizeof(localUePort);
    registration->setChunkLength(inet::B(chunkLen));

    packet->insertAtFront(registration);
    packet->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    platooningProviderAppSocket.sendTo(packet, platooningProviderAddress_, platooningProviderPort_);
}


void MECPlatooningConsumerApp::handleUeMessage(omnetpp::cMessage *msg)
{
    // determine its source address/port
    auto pk = check_and_cast<Packet *>(msg);
    auto platooningPkt = pk->peekAtFront<PlatooningAppPacket>();

    // TODO use enumerate variables instead of strings for comparison

    if(strcmp(platooningPkt->getType(), REGISTRATION_RESPONSE) == 0) // from mec provider app
        handleProviderRegistrationResponse(msg);

    else if(strcmp(platooningPkt->getType(), JOIN_REQUEST) == 0)     // from client app
    {
        // TODO move this somewhere else
        ueAppAddress = pk->getTag<L3AddressInd>()->getSrcAddress();
        ueAppPort = pk->getTag<L4PortInd>()->getSrcPort();

        handleJoinPlatoonRequest(msg);
    }

    else if (strcmp(platooningPkt->getType(), LEAVE_REQUEST) == 0)   // from client app
        handleLeavePlatoonRequest(msg);

    else if (strcmp(platooningPkt->getType(), JOIN_RESPONSE) == 0)   // from mec provider app
        handleJoinPlatoonResponse(msg);

    else if (strcmp(platooningPkt->getType(), LEAVE_RESPONSE) == 0)  // from mec provider app
        handleLeavePlatoonResponse(msg);

    else if (strcmp(platooningPkt->getType(), PLATOON_CMD) == 0)  // from mec provider app
        handlePlatoonCommand(msg);

    else
        throw cRuntimeError("MECPlatooningConsumerApp::handleUeMessage - packet not recognized");
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

    inet::Packet* fwPacket = new Packet (packet->getName());
    joinReq->setMecAppId(getId());
    joinReq->setUeAddress(ueAddress);
    fwPacket->insertAtFront(joinReq);
    fwPacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    platooningProviderAppSocket.sendTo(fwPacket, platooningProviderAddress_, platooningProviderPort_);

    delete packet;
}

void MECPlatooningConsumerApp::handleLeavePlatoonRequest(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto leaveReq = packet->removeAtFront<PlatooningLeavePacket>();

    EV << "MECPlatooningConsumerApp::handleLeavePlatoonRequest - Forward leave request to the MECPlatooningProviderApp" << endl;

    // TODO get latest UE position (from the Location Service) and add it to the message for the provider app

    inet::Packet* fwPacket = new Packet (packet->getName());
    leaveReq->setMecAppId(getId());
    leaveReq->setControllerIndex(controllerIndex_);
    fwPacket->insertAtFront(leaveReq);
    fwPacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    platooningProviderAppSocket.sendTo(fwPacket, platooningProviderAddress_, platooningProviderPort_);

    delete packet;
}

void MECPlatooningConsumerApp::handleJoinPlatoonResponse(cMessage* msg)
{
    // forward to client

    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto joinResp = packet->removeAtFront<PlatooningJoinPacket>();

    // set controller index
    if (joinResp->getResponse())
        controllerIndex_ = joinResp->getControllerIndex();

    EV << "MECPlatooningConsumerApp::handleJoinPlatoonResponse - Forward join response to the UE" << endl;

    // if response is True, start to require UE location from Location service (if established)
    // test
    //requestLocation();

    inet::Packet* fwPacket = new Packet (packet->getName());
    fwPacket->insertAtFront(joinResp);
    fwPacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    ueAppSocket.sendTo(fwPacket, ueAppAddress, ueAppPort);

    delete packet;
}

void MECPlatooningConsumerApp::handleLeavePlatoonResponse(cMessage* msg)
{
    // forward to client

    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto leaveResp = packet->removeAtFront<PlatooningLeavePacket>();

    // detach controller index
    if (leaveResp->getResponse())
        controllerIndex_ = -1;


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





