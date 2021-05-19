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

#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"

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
    MecAppBase::initialize(stage);
    EV << "MEWarningAlertApp_rest::initialize - stage " << stage << endl;
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    //retrieving parameters
    size_ = par("packetSize");
   // ueSimbolicAddress = (char*)par("ueSimbolicAddress").stringValue();
//    meHostSimbolicAddress = (char*)par("localAddress").stringValue();
//    destAddress_ = inet::L3AddressResolver().resolve(ueSimbolicAddress);
//    destPort_ = par("uePort");
//    localPort_ = par("localPort");

    // set Udp Socket
    ueSocket.setOutputGate(gate("socketOut"));

    localUePort = par("localUePort");
    ueSocket.bind(localUePort);
//    ueSocket.bind(localPort_);

//    // set Tcp Socket
//    socket.bind(L3AddressResolver().resolve(meHostSimbolicAddress), localPort_);
//
//    socket.setCallback(this);
//    socket.setOutputGate(gate("socketOut"));

    //testing
//    EV << "MEWarningAlertApp_rest::initialize - MEWarningAlertApp_rest Symbolic Address: " << meHostSimbolicAddress << endl;
//    EV << "MEWarningAlertApp_rest::initialize - UEWarningAlertApp Symbolic Address: " << ueSimbolicAddress <<  " [" << destAddress_.str() << "]" << endl;

    cMessage *msg = new cMessage("connect!");
    scheduleAt(simTime() + 0.01, msg);

}

void MEWarningAlertApp_rest::handleMessage(cMessage *msg)
{
//        MecAppBase::handleMessage(msg);
    if (msg->isSelfMessage())
    {
        EV << "MecAppBase::handleMessage" << endl;
        if(strcmp(msg->getName(), "processedServiceResponse") == 0)
        {
            handleServiceMessage();
            if(serviceHttpMessage != nullptr)
            {
                delete serviceHttpMessage;
                serviceHttpMessage = nullptr;
            }
        }
        else if (strcmp(msg->getName(), "processedMp1Response") == 0)
        {
            handleMp1Message();
            if(mp1HttpMessage != nullptr)
            {
                delete mp1HttpMessage;
                mp1HttpMessage = nullptr;
            }
        }
        else
        {
            handleSelfMessage(msg);
        }
    }
    else
    {
        // from service or from ue app?
        if(serviceSocket_.belongsToSocket(msg))
        {
            serviceSocket_.processMessage(msg);
        }
        else if(mp1Socket_.belongsToSocket(msg))
        {
            mp1Socket_.processMessage(msg);
        }
        else
        {
            handleUeMessage(msg);
        }
    }
}

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

void MEWarningAlertApp_rest::handleUeMessage(omnetpp::cMessage *msg)
{
    // determine its source address/port

    auto pk = check_and_cast<Packet *>(msg);
    ueAppAddress = pk->getTag<L3AddressInd>()->getSrcAddress();
    ueAppPort = pk->getTag<L4PortInd>()->getSrcPort();

    circle = new cOvalFigure("circle");
    circle->setBounds(cFigure::Rectangle(150,200,120,120));
    circle->setLineWidth(2);
    circle->setLineColor(cFigure::RED);
    getSimulation()->getSystemModule()->getCanvas()->addFigure(circle);

    EV << "ARRIVATO QUALCOSA" << endl;

    cMessage *m = new cMessage("connect");
    scheduleAt(simTime()+0.5, m);
}

