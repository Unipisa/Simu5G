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

#include "apps/mec/PlatooningApp/MECPlatooningApp.h"
#include "apps/mec/PlatooningApp/packets/PlatooningPacket_Types.h"
#include "apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket_Types.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/utils/httpUtils/json.hpp"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.h"

#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/Packet_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"

#include <fstream>

Define_Module(MECPlatooningApp);

using namespace inet;
using namespace omnetpp;

MECPlatooningApp::MECPlatooningApp(): MecAppBase()
{
}

MECPlatooningApp::~MECPlatooningApp()
{

}


void MECPlatooningApp::initialize(int stage)
{
    MecAppBase::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    //retrieving parameters
    newAcceleration_ = 0.0;

    // set UDP Socket
    localUePort = par("localUePort");
    ueSocket.setOutputGate(gate("socketOut"));
    ueSocket.bind(localUePort);

    EV << "MECPlatooningApp::initialize - Mec application "<< getClassName() << " with mecAppId["<< mecAppId << "] has started!" << endl;

    // connect with the service registry
    EV << "MECPlatooningApp::initialize - Initialize connection with the service registry via Mp1" << endl;
    connect(&mp1Socket_, mp1Address, mp1Port);
}

void MECPlatooningApp::handleMessage(cMessage *msg)
{
    if (!msg->isSelfMessage())
    {
        if(ueSocket.belongsToSocket(msg))
        {
            handleUeMessage(msg);
            return;
        }
    }
    MecAppBase::handleMessage(msg);

}

void MECPlatooningApp::finish()
{
    MecAppBase::finish();
    EV << "MECPlatooningApp::finish()" << endl;
    if(gate("socketOut")->isConnected())
    {

    }
}

void MECPlatooningApp::handleSelfMessage(cMessage *msg)
{
    if (strcmp(msg->getName(), "controllerTrigger") == 0)
    {
        control();

        scheduleAt(simTime()+0.1, msg);
    }
}

