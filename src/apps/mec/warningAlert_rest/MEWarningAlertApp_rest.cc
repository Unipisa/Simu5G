//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "apps/mec/warningAlert_rest/MEWarningAlertApp_rest.h"
#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/Packet_m.h"

#include "nodes/mec/MEPlatform/MeServices/httpUtils/httpUtils.h"
#include "nodes/mec/MEPlatform/MeServices/httpUtils/json.hpp"
#include "nodes/mec/MEPlatform/MeServices/packets/HttpResponseMessage/HttpResponseMessage.h"



Define_Module(MEWarningAlertApp_rest);

using namespace inet;
using namespace omnetpp;

MEWarningAlertApp_rest::MEWarningAlertApp_rest()
{
    meHost = nullptr;
    mePlatform = nullptr;;
    serviceRegistry = nullptr;;
    circle = nullptr; // circle danger zone

}
MEWarningAlertApp_rest::~MEWarningAlertApp_rest()
{
    if(circle != nullptr)
    {
        if(getSimulation()->getSystemModule()->getCanvas()->findFigure(circle) != -1)
            getSimulation()->getSystemModule()->getCanvas()->removeFigure(circle);
        delete circle;
//        dropAndDelete(circle);
//        circle->dropAndDelete(circle);
    }

    }


void MEWarningAlertApp_rest::initialize(int stage)
{
    MeAppBase::initialize(stage);
    EV << "MEWarningAlertApp_rest::initialize - stage " << stage << endl;
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
    meHost = getParentModule();
    mePlatform = meHost->getSubmodule("mePlatform");

    serviceRegistry = check_and_cast<ServiceRegistry *>(mePlatform->getSubmodule("serviceRegistry"));

    // set Udp Socket
    ueSocket.setOutputGate(gate("socketOut"));
    ueSocket.bind(localPort_);

//    // set Tcp Socket
//    socket.bind(L3AddressResolver().resolve(meHostSimbolicAddress), localPort_);
//
//    socket.setCallback(this);
//    socket.setOutputGate(gate("socketOut"));


    cMessage *m = new cMessage("connect");
            scheduleAt(simTime()+0.5, m);

    circle = new cOvalFigure("circle");
    circle->setBounds(cFigure::Rectangle(150,200,120,120));
    circle->setLineWidth(2);
    circle->setLineColor(cFigure::RED);
    getSimulation()->getSystemModule()->getCanvas()->addFigure(circle);


    //testing
    EV << "MEWarningAlertApp_rest::initialize - MEWarningAlertApp_rest Symbolic Address: " << meHostSimbolicAddress << endl;
    EV << "MEWarningAlertApp_rest::initialize - UEWarningAlertApp Symbolic Address: " << ueSimbolicAddress <<  " [" << destAddress_.str() << "]" << endl;

}

void MEWarningAlertApp_rest::handleMessageWhenUp(cMessage *msg)
{
    EV << "MeAppBase::handleMessageWhenUp" << endl;
    if (msg->isSelfMessage())
        handleSelfMessage(msg);
    else if (socket.belongsToSocket(msg))
        socket.processMessage(msg);
    else if (ueSocket.belongsToSocket(msg))
    {
        EV << "UDP MESSAGE" << endl;
        delete msg;
    }
    else
        delete msg;

}


//void MEWarningAlertApp_rest::handleMessage(cMessage *msg)
//{
//    EV << "MEWarningAlertApp_rest::handleMessage - \n";
//    MeAppBase::handleMessage(msg);
//
////
////    if(msg->isSelfMessage())
////    {
////        if(strcmp(msg->getName(), "connect") == 0)
////         {
////             connect();
////             delete msg;
////         }
////    }
////    else{
////
////        if(ueSocket.belongsToSocket(msg))
////        {
////            EV << "UDP message" << endl;
////            return msg;
////        }
////        else if(socket.belongsToSocket(msg))
////        {
////            socket.processMessage(msg);
////        }
////    }
////        else
////        {
////            delete msg;
////        }
//
//}

void MEWarningAlertApp_rest::finish(){

    EV << "MEWarningAlertApp_rest::finish - Sending " << STOP_MEAPP << " to the MEIceAlertService" << endl;

    if(gate("socketOut")->isConnected()){
        // TODO
//        WarningAlertPacket* packet = new WarningAlertPacket();
//        packet->setType(STOP_MEAPP);
//
//        send((cMessage*)packet, "socketOut");
    }
}