void MEWarningAlertApp_rest::handleServiceMessage()
{
    if(serviceHttpMessage->getType() == REQUEST)
    {
        Http::send204Response(&serviceSocket_);
        nlohmann::json jsonBody;
        EV << "MEClusterizeService::handleTcpMsg - REQUEST " << serviceHttpMessage->getBody()<< endl;
        try
        {

           jsonBody = nlohmann::json::parse(serviceHttpMessage->getBody()); // get the JSON structure
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
                    std::string host = serviceSocket_.getRemoteAddress().str()+":"+std::to_string(serviceSocket_.getRemotePort());
                    Http::sendDeleteRequest(&serviceSocket_, host.c_str(), uri.c_str());

                }

                //send subscription for leaving..
                //instantiation requirements and info
                alert->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
                alert->setType(INFO_MEAPP);


                //connection info
//                alert->setSourceAddress(meHostSimbolicAddress);
//                alert->setSourcePort(localPort_);
//                alert->setDestinationAddress(ueSimbolicAddress);

                //other info

                alert->setChunkLength(inet::B(size_));

                inet::Packet* packet = new inet::Packet("WarningAlertPacketInfo");
                packet->insertAtBack(alert);
                ueSocket.sendTo(packet, ueAppAddress, ueAppPort);

            }
        }
    }
    else if(serviceHttpMessage->getType() == RESPONSE)
    {
        HttpResponseMessage *rspMsg = dynamic_cast<HttpResponseMessage*>(serviceHttpMessage);


        if(std::strcmp(rspMsg->getCode(), "204") == 0)
        {
            EV << "MEClusterizeService::handleTcpMsg - response 204 " << rspMsg->getBody()<< endl;
             serviceSocket_.close();
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

void MEWarningAlertApp_rest::modifySubscription()
{
    std::string body = "{  \"circleNotificationSubscription\": {"
                       "\"callbackReference\" : {"
                        "\"callbackData\":\"1234\","
                        "\"notifyURL\":\"example.com/notification/1234\"},"
                       "\"checkImmediate\": \"false\","
                        "\"address\": \"" + ueAppAddress.str()+ "\","
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
    std::string host = serviceSocket_.getRemoteAddress().str()+":"+std::to_string(serviceSocket_.getRemotePort());
    Http::sendPutRequest(&serviceSocket_, body.c_str(), host.c_str(), uri.c_str());
}

void MEWarningAlertApp_rest::sendSubscription()
{
    std::string body = "{  \"circleNotificationSubscription\": {"
                           "\"callbackReference\" : {"
                            "\"callbackData\":\"1234\","
                            "\"notifyURL\":\"example.com/notification/1234\"},"
                           "\"checkImmediate\": \"false\","
                            "\"address\": \"" + ueAppAddress.str()+ "\","
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
    std::string host = serviceSocket_.getRemoteAddress().str()+":"+std::to_string(serviceSocket_.getRemotePort());
    Http::sendPostRequest(&serviceSocket_, body.c_str(), host.c_str(), uri.c_str());
}

void MEWarningAlertApp_rest::established(int connId)
{
    if(connId == mp1Socket_.getSocketId())
    {
        EV << "MEWarningAlertApp_rest::established - Mp1Socket"<< endl;
        // get endPoint of the required service
        const char *uri = "/example/mec_service_mgmt/v1/services?ser_name=LocationService";
        std::string host = mp1Socket_.getRemoteAddress().str()+":"+std::to_string(mp1Socket_.getRemotePort());

        Http::sendGetRequest(&mp1Socket_, "GET", uri, host.c_str());
        return;
    }
    else if (connId == serviceSocket_.getSocketId())
    {
        EV << "MEWarningAlertApp_rest::established - serviceSocket"<< endl;
        sendSubscription();
    }
    else
    {
        throw cRuntimeError("MecAppBase::socketEstablished - Socket %s not recognized", connId);
    }

    EV << "MEWarningAlertApp_rest::established" << endl;
//    sendSubscription("Entering");

}

void MEWarningAlertApp_rest::handleMp1Message()
{
//    throw cRuntimeError("QUiI");
    EV << "MEWarningAlertApp_rest::handleMp1Message - payload: " << mp1HttpMessage->getBody() << endl;

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
                    serviceAddress = L3AddressResolver().resolve(address.c_str());;
                    servicePort = endPoint["port"];
                    //connect(&serviceSocket_, serviceAddress, servicePort);
                }

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

void MEWarningAlertApp_rest::handleSelfMessage(cMessage *msg)
{
    if(strcmp(msg->getName(), "connect!") == 0)
    {
        EV << "MecAppBase::handleMessage- " << msg->getName() << endl;
        connect(&mp1Socket_, mp1Address, mp1Port);
    }

    else if(strcmp(msg->getName(), "connect") == 0)
        {
            EV << "MecAppBase::handleMessage- " << msg->getName() << endl;
            connect(&serviceSocket_, serviceAddress, servicePort);
        }

    delete msg;

}