void MECPlatooningApp::handleMp1Message()
{
    EV << "MECPlatooningApp::handleMp1Message - payload: " << mp1HttpMessage->getBody() << endl;

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
                    connect(&serviceSocket_, locationServiceAddress_, locationServicePort_);
                }
            }
            else
            {
                EV << "MECPlatooningApp::handleMp1Message - LocationService not found"<< endl;
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

void MECPlatooningApp::handleServiceMessage()
{
//    if(serviceHttpMessage->getType() == REQUEST)
//    {
//        Http::send204Response(&serviceSocket_); // send back 204 no content
//        nlohmann::json jsonBody;
//        EV << "MEClusterizeService::handleTcpMsg - REQUEST " << serviceHttpMessage->getBody()<< endl;
//        try
//        {
//           jsonBody = nlohmann::json::parse(serviceHttpMessage->getBody()); // get the JSON structure
//        }
//        catch(nlohmann::detail::parse_error e)
//        {
//           std::cout  <<  e.what() << std::endl;
//           // body is not correctly formatted in JSON, manage it
//           return;
//        }
//
//        if(jsonBody.contains("subscriptionNotification"))
//        {
//            if(jsonBody["subscriptionNotification"].contains("enteringLeavingCriteria"))
//            {
//                nlohmann::json criteria = jsonBody["subscriptionNotification"]["enteringLeavingCriteria"] ;
//                auto alert = inet::makeShared<WarningAlertPacket>();
//                alert->setType(WARNING_ALERT);
//
//                if(criteria == "Entering")
//                {
//                    EV << "MEClusterizeService::handleTcpMsg - Ue is Entered in the danger zone "<< endl;
//                    alert->setDanger(true);
//
//                    if(par("logger").boolValue())
//                    {
//                        ofstream myfile;
//                        myfile.open ("example.txt", ios::app);
//                        if(myfile.is_open())
//                        {
//                            myfile <<"["<< NOW << "] MEWarningAlertApp - Received circleNotificationSubscription notification from Location Service. UE's entered the red zone! \n";
//                            myfile.close();
//                        }
//                    }
//
//                    // send subscription for leaving..
//                    modifySubscription();
//
//                }
//                else if (criteria == "Leaving")
//                {
//                    EV << "MEClusterizeService::handleTcpMsg - Ue left from the danger zone "<< endl;
//                    alert->setDanger(false);
//                    if(par("logger").boolValue())
//                    {
//                        ofstream myfile;
//                        myfile.open ("example.txt", ios::app);
//                        if(myfile.is_open())
//                        {
//                            myfile <<"["<< NOW << "] MEWarningAlertApp - Received circleNotificationSubscription notification from Location Service. UE's exited the red zone! \n";
//                            myfile.close();
//                        }
//                    }
//                    sendDeleteSubscription();
//                }
//
//                alert->setPositionX(jsonBody["subscriptionNotification"]["terminalLocationList"]["currentLocation"]["x"]);
//                alert->setPositionY(jsonBody["subscriptionNotification"]["terminalLocationList"]["currentLocation"]["y"]);
//                alert->setChunkLength(inet::B(20));
//                alert->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
//
//                inet::Packet* packet = new inet::Packet("WarningAlertPacketInfo");
//                packet->insertAtBack(alert);
//                ueSocket.sendTo(packet, ueAppAddress, ueAppPort);
//
//            }
//        }
//    }
//    else if(serviceHttpMessage->getType() == RESPONSE)
//    {
//        HttpResponseMessage *rspMsg = dynamic_cast<HttpResponseMessage*>(serviceHttpMessage);
//
//        if(rspMsg->getCode() == 204) // in response to a DELETE
//        {
//            EV << "MEClusterizeService::handleTcpMsg - response 204, removing circle" << rspMsg->getBody()<< endl;
//             serviceSocket_.close();
//
//        }
//        else if(rspMsg->getCode() == 201) // in response to a POST
//        {
//            nlohmann::json jsonBody;
//            EV << "MEClusterizeService::handleTcpMsg - response 201 " << rspMsg->getBody()<< endl;
//            try
//            {
//               jsonBody = nlohmann::json::parse(rspMsg->getBody()); // get the JSON structure
//            }
//            catch(nlohmann::detail::parse_error e)
//            {
//               EV <<  e.what() << endl;
//               // body is not correctly formatted in JSON, manage it
//               return;
//            }
//            std::string resourceUri = jsonBody["circleNotificationSubscription"]["resourceURL"];
//            std::size_t lastPart = resourceUri.find_last_of("/");
//            if(lastPart == std::string::npos)
//            {
//                EV << "1" << endl;
//                return;
//            }
//            // find_last_of does not take in to account if the uri has a last /
//            // in this case subscriptionType would be empty and the baseUri == uri
//            // by the way the next if statement solve this problem
//            std::string baseUri = resourceUri.substr(0,lastPart);
//            //save the id
//            subId = resourceUri.substr(lastPart+1);
//            EV << "subId: " << subId << endl;
//        }
//    }

}

void MECPlatooningApp::handleUeMessage(omnetpp::cMessage *msg)
{
    // determine its source address/port
    auto pk = check_and_cast<Packet *>(msg);
    ueAppAddress = pk->getTag<L3AddressInd>()->getSrcAddress();
    ueAppPort = pk->getTag<L4PortInd>()->getSrcPort();

    auto platooningPkt = pk->peekAtFront<PlatooningAppPacket>();

    if(strcmp(platooningPkt->getType(), JOIN_REQUEST) == 0)
    {
        handleJoinPlatoonRequest(msg);

        if(par("logger").boolValue())
        {
            ofstream myfile;
            myfile.open ("example.txt", ios::app);
            if(myfile.is_open())
            {
                myfile <<"["<< NOW << "] MEWarningAlertApp - Received message from UE, connecting to the Location Service\n";
                myfile.close();
            }
        }

        // start controlling
        cMessage *trigger = new cMessage("controllerTrigger");
        scheduleAt(simTime()+0.1, trigger);
    }
    else if (strcmp(platooningPkt->getType(), LEAVE_REQUEST) == 0)
    {
        handleLeavePlatoonRequest(msg);
    }
    else
    {
        throw cRuntimeError("MECPlatooningApp::handleUeMessage - packet not recognized");
    }
}


void MECPlatooningApp::handleJoinPlatoonRequest(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto joinReq = packet->removeAtFront<PlatooningJoinPacket>();

    EV << "MECPlatooningApp::handleJoinRequest - Received join request from ..." << endl;

    // accept the request and send ACK
    inet::Packet* responsePacket = new Packet (packet->getName());
    joinReq->setType(JOIN_RESPONSE);
    joinReq->setResponse(true);
    joinReq->setColor("green");
    responsePacket->insertAtFront(joinReq);
    responsePacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    ueSocket.sendTo(responsePacket, ueAppAddress, ueAppPort);
    EV << "UEPlatooningApp::sendJoinPlatoonRequest() - Join response sent to the UE" << endl;

    delete packet;
}

void MECPlatooningApp::handleLeavePlatoonRequest(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto leaveReq = packet->removeAtFront<PlatooningLeavePacket>();

    EV << "MECPlatooningApp::handleJoinRequest - Received leave request from ..." << endl;

    inet::Packet* responsePacket = new Packet (packet->getName());

    // accept the request and send ACK
    leaveReq->setType(LEAVE_RESPONSE);
    leaveReq->setResponse(true);
    responsePacket->insertAtFront(leaveReq);
    responsePacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    ueSocket.sendTo(responsePacket, ueAppAddress, ueAppPort);
    EV << "UEPlatooningApp::sendJoinPlatoonRequest() - Join response sent to the UE" << endl;

    delete packet;
}

void MECPlatooningApp::control()
{
    inet::Packet* pkt = new inet::Packet("PlatooningInfoPacket");
    auto cmd = inet::makeShared<PlatooningInfoPacket>();
    cmd->setType(PLATOON_CMD);
    cmd->setNewAcceleration(newAcceleration_);
    cmd->setChunkLength(inet::B( sizeof(double) ));
    cmd->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    pkt->insertAtBack(cmd);

    ueSocket.sendTo(pkt, ueAppAddress, ueAppPort);

    EV << "UEPlatooningApp::control() - Sent new acceleration value[" << newAcceleration_ << "] to the UE" << endl;

}

//void MECPlatooningApp::modifySubscription()
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
//void MECPlatooningApp::sendSubscription()
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
//void MECPlatooningApp::sendDeleteSubscription()
//{
//    std::string uri = "/example/location/v2/subscriptions/area/circle/" + subId;
//    std::string host = serviceSocket_.getRemoteAddress().str()+":"+std::to_string(serviceSocket_.getRemotePort());
//    Http::sendDeleteRequest(&serviceSocket_, host.c_str(), uri.c_str());
//}

void MECPlatooningApp::established(int connId)
{
    if(connId == mp1Socket_.getSocketId())
    {
        EV << "MECPlatooningApp::established - Mp1Socket"<< endl;

        // once the connection with the Service Registry has been established, obtain the
        // endPoint (address+port) of the Location Service
        const char *uri = "/example/mec_service_mgmt/v1/services?ser_name=LocationService";
        std::string host = mp1Socket_.getRemoteAddress().str()+":"+std::to_string(mp1Socket_.getRemotePort());

        Http::sendGetRequest(&mp1Socket_, host.c_str(), uri);
    }
    else if (connId == serviceSocket_.getSocketId())
    {
        EV << "MECPlatooningApp::established - serviceSocket"<< endl;

        // here, the connection with the Location Service has been established
        // TODO how to distinguish the service in case this app uses more than one MEC service?
    }
    else
    {
        throw cRuntimeError("MecAppBase::socketEstablished - Socket %s not recognized", connId);
    }
}