void MEWarningAlertApp_rest::handleServiceMessage()
{
    if(currentHttpMessage->getType() == REQUEST)
    {
        Http::send204Response(&socket);
        nlohmann::json jsonBody;
        EV << "MEClusterizeService::handleTcpMsg - REQUEST " << currentHttpMessage->getBody()<< endl;
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

        if(jsonBody.contains("subscriptionNotification"))
        {
            if(jsonBody["subscriptionNotification"].contains("enteringLeavingCriteria"))
            {
                nlohmann::json criteria = jsonBody["subscriptionNotification"]["enteringLeavingCriteria"] ;
                auto alert = inet::makeShared<WarningAlertPacket>();

                if(criteria == "Entering")
                {
                    EV << "MEClusterizeService::handleTcpMsg - Ue is Entered in the danger zone "<< endl;
                    alert->setDanger(true);
                    modifySubscription();

                }
                else if (criteria == "Leaving")
                {
                    EV << "MEClusterizeService::handleTcpMsg - Ue is Exited in the danger zone "<< endl;
                    alert->setDanger(false);
                    std::string uri = "/example/location/v2/subscriptions/area/circle/" + subId;
                    std::string host = socket.getRemoteAddress().str()+":"+std::to_string(socket.getRemotePort());
                    Http::sendDeleteRequest(&socket, host.c_str(), uri.c_str());

                }

                //send subscription for leaving..
                //instantiation requirements and info
                alert->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
                alert->setType(INFO_MEAPP);


                //connection info
                alert->setSourceAddress(meHostSimbolicAddress);
                alert->setSourcePort(localPort_);
                alert->setDestinationAddress(ueSimbolicAddress);

                //other info

                alert->setChunkLength(inet::B(size_));

                inet::Packet* packet = new inet::Packet("WarningAlertPacketInfo");
                packet->insertAtBack(alert);
                ueSocket.sendTo(packet, destAddress_, destPort_);

            }
        }
    }
    else if(currentHttpMessage->getType() == RESPONSE)
    {
        HttpResponseMessage *rspMsg = dynamic_cast<HttpResponseMessage*>(currentHttpMessage);


        if(std::strcmp(rspMsg->getCode(), "204") == 0)
        {
            EV << "MEClusterizeService::handleTcpMsg - response 204 " << rspMsg->getBody()<< endl;
             socket.close();
             getSimulation()->getSystemModule()->getCanvas()->removeFigure(circle);

        }
        else if(std::strcmp(rspMsg->getCode(), "201") == 0)
        {
            nlohmann::json jsonBody;
            EV << "MEClusterizeService::handleTcpMsg - response 201 " << rspMsg->getBody()<< endl;
            try
            {

               jsonBody = nlohmann::json::parse(rspMsg->getBody()); // get the JSON structure
            }
            catch(nlohmann::detail::parse_error e)
            {
               EV <<  e.what() << endl;
               // body is not correctly formatted in JSON, manage it
               return;
            }
            std::string resourceUri = jsonBody["circleNotificationSubscription"]["resourceURL"];
            std::size_t lastPart = resourceUri.find_last_of("/");
            if(lastPart == std::string::npos)
            {
                EV << "1" << endl;
                return;
            }
            // find_last_of does not take in to account if the uri has a last /
            // in this case subscriptionType would be empty and the baseUri == uri
            // by the way the next if statement solve this problem
            std::string baseUri = resourceUri.substr(0,lastPart);
            //save the id
            subId = resourceUri.substr(lastPart+1);
            EV << "subId: " << subId << endl;
        }
    }


}

void MEWarningAlertApp_rest::connect()
{
    socket.renewSocket();
    EV << "MEWarningAlertApp_rest::connect" << endl;
    // we need a new connId if this is not the first connection
    //socket.renewSocket();

    // use the service Registry to get ip and port of the service
    serviceSockAddress = serviceRegistry->retrieveMeService("LocationService");
    if(serviceSockAddress.addr.getType() == L3Address::NONE)
        throw cRuntimeError("MEWarningAlertApp_rest::connect - MeService LocationService not found!");
    EV << "MEWarningAlertApp_rest::connect to:  " << serviceSockAddress.str() << endl;
    socket.connect(serviceSockAddress.addr, serviceSockAddress.port);
}

void MEWarningAlertApp_rest::modifySubscription()
{
    //    sendSubscription("Entering");
        std::string body = "{  \"circleNotificationSubscription\": {"
                           "\"callbackReference\" : {"
                            "\"callbackData\":\"1234\","
                            "\"notifyURL\":\"example.com/notification/1234\"},"
                           "\"checkImmediate\": \"false\","
                            "\"address\": \"" + destAddress_.str()+ "\","
                            "\"clientCorrelator\": \"ciao\","
                            "\"enteringLeavingCriteria\": \"Leaving\","
                            "\"frequency\": 10,"
                            "\"radius\": 60,"
                            "\"trackingAccuracy\": 10,"
                            "\"latitude\": 210,"
                            "\"longitude\": 260"
                            "}"
                            "}\r\n";
                std::string uri = "/example/location/v2/subscriptions/area/circle/" + subId;
                std::string host = socket.getRemoteAddress().str()+":"+std::to_string(socket.getRemotePort());
                Http::sendPutRequest(&socket, body.c_str(), host.c_str(), uri.c_str());


}

void MEWarningAlertApp_rest::established(int connId)
{
    EV << "MEWarningAlertApp_rest::established" << endl;
//    sendSubscription("Entering");
    std::string body = "{  \"circleNotificationSubscription\": {"
                       "\"callbackReference\" : {"
                        "\"callbackData\":\"1234\","
                        "\"notifyURL\":\"example.com/notification/1234\"},"
                       "\"checkImmediate\": \"false\","
                        "\"address\": \"" + destAddress_.str()+ "\","
                        "\"clientCorrelator\": \"ciao\","
                        "\"enteringLeavingCriteria\": \"Entering\","
                        "\"frequency\": 10,"
                        "\"radius\": 60,"
                        "\"trackingAccuracy\": 10,"
                        "\"latitude\": 210,"
                        "\"longitude\": 260"
                        "}"
                        "}\r\n";
            std::string uri = "/example/location/v2/subscriptions/area/circle";
            std::string host = socket.getRemoteAddress().str()+":"+std::to_string(socket.getRemotePort());
            Http::sendPostRequest(&socket, body.c_str(), host.c_str(), uri.c_str());
}

void MEWarningAlertApp_rest::handleSelfMessage(cMessage *msg)
{

    if(strcmp(msg->getName(), "connect") == 0)
    {
        connect();
    }
    delete msg;

}
