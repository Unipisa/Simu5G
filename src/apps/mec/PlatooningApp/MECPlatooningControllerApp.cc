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

#include "apps/mec/PlatooningApp/MECPlatooningControllerApp.h"
#include "apps/mec/PlatooningApp/platoonSelection/PlatoonSelectionSample.h"
#include "apps/mec/PlatooningApp/platoonController/SafePlatoonController.h"
#include "apps/mec/PlatooningApp/platoonController/RajamaniPlatoonController.h"
#include "apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket_Types.h"
#include "apps/mec/PlatooningApp/packets/CurrentPlatoonRequestTimer_m.h"
#include "apps/mec/PlatooningApp/packets/tags/PlatooningPacketTags_m.h"

#include "apps/mec/PlatooningApp/platoonController/SafePlatoonController.h"
#include "apps/mec/PlatooningApp/platoonController/RajamaniPlatoonController.h"

#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/utils/httpUtils/json.hpp"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.h"

#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/Packet_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"


#include <fstream>

Define_Module(MECPlatooningControllerApp);
//Register_Class(MECPlatooningControllerApp);
using namespace inet;
using namespace omnetpp;

MECPlatooningControllerApp::MECPlatooningControllerApp(): MecAppBase()
{
    platoonSelection_ = nullptr;
    isConfigured_ = false;
    updatePositionTimer_ = nullptr;
    controlTimer_ = nullptr;
    maneuvringVehicle_ = nullptr; // one at a time
    maneuvringVehicleJoinMsg_ = nullptr;
    state_ = INACTIVE_CONTROLLER;

}

MECPlatooningControllerApp::~MECPlatooningControllerApp()
{
    delete platoonSelection_;
    delete longitudinalController_;
    delete lateralController_;
    delete maneuvringVehicle_;
//    delete maneuvringVehicleJoinMsg_;
    while(!joinRequests_.isEmpty())
    {
        delete joinRequests_.pop();
    }
    cancelAndDelete(heartbeatMsg_);
}

void MECPlatooningControllerApp::initialize(int stage)
{
    MecAppBase::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    // create a new selection algorithm
    // TODO read from parameter
    platoonSelection_ = new PlatoonSelectionSample();


    // set UDP Sockets
    platooningProducerAppPort_ = par("localUePort");
    platooningProducerAppSocket_.setOutputGate(gate("socketOut"));
    platooningProducerAppSocket_.bind(platooningProducerAppPort_);

    platooningConsumerAppsPort_ = platooningProducerAppPort_ + (int)mecHost->par("maxMECApps") + 1;
    platooningConsumerAppsSocket_.setOutputGate(gate("socketOut"));
    platooningConsumerAppsSocket_.bind(platooningConsumerAppsPort_);

    isConfigured_ = false;

    EV << "MECPlatooningControllerApp::initialize - MEC application "<< getClassName() << " with mecAppId["<< mecAppId << "] has started!" << endl;

   // connect with the service registry
   EV << "MECPlatooningControllerApp::initialize - Initialize connection with the Service Registry via Mp1" << endl;

      // retrieve global PlatoonProducerApp endpoints

    mp1Socket_ = addNewSocket();

//    // Add local producerApp
//    ProducerAppInfo localProducerAppInfo;
//    localProducerAppInfo.port = -1; // it is not used
//    localProducerAppInfo.address = L3Address(); // it is not used
//
//    localProducerAppInfo.locationServicePort = -1;    // set with service registry
//    localProducerAppInfo.locationServiceAddress = L3Address(); // set with service registry
//    localProducerAppInfo.locationServiceSocket = addNewSocket();
//    producerAppEndpoint_[mecAppId] = localProducerAppInfo;

   connect(mp1Socket_, mp1Address, mp1Port);
}

void MECPlatooningControllerApp::handleMessage(cMessage *msg)
{
    if (!msg->isSelfMessage())
    {
        if(platooningConsumerAppsSocket_.belongsToSocket(msg))
        {
            if(!isConfigured_)
            {
                EV << "MECPlatooningControllerApp::handleMessage - The controller is not configured yet. It cannot handle any packet from a consumer app" << endl;
                delete msg;
            }
            // determine its source address/port
            auto pk = check_and_cast<Packet *>(msg);
            ueAppAddress = pk->getTag<L3AddressInd>()->getSrcAddress();
            ueAppPort = pk->getTag<L4PortInd>()->getSrcPort();

            auto platooningPkt = pk->peekAtFront<PlatooningAppPacket>();
            EV << "MECPlatooningControllerApp::handleMessage(): messageType " << platooningPkt->getType()  << endl;

            if(platooningPkt->getType() == JOIN_REQUEST)
            {
                handleJoinPlatoonRequest(msg);

                // start controlling
                // TODO remove?
//                cMessage *trigger = new cMessage("controllerTrigger");
//                scheduleAt(simTime()+0.1, trigger);
            }
            else if (platooningPkt->getType() == LEAVE_REQUEST)
            {
                handleLeavePlatoonRequest(msg);
            }
            else
            {
                throw cRuntimeError("MECPlatooningControllerApp::handleMessage - packet from platoonConsumerApp not recognized");
            }

            return;
        }
        else if(platooningProducerAppSocket_.belongsToSocket(msg))
        {
            // determine its source address/port
            auto pk = check_and_cast<Packet *>(msg);
            auto platooningPkt = pk->peekAtFront<PlatooningAppPacket>();
            EV << "MECPlatooningControllerApp::handleMessage(): messageType " << platooningPkt->getType()  << endl;
            if (platooningPkt->getType() == CONF_CONTROLLER)
            {
                handleControllerConfiguration(msg);
            }
            else
            {
                throw cRuntimeError("MECPlatooningControllerApp::handleMessage - packet from platoonProviderApp not recognized");
            }
            return;
        }

    }
    MecAppBase::handleMessage(msg);
}

void MECPlatooningControllerApp::finish()
{
    MecAppBase::finish();
    EV << "MECPlatooningControllerApp::finish()" << endl;
    if(gate("socketOut")->isConnected())
    {

    }
}

void MECPlatooningControllerApp::handleSelfMessage(cMessage *msg)
{
    if (strcmp(msg->getName(), "PlatooningTimer") == 0)
    {
        PlatooningTimer* timer = check_and_cast<PlatooningTimer*>(msg);

        switch(timer->getType())
        {
            case PLATOON_LONGITUDINAL_CONTROL_TIMER:
            {
                ControlTimer* ctrlTimer = check_and_cast<ControlTimer*>(timer);
                handleLongitudinalControllerTimer(ctrlTimer);
                break;
            }
            case PLATOON_LATERAL_CONTROL_TIMER:
            {
                ControlTimer* ctrlTimer = check_and_cast<ControlTimer*>(timer);
                handleLateralControllerTimer(ctrlTimer);
                break;
            }
            case PLATOON_LONGITUDINAL_UPDATE_POSITION_TIMER:
            case PLATOON_LATERAL_UPDATE_POSITION_TIMER:

            {
                UpdatePositionTimer* posTimer = check_and_cast<UpdatePositionTimer*>(timer);
                handleUpdatePositionTimer(posTimer);
                break;
            }

            default: throw cRuntimeError("MECPlatooningControllerApp::handleSelfMessage - unrecognized timer type %d", timer->getType());
        }
    }
    else if (strcmp(msg->getName(), "heartbeatNotification") == 0)
    {
        sendHeartbeatNotification();
        scheduleAt(simTime() + heartbeatPeriod_, heartbeatMsg_);
    }

}

void MECPlatooningControllerApp::sendHeartbeatNotification()
{
    EV << "MECPlatooningControllerApp::sendHeartbeatNotification()" << endl;
    inet::Packet* pkt = new inet::Packet("PlatooningControllerNotificationPacket");
    auto notification = inet::makeShared<PlatooningControllerNotificationPacket>();

    notification->setType(CONTROLLER_NOTIFICATION);
    notification->setNotificationType(HEARTBEAT);
    notification->setControllerId(controllerId_);
    notification->setProducerAppId(producerAppId_);

    auto members = getPlatoonMembers();

    notification->setVehiclesOrder(members);
    notification->setHeartbeatTimeStamp(simTime());

    notification->setChunkLength(inet::B(20 +  sizeof(members)));
    notification->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    pkt->insertAtBack(notification);

    platooningProducerAppSocket_.sendTo(pkt, platooningProducerAppEndpoint_.address, platooningProducerAppEndpoint_.port);
}

void MECPlatooningControllerApp::handleHttpMessage(int connId)
{
    if (connId == mp1Socket_->getSocketId())
    {
        handleMp1Message(connId);
    }
    else// if (connId == serviceSocket_->getSocketId())
    {
        handleServiceMessage(connId);
    }
//    else
//    {
//        throw cRuntimeError("Socket with connId [%d] is not present in thi application!", connId);
//    }
}

void MECPlatooningControllerApp::handleMp1Message(int connId)
{
    // for now I only have just one Service Registry
    HttpMessageStatus* msgStatus = (HttpMessageStatus*)mp1Socket_->getUserData();
    mp1HttpMessage = (HttpBaseMessage*)msgStatus->httpMessageQueue.front();
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

//                      // once we obtained the endpoint of the Location Service, establish a connection with it
//                      auto localProducerApp = producerAppEndpoint_.find(mecAppId);
//                      localProducerApp->second.locationServiceAddress = locationServiceAddress_;
//                      localProducerApp->second.locationServicePort = locationServicePort_;

                      //                      // once we obtained the endpoint of the Location Service, establish a connection with it

                      ProducerAppInfo producerAppinfo;
                      producerAppinfo.port =  platooningProducerAppEndpoint_.port;
                      producerAppinfo.address =     platooningProducerAppEndpoint_.address;
                      producerAppinfo.locationServicePort = locationServicePort_;
                      producerAppinfo.locationServiceAddress = locationServiceAddress_;
                      producerAppinfo.locationServiceSocket = addNewSocket();


                      // TODO create socket
                      producerAppEndpoint_[producerAppId_] = producerAppinfo;


//                      connect(localProducerApp->second.locationServiceSocket , locationServiceAddress_, locationServicePort_);
                      // connect to all the federated location services
                      for(auto const& producerApp : producerAppEndpoint_)
                      {
                          EV << "MECPlatooningControllerApp::handleMp1Message(): connecting to Location Service of producer app with ID: "<< producerApp.first <<
                                 " through socket with connId: " <<  producerApp.second.locationServiceSocket-> getSocketId() << endl;
                          connect(producerApp.second.locationServiceSocket, producerApp.second.locationServiceAddress, producerApp.second.locationServicePort );
                      }

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

void MECPlatooningControllerApp::handleServiceMessage(int connId)
{
    HttpBaseMessage* httpMessage = nullptr;

    for(auto& producerApp : producerAppEndpoint_)
    {
        if(producerApp.second.locationServiceSocket->getSocketId() == connId)
        {
            HttpMessageStatus* msgStatus = (HttpMessageStatus*)producerApp.second.locationServiceSocket->getUserData();
            httpMessage = (HttpBaseMessage*)msgStatus->httpMessageQueue.front();
            break;
        }
    }
    if(httpMessage == nullptr)
    {
        EV << "MECPlatooningControllerApp::handleServiceMessage - Location Service associated with SocketId [" << connId << "] not found" << endl;
        return;
    }


    if(httpMessage == nullptr)
    {
        throw cRuntimeError("MECPlatooningControllerApp::handleServiceMessage() - httpMessage is null!");
    }

    if(httpMessage->getType() == RESPONSE)
    {
        HttpResponseMessage *rspMsg = dynamic_cast<HttpResponseMessage*>(httpMessage);
        if(rspMsg->getCode() == 200) // in response to a successful GET request
        {
            nlohmann::json jsonBody;
            EV << "MECPlatooningControllerApp::handleServiceMessage - response 200 " << endl;

//            received_++;

            try
            {
              jsonBody = nlohmann::json::parse(rspMsg->getBody()); // get the JSON structure
              std::vector<UEInfo> uesInfo;
//              EV << "BODY" <<jsonBody.dump(2) << endl;
              //get the positions of all ues
              if(!jsonBody.contains("userInfo"))
              {
                  EV << "MECPlatooningControllerApp::handleServiceMessage: ues not found" << endl;
                  return;
              }
              nlohmann::json jsonUeInfo = jsonBody["userInfo"];
              if(jsonUeInfo.is_array())
              {
                  for(int i = 0 ; i < jsonUeInfo.size(); i++)
                  {
                        nlohmann::json ue = jsonUeInfo.at(i);
                        //EV << "UE INFO " << ue << endl;
                        if(ue.is_string())
                        {
                            continue;
                        }
                        UEInfo ueInfo;
                        std::string address = ue["address"];
                        ueInfo.address = inet::L3Address(address.substr(address.find(":")+1, address.size()).c_str());
                        ueInfo.timestamp = ue["timeStamp"]["nanoSeconds"];
                        ueInfo.position = inet::Coord(double(ue["locationInfo"]["x"]), double(ue["locationInfo"]["y"]), double(ue["locationInfo"]["z"]));
                        if(ue["locationInfo"].contains("velocity"))
                            ueInfo.speed = ue["locationInfo"]["velocity"]["horizontalSpeed"];
                        else
                            ueInfo.speed = -1000; // TODO define
                        uesInfo.push_back(ueInfo);
                  }
              }
              else
              {
                  if(!jsonUeInfo.is_string())
                  {

                      UEInfo ueInfo;
                      std::string address = jsonUeInfo["address"];
                      ueInfo.address = inet::L3Address(address.substr(address.find(":")+1, address.size()).c_str());
                      ueInfo.timestamp = jsonUeInfo["timeStamp"]["nanoSeconds"];
                      ueInfo.position = inet::Coord(double(jsonUeInfo["locationInfo"]["x"]), double(jsonUeInfo["locationInfo"]["y"]), double(jsonUeInfo["locationInfo"]["z"]));
                      if(jsonUeInfo["locationInfo"].contains("velocity"))
                          ueInfo.speed = jsonUeInfo["locationInfo"]["velocity"]["horizontalSpeed"];
                      else
                          ueInfo.speed = -1000; // TODO define
                      uesInfo.push_back(ueInfo);
                  }
              }

              // notify the controller
              updatePlatoonPositions(&uesInfo);

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
            EV << "MECPlatooningControllerApp::handleServiceMessage - response with HTTP code:  " << rspMsg->getCode() << endl;
        }
    }
    // it is a request. It should not arrive, for now (think about sub-not)
    else
    {
        HttpRequestMessage *reqMsg = dynamic_cast<HttpRequestMessage*>(httpMessage);
        EV << "MECPlatooningControllerApp::handleServiceMessage - response with HTTP code:  " << reqMsg->getMethod() << " discarded"  << endl;
    }
}

void MECPlatooningControllerApp::handleControllerConfiguration(cMessage * msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto confMsg = packet->removeAtFront<PlatooningConfigureControllerPacket>();

    platooningProducerAppEndpoint_.address = packet->getTag<L3AddressInd>()->getSrcAddress();
    platooningProducerAppEndpoint_.port = packet->getTag<L4PortInd>()->getSrcPort();
    direction_ = confMsg->getDirection();

    //   cModule* vi = this->getParentModule();
   cModule* localProducerAppModule = this->getModuleByPath("^.bgApp[0]");
   MECPlatooningProducerApp* localProducerApp = check_and_cast<MECPlatooningProducerApp *>(localProducerAppModule);

   cValueArray *producerAppEndpoints = check_and_cast<cValueArray *>(localProducerApp->par("federatedProducerApps").objectValue());
   for(int i = 0; i < producerAppEndpoints->size(); ++i)
   {
       cValueMap *entry = check_and_cast<cValueMap *>(producerAppEndpoints->get(i).objectValue());
       ProducerAppInfo producerAppinfo;
       producerAppinfo.port = entry->get("port");
       producerAppinfo.address = L3AddressResolver().resolve(entry->get("address").stringValue());

       producerAppinfo.locationServicePort = entry->get("locationServicePort");
       producerAppinfo.locationServiceAddress = L3AddressResolver().resolve(entry->get("locationServiceAddress").stringValue());
       producerAppinfo.locationServiceSocket = addNewSocket();

       int id = entry->get("id");

       // TODO create socket
       producerAppEndpoint_[id] = producerAppinfo;

       EV <<"MECPlatooningControllerApp::initialize - id: " << id <<" address: "  << producerAppinfo.address.str() <<" port " << producerAppinfo.port << endl;
   }

   // the local location service is connected via UALCMP

   producerAppId_ =  confMsg->getProducerAppId();

   minimumDistanceForManoeuvre_ = par("minimumDistanceForManoeuvre").doubleValue();

   heartbeatPeriod_ = confMsg->getHeartbeatPeriod();
   heartbeatMsg_ = new cMessage("heartbeatNotification");
   scheduleAt(simTime() + heartbeatPeriod_, heartbeatMsg_);


   mp1Socket_ = addNewSocket();
   connect(mp1Socket_, mp1Address, mp1Port);


   // plus the local one

    controllerId_ = confMsg->getControllerId();

    // instantiate controllers
    controlPerdiodLongitudinal_ = confMsg->getLongitudinalControlPeriod();;
    controlPerdiodLateral_ = confMsg->getLateralControlPeriod();;

    updatePositionTimerLongitudinal_ = confMsg->getLongitudinalUpdatePositionPeriod();
    updatePositionTimerLateral_ = confMsg->getLateralUpdatePositionPeriod();


    adjustPosition_  = confMsg->getAdjustPosition();
    sendBulk_ = confMsg->getSendBulk();

    if(strcmp( confMsg->getLongitudinalController() , "Rajamani") == 0 )
    {
        longitudinalController_ =   new RajamaniPlatoonController(this, controllerId_, controlPerdiodLongitudinal_, updatePositionTimerLongitudinal_);
        longitudinalController_->setDirection(confMsg->getDirection());
    }
    else if(strcmp( confMsg->getLongitudinalController(), "Safe") == 0 )
    {
        longitudinalController_ =   new SafePlatoonController(this, controllerId_, controlPerdiodLongitudinal_, updatePositionTimerLongitudinal_);
        longitudinalController_->setDirection(confMsg->getDirection());
    }
    else
    {
        throw cRuntimeError("MECPlatooningControllerApp::handleControllerConfiguration - Longitudinal controller %s does not exists!", confMsg->getLongitudinalController());
    }



    if(strcmp( confMsg->getLateralController() , "Rajamani") == 0 )
    {
        lateralController_ = new RajamaniPlatoonController(this, controllerId_, controlPerdiodLateral_, updatePositionTimerLateral_, true);
        lateralController_->setDirection(confMsg->getDirection());
    }
    else
    {
        throw cRuntimeError("MECPlatooningControllerApp::handleControllerConfiguration - Lateral controller %s does not exists!", confMsg->getLongitudinalController());
    }



    isConfigured_ = true;


    inet::Packet* responsePacket = new Packet(packet->getName());
    confMsg->setType(CONF_CONTROLLER_RESPONSE);
    confMsg->setResponse(true);

    responsePacket->insertAtBack(confMsg);

    // TODO look for endpoint in the structure?
    platooningProducerAppSocket_.sendTo(responsePacket, platooningProducerAppEndpoint_.address, platooningProducerAppEndpoint_.port);
    delete packet;
}

bool MECPlatooningControllerApp::addMember(int consumerAppId, L3Address& carIpAddress ,Coord& position, int producerAppId)
{
    bool success = addPlatoonMember(consumerAppId, producerAppId,  position, carIpAddress);

    if(success)
    {
        EV << "MECPlatooningControllerApp::addMember - MEC App " << consumerAppId << " added to platoon with Id: " << controllerId_ << endl;
        remoteConsumerAppToProducerApp_[consumerAppId] = producerAppId;
    }
    else
    {
        EV << "MECPlatooningControllerApp::addMember - MEC App " << consumerAppId << "is not added to any platoon" << endl;

    }

    return success;
}

void MECPlatooningControllerApp::finalizeJoinPlatoonRequest(cMessage* msg, bool success)
{
    EV << "MECPlatooningControllerApp::finalizeJoinPlatoonRequest" << endl;

    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    L3Address consumerAppAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
    int consumerAppPort = packet->getTag<L4PortInd>()->getSrcPort();
    auto req = packet->removeAtFront<PlatooningJoinPacket>();

    req->setType(JOIN_RESPONSE);

    if(success)
    {
        //add the car to the platoon

        int consumerAppId = req->getConsumerAppId();
        int producerAppId = req->getProducerAppId();
        L3Address carIpAddress = req->getUeAddress();

        // if the finalizeJoinPlatoonRequest is called for the maneuvring vehicle,
        // i.e msg == maneuvringVehicleJoinMsg_, the position must be the last one received during
        // the maneouvre!
        inet::Coord position;
        if(msg != maneuvringVehicleJoinMsg_)
        {
            position = req->getLastPosition();
        }
        else
        {
            position = maneuvringVehicle_->getPosition();
        }

        addMember(consumerAppId, carIpAddress, position, producerAppId);

        req->setColor("green");
        // save info in some data structure

        // add CONTROLLER_NOTIFICATION TO THE PRODUCER APP
        inet::Packet* pkt = new inet::Packet("PlatooningControllerNotificationPacket");
        auto notification = inet::makeShared<PlatooningControllerNotificationPacket>();

        notification->setType(CONTROLLER_NOTIFICATION);
        notification->setNotificationType(NEW_MEMBER);
        notification->setConsumerAppId(req->getConsumerAppId());

        notification->setControllerId(controllerId_);
        notification->setProducerAppId(req->getProducerAppId());

        auto members = getPlatoonMembers();

        notification->setVehiclesOrder(members);
        notification->setHeartbeatTimeStamp(simTime());

        notification->setChunkLength(inet::B(20 +  sizeof(members)));
        notification->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
        pkt->insertAtBack(notification);

        platooningProducerAppSocket_.sendTo(pkt, platooningProducerAppEndpoint_.address, platooningProducerAppEndpoint_.port);

        EV << "MECPlatooningControllerApp::handleJoinPlatoonRequest - MECPlatooningConsumerApp with id [" << req->getConsumerAppId() << "] added to platoon" << endl;
    }

    else
    {
        req->setColor("red");
        EV << "MECPlatooningControllerApp::handleJoinPlatoonRequest - MECPlatooningConsumerApp with id [" << req->getConsumerAppId() << "] not added to platoon" << endl;

    }

    req->setResponse(success);

    inet::Packet* responsePacket = new Packet(packet->getName());
    responsePacket->insertAtBack(req);
    // TODO look for endpoint in the structure?
    platooningConsumerAppsSocket_.sendTo(responsePacket, consumerAppAddress, consumerAppPort);
    EV << "MECPlatooningControllerApp::sendJoinPlatoonRequest - Join response sent to MECPlatooningConsumerApp with id [" << req->getConsumerAppId() << endl;

    if(!joinRequests_.isEmpty() && state_ != MANOEUVRE){
        EV << "MECPlatooningControllerApp::handleJoinPlatoonRequest - Process next join request" << endl;
        cMessage* newJoin = check_and_cast<cMessage*>(joinRequests_.pop());
        maneuvringVehicleJoinMsg_ = nullptr;
        delete maneuvringVehicle_;
        maneuvringVehicle_ = nullptr;
        handleJoinPlatoonRequest(newJoin, true);
    }
    else if(joinRequests_.isEmpty() && state_ != MANOEUVRE)
    {
        maneuvringVehicleJoinMsg_ = nullptr;
        delete maneuvringVehicle_;
        maneuvringVehicle_ = nullptr;
        if(membersInfo_.size() > 0)
        {
            EV << "MECPlatooningControllerApp::handleJoinPlatoonRequest - No other vehicles are waiting, switch to cruise" << endl;
            switchState(CRUISE); // no other vehicles are waiting, switch to cruise

        }
        else
        {
            EV << "MECPlatooningControllerApp::handleJoinPlatoonRequest - No other vehicles are waiting and the platoon is empty, switch to inactive" << endl;
            switchState(INACTIVE_CONTROLLER);
            // TODO sendo notification to the producer
        }
    }

    delete packet; // delete join request
}

void MECPlatooningControllerApp::switchState(ControllerState state)
{
    EV << "MECPlatooningControllerApp::switchState" << endl;

    if(controlTimer_ == nullptr)
    {
        controlTimer_ = new ControlTimer("PlatooningTimer");
    }
    if(updatePositionTimer_ == nullptr)
    {
        updatePositionTimer_ = new UpdatePositionTimer("PlatooningTimer");
    }

    if(state == CRUISE){
        EV << "MECPlatooningControllerApp::switchState - CRUISE " << endl;
        controlTimer_->setType(PLATOON_LONGITUDINAL_CONTROL_TIMER);
        controlTimer_->setPeriod(controlPerdiodLongitudinal_); // it starts with the lateral period.
        startTimer(controlTimer_, controlPerdiodLongitudinal_);

        updatePositionTimer_->setType(PLATOON_LONGITUDINAL_UPDATE_POSITION_TIMER);
        updatePositionTimer_->setPeriod(updatePositionTimerLongitudinal_); // it starts with the lateral period.
        startTimer(updatePositionTimer_, updatePositionTimerLongitudinal_); // TODO check timing

        state_ = CRUISE;

    }

    else if(state == MANOEUVRE)
    {
        EV << "MECPlatooningControllerApp::switchState - MANOEUVRE " << endl;
        controlTimer_->setType(PLATOON_LATERAL_CONTROL_TIMER);
        controlTimer_->setPeriod(controlPerdiodLateral_); // it starts with the lateral period.
        startTimer(controlTimer_, controlPerdiodLateral_);

        updatePositionTimer_->setType(PLATOON_LATERAL_UPDATE_POSITION_TIMER);
        updatePositionTimer_->setPeriod(updatePositionTimerLateral_); // it starts with the lateral period.
        startTimer(updatePositionTimer_, updatePositionTimerLateral_); // TODO check timing

        state_ = MANOEUVRE;
    }

    else if(state  == INACTIVE_CONTROLLER)
    {

        stopTimer(state == CRUISE ? PLATOON_LONGITUDINAL_CONTROL_TIMER : PLATOON_LATERAL_CONTROL_TIMER);
        stopTimer(state == CRUISE ? PLATOON_LONGITUDINAL_UPDATE_POSITION_TIMER : PLATOON_LATERAL_UPDATE_POSITION_TIMER);
        state_ = INACTIVE_CONTROLLER;
    }

}

void MECPlatooningControllerApp::handleJoinPlatoonRequest(cMessage* msg, bool fromQueue)
{
    /*
     * We should check where the car is. For now, we should only add the car to the platoon and start the CRUISE phase
     */
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    L3Address consumerAppAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
    int consumerAppPort = packet->getTag<L4PortInd>()->getSrcPort();
    auto req = packet->peekAtFront<PlatooningJoinPacket>();

    int consumerAppId = req->getConsumerAppId();
    int producerAppId = req->getProducerAppId();
    L3Address carIpAddress = req->getUeAddress();
    Coord position = req->getLastPosition();

    EV << "MECPlatooningControllerApp::handleJoinPlatoonRequest - Join request arrived from MECPlatooningConsumerApp with id [" << consumerAppId << "] associated to UE [" << carIpAddress.str() << "]" << endl;

    if(state_ == INACTIVE_CONTROLLER || state_ == CRUISE)
    {

        if(fromQueue && state_ == CRUISE) // the request was a pending one and there is at least a car in the platoon
        {
            string address = "acr:";
            address += carIpAddress.str();
            sendGetRequest(producerAppId, address);

            EV << "MECPlatooningControllerApp::handleJoinPlatoonRequest: ASK REQUEST" << endl;
            if(maneuvringVehicle_ != nullptr)
            {
                throw cRuntimeError("MECPlatooningControllerApp::handleJoinPlatoonRequest(): Error, maneuvringVehicle_ is not null!");
            }

            maneuvringVehicle_ = new PlatoonVehicleInfo();
            maneuvringVehicle_->setPosition(position);
            maneuvringVehicle_->setUeAddress(carIpAddress);
            maneuvringVehicle_->setMecAppId(consumerAppId);
            maneuvringVehicle_->setProducerAppId(producerAppId);
            maneuvringVehicleJoinMsg_ = msg;

            ConsumerAppInfo info;
            info.address = consumerAppAddress;
            info.port = consumerAppPort;
            info.producerAppId = producerAppId; //for now, after the join it will be set

            consumerAppEndpoint_[consumerAppId] = info;

        }

        else
        {
            EV << "MECPlatooningControllerApp::handleJoinPlatoonRequest: switching to state MANOEUVRE" << endl;
            if(maneuvringVehicle_ != nullptr)
            {
                throw cRuntimeError("MECPlatooningControllerApp::handleJoinPlatoonRequest(): Error, maneuvringVehicle_ is not null!");
            }

            maneuvringVehicle_ = new PlatoonVehicleInfo();
            maneuvringVehicle_->setPosition(position);
            maneuvringVehicle_->setUeAddress(carIpAddress);
            maneuvringVehicle_->setMecAppId(consumerAppId);
            maneuvringVehicle_->setProducerAppId(producerAppId);
            maneuvringVehicleJoinMsg_ = msg;

            ConsumerAppInfo info;
            info.address = consumerAppAddress;
            info.port = consumerAppPort;
            info.producerAppId = producerAppId; //for now, after the join it will be set

            consumerAppEndpoint_[consumerAppId] = info;

            // START MANOUVRING
            switchState(MANOEUVRE);

            sendManoeuvreNotification(msg);
        }
    }
    else
    {
        // it is already in MANOEUVRE, queue
        // suppose all checks are ok, add the car to the platoon
        PlatoonVehicleInfo* lastCar = getLastPlatoonCar();
        if(lastCar != nullptr && position.distance(lastCar->getPosition()) > minimumDistanceForManoeuvre_)
        {
            EV << "MECPlatooningControllerApp::handleJoinPlatoonRequest - The car relative to queue join request is " << position.distance(lastCar->getPosition()) << "meters  far from the platoon!! The request from MECPlatoonConsumerApp with id [" << consumerAppId << "] is rejected!" << endl;
            finalizeJoinPlatoonRequest(msg, false);
            return;
        }
        else
        {
            EV << "MECPlatooningControllerApp::handleJoinPlatoonRequest: Another car is currently the platoon. The request from MECPlatoonConsumerApp with id [" << consumerAppId << "] is queued up" << endl;

            joinRequests_.insert(msg);
            sendQueuedJoinNotification(msg);
        }
    }
}

void MECPlatooningControllerApp::sendQueuedJoinNotification(cMessage* msg)
{
    EV << "MECPlatooningControllerApp::sendQueuedJoinNotification" << endl;
    inet::Packet* packet =  new inet::Packet("PlatooningQueuedJoinNotificationPacket");
    auto manNotification = inet::makeShared<PlatooningQueuedJoinNotificationPacket>();
    manNotification->setType(QUEUED_JOIN_NOTIFICATION);
    manNotification->setResponse(true);
    manNotification->setColor("yellow");
    manNotification->setControllerId(controllerId_);
    manNotification->setChunkLength(B(10));

    packet->insertAtBack(manNotification);
    packet->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    // send response to the UE's MEC app

    inet::Packet* pkt = check_and_cast<inet::Packet*>(msg);
    L3Address address = pkt->getTag<L3AddressInd>()->getSrcAddress();
    int port = pkt->getTag<L4PortInd>()->getSrcPort();
    platooningConsumerAppsSocket_.sendTo(packet, address, port);

}


void MECPlatooningControllerApp::sendManoeuvreNotification(cMessage* msg)
{
    inet::Packet* packet =  new inet::Packet("PlatooningManoeuvreNotificationPacket");
    auto manNotification = inet::makeShared<PlatooningManoeuvreNotificationPacket>();
    manNotification->setType(MANOEUVRE_NOTIFICATION);
    manNotification->setResponse(true);
    manNotification->setColor("cyan");
    manNotification->setControllerId(controllerId_);
    manNotification->setChunkLength(B(10));

    packet->insertAtBack(manNotification);
    packet->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    // send response to the UE's MEC app
    inet::Packet* pkt = check_and_cast<inet::Packet*>(msg);
    L3Address address = pkt->getTag<L3AddressInd>()->getSrcAddress();
    int port = pkt->getTag<L4PortInd>()->getSrcPort();
    auto req = pkt->peekAtFront<PlatooningJoinPacket>();
    platooningConsumerAppsSocket_.sendTo(packet, address, port);

    EV << "MECPlatooningControllerApp::sendManoeuvreNotification - Manoeuvre notification sent to MECPlatoonConsumerApp with id [" << req->getConsumerAppId() << "]" << endl;
}

void MECPlatooningControllerApp::sendJoinPlatoonResponse(bool success, int platoonIndex, cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    L3Address address = packet->getTag<L3AddressInd>()->getSrcAddress();
    int port = packet->getTag<L4PortInd>()->getSrcPort();
    auto joinReq = packet->removeAtFront<PlatooningJoinPacket>();

    joinReq->setType(JOIN_RESPONSE);
    if (success)
    {
        // accept the request and send ACK
        joinReq->setResponse(true);
        joinReq->setControllerId(platoonIndex);
        joinReq->setColor("green");
    }
    else
    {
        joinReq->setResponse(false);
    }

    // prepare response
    inet::Packet* responsePacket = new Packet (packet->getName());
    responsePacket->insertAtBack(joinReq);
    responsePacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    // send response to the UE's MEC app
    platooningConsumerAppsSocket_.sendTo(responsePacket, address, port);
    EV << "MECPlatooningControllerApp::sendJoinPlatoonRequest - Join response sent to MECPlatoonConsumerApp with id [" << joinReq->getConsumerAppId() << "]" << endl;

    delete packet;
}

void MECPlatooningControllerApp::handleLeavePlatoonRequest(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto leaveReq = packet->removeAtFront<PlatooningLeavePacket>();
    L3Address mecAppAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
    int mecAppPort = packet->getTag<L4PortInd>()->getSrcPort();

    int mecAppId = leaveReq->getConsumerAppId();
//    int controllerIndex = leaveReq->getControllerId();

    // check if the MEC consumer app is in a local platoon or  not
    auto consApp = consumerAppEndpoint_.find(mecAppId);

    // TODO remove the info from consumerAppEndpoint_ structure?


    if(consApp == consumerAppEndpoint_.end())
        throw cRuntimeError("MECPlatooningControllerApp::handleLeavePlatoonRequest - MEC consumer app with index %d does not exist", mecAppId);

    bool success = removePlatoonMember(mecAppId);

    if(success)
    {
        consumerAppEndpoint_.erase(mecAppId);

        // add CONTROLLER_NOTIFICATION TO THE PRODUCER APP
        inet::Packet* pkt = new inet::Packet("PlatooningControllerNotificationPacket");
        auto notification = inet::makeShared<PlatooningControllerNotificationPacket>();

        notification->setType(CONTROLLER_NOTIFICATION);
        notification->setNotificationType(REMOVED_MEMBER);
        notification->setConsumerAppId(leaveReq->getConsumerAppId());

        notification->setControllerId(controllerId_);
        notification->setProducerAppId(leaveReq->getProducerAppId());

        auto members = getPlatoonMembers();
        notification->setVehiclesOrder(members);
        notification->setHeartbeatTimeStamp(simTime());

        notification->setChunkLength(inet::B(20 + sizeof(members)));
        notification->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
        pkt->insertAtBack(notification);

        platooningProducerAppSocket_.sendTo(pkt, platooningProducerAppEndpoint_.address, platooningProducerAppEndpoint_.port);

        EV << "MECPlatooningControllerApp::handleJoinPlatoonRequest - MECPlatooningConsumerApp with id [" << leaveReq->getConsumerAppId() << "] removed from platoon" << endl;
    }


    inet::Packet* responsePacket = new Packet (packet->getName());
    leaveReq->setType(LEAVE_RESPONSE);

       // accept the request and send ACK
    leaveReq->setResponse(success);

    responsePacket->insertAtBack(leaveReq);
    responsePacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    // send response to the UE's consumer MEC app
    platooningConsumerAppsSocket_.sendTo(responsePacket, mecAppAddress, mecAppPort);
    EV << "MECPlatooningControllerApp::handleLeavePlatoonRequest - Leave response sent to the UE" << endl;

    delete packet;
}


bool MECPlatooningControllerApp::removePlatoonMember(int mecAppId)
{
    PlatoonMembersInfo::iterator it = membersInfo_.find(mecAppId);
    if (it == membersInfo_.end())
    {
        EV << "MECPlatooningControllerApp::removePlatoonMember - Member [" << mecAppId << "] was not part of platoon " << endl;
        return false;
    }

    // remove mecAppId from relevant data structures
    std::list<int>::iterator pt = platoonPositions_.begin();
    for ( ; pt != platoonPositions_.end(); ++pt)
    {
        if (*pt == mecAppId)
        {
            platoonPositions_.erase(pt);
            break;
        }
    }
    membersInfo_.erase(mecAppId);

    // check if the platoon is now empty
    if (membersInfo_.empty())
    {
        if(state_ == CRUISE)
            switchState(INACTIVE_CONTROLLER);
        // TODO if is it MANOEUVRE?
    }

    EV << "PlatoonControllerBase::removePlatoonMember - Member [" << mecAppId << "] removed from platoon " << endl;
    return true;
}


void MECPlatooningControllerApp::startTimer(cMessage* msg, double timeOffset)
{
    if (timeOffset < 0.0)
        throw cRuntimeError("MECPlatooningControllerApp::startTimer - time offset not valid[%f]", timeOffset);

    PlatooningTimer* timer = check_and_cast<PlatooningTimer*>(msg);
    PlatooningTimerType timerType = (PlatooningTimerType)timer->getType();

    if (timerType == PLATOON_LATERAL_CONTROL_TIMER || timerType == PLATOON_LONGITUDINAL_CONTROL_TIMER)
    {
        // if a control timer was already set for this controller, cancel it
        if(controlTimer_ != nullptr && controlTimer_->isScheduled())
            cancelEvent(controlTimer_);

        EV << "MECPlatooningControllerApp::startTimer() - New ControlTimer set for controller " << controllerId_ << " in " << timeOffset << "s" << endl;

        controlTimer_ = check_and_cast<ControlTimer*>(timer);
        //schedule timer
        scheduleAt(simTime() + timeOffset, controlTimer_);
    }
    else if (timerType == PLATOON_LATERAL_UPDATE_POSITION_TIMER || timerType == PLATOON_LONGITUDINAL_UPDATE_POSITION_TIMER)
    {
        // if a position timer was already set for this controller, cancel it
        if(updatePositionTimer_ != nullptr && updatePositionTimer_->isScheduled())
            cancelEvent(updatePositionTimer_);

        EV << "MECPlatooningControllerApp::startTimer() - New UpdatePositionTimer set for controller " << controllerId_ << " in " << timeOffset << "s" << endl;

        updatePositionTimer_ = check_and_cast<UpdatePositionTimer*>(timer);
        //schedule timer
        scheduleAt(simTime() + timeOffset, updatePositionTimer_);
    }

}

void MECPlatooningControllerApp::stopTimer(PlatooningTimerType timerType)
{
    if (timerType == PLATOON_LATERAL_CONTROL_TIMER || timerType == PLATOON_LONGITUDINAL_CONTROL_TIMER)
    {
        if (controlTimer_ == nullptr || !controlTimer_->isScheduled())
            throw cRuntimeError("MECPlatooningControllerApp::stopTimer - no active ControlTimer");

        cancelAndDelete(controlTimer_);
        EV << "MECPlatooningControllerApp::stopTimer - ControlTimer for controller " << controllerId_ << "has been stopped" << endl;
    }
    else if (timerType == PLATOON_LATERAL_UPDATE_POSITION_TIMER || timerType == PLATOON_LONGITUDINAL_CONTROL_TIMER)
    {
        if (updatePositionTimer_ == nullptr || !updatePositionTimer_->isScheduled())
            throw cRuntimeError("MECPlatooningControllerApp::stopTimer - no active UpdatePositionTimer with index %d", controllerId_);

        cancelAndDelete(updatePositionTimer_);
        EV << "MECPlatooningControllerApp::stopTimer - UpdatePositionTimer for controller " << controllerId_ << "has been stopped" << endl;
    }
    else
        throw cRuntimeError("MECPlatooningControllerApp::stopTimer - unrecognized timer type %d", timerType);
}

void MECPlatooningControllerApp::handleLongitudinalControllerTimer(ControlTimer* ctrlTimer)
{
    int controllerIndex = ctrlTimer->getControllerId();

    EV << "MECPlatooningControllerApp::handleControlTimer - Control platoon with index " << controllerIndex << endl;

    const CommandList* cmdList = longitudinalController_->controlPlatoon();

    // TODO the function might end here, the controller will provide the commands as soon as it will be finished

    if (cmdList != nullptr)
    {
        // send a command to every connected MecApp
        // TODO this will be changed once we know the interface with the controller

        CommandList::const_iterator it = cmdList->begin();
        for (; it != cmdList->end(); ++it)
        {
            int mecAppId = it->first;
            double newAcceleration = it->second.acceleration;
            inet::L3Address precUeAddress = it->second.address;

            auto consumerApp = consumerAppEndpoint_.find(mecAppId);
            if (consumerApp == consumerAppEndpoint_.end())
            {
                throw cRuntimeError("MECPlatooningControllerApp::handleControlTimer - endpoint for MEC app %d not found.", mecAppId);
            }
            inet::Packet* pkt = new inet::Packet("PlatooningInfoPacket");

            auto cmd = inet::makeShared<PlatooningInfoPacket>();
            cmd->setType(PLATOON_CMD);
            cmd->setConsumerAppId(mecAppId); // used in case of it is sent to an other produceApp
            cmd->setNewAcceleration(newAcceleration);
            cmd->setChunkLength(inet::B( sizeof(double) + sizeof(int) + 1));
            cmd->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
            cmd->addTagIfAbsent<inet::PrecedingVehicleTag>()->setUeAddress(precUeAddress);
            pkt->insertAtBack(cmd);

            //send to the relative consumer App
            platooningConsumerAppsSocket_.sendTo(pkt, consumerApp->second.address , consumerApp->second.port);
        }
        delete cmdList;
    }

    // re-schedule controller timer
    if (ctrlTimer->getPeriod() > 0.0)
        scheduleAt(simTime() + ctrlTimer->getPeriod(), ctrlTimer);
    else
        stopTimer((PlatooningTimerType)ctrlTimer->getType());
}

void MECPlatooningControllerApp::handleLateralControllerTimer(ControlTimer* ctrlTimer)
{
    EV << "MECPlatooningControllerApp::handleControlTimer - Control platoon" << endl;

    const CommandList* cmdList = lateralController_->controlPlatoon();

    // TODO the function might end here, the controller will provide the commands as soon as it will be finished

    if (cmdList != nullptr)
    {
        // send a command to every connected MecApp
        // TODO this will be changed once we know the interface with the controller

        CommandList::const_iterator it = cmdList->begin();
        for (; it != cmdList->end(); ++it)
        {
            int mecAppId = it->first;
            if(maneuvringVehicle_ != nullptr && maneuvringVehicle_->getMecAppId() == mecAppId)
            {
                EV << "MECPlatooningControllerApp::handleControlTimer - is the maneuvering vehicle" << endl;
                if(it->second.isManouveringEnded)
                {
                    switchState(CRUISE);
                    finalizeJoinPlatoonRequest(maneuvringVehicleJoinMsg_, it->second.isManouveringEnded);
                }
            }
            double newAcceleration = it->second.acceleration;
            inet::L3Address precUeAddress = it->second.address;

            auto consumerApp = consumerAppEndpoint_.find(mecAppId);
            if (consumerApp == consumerAppEndpoint_.end())
            {
                throw cRuntimeError("MECPlatooningControllerApp::handleControlTimer - endpoint for MEC app %d not found.", mecAppId);
            }
            inet::Packet* pkt = new inet::Packet("PlatooningInfoPacket");

            auto cmd = inet::makeShared<PlatooningInfoPacket>();
            cmd->setType(PLATOON_CMD);
            cmd->setConsumerAppId(mecAppId); // used in case of it is sent to an other produceApp
            cmd->setNewAcceleration(newAcceleration);
            cmd->setChunkLength(inet::B( sizeof(double) + sizeof(int) + 1));
            cmd->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
            cmd->addTagIfAbsent<inet::PrecedingVehicleTag>()->setUeAddress(precUeAddress);
            pkt->insertAtBack(cmd);

            //send to the relative consumer App
            platooningConsumerAppsSocket_.sendTo(pkt, consumerApp->second.address , consumerApp->second.port);
            EV << "MECPlatooningControllerApp::handleLateralControllerTimer - send command to MECPlatoonConsumerApp [" << mecAppId << "]" << endl;
        }
        delete cmdList;
    }

        // re-schedule controller timer
        if (ctrlTimer->getPeriod() > 0.0 && !ctrlTimer->isScheduled())
            scheduleAt(simTime() + ctrlTimer->getPeriod(), ctrlTimer);
        else if(ctrlTimer->getPeriod() <= 0.0)
            stopTimer((PlatooningTimerType)ctrlTimer->getType());
}

void MECPlatooningControllerApp::handleUpdatePositionTimer(UpdatePositionTimer* posTimer)
{

    EV << "MECPlatooningControllerApp::handleUpdatePositionTimer - Request position updates for platoon with index " << controllerId_ << endl;

    // retrieve platoon controller and run the control algorithm
    std::map<int, std::set<inet::L3Address> > addressList = getUeAddressList();
    if(posTimer->getType() == PLATOON_LATERAL_UPDATE_POSITION_TIMER) // add the maneuvring vehicle
    {
        if(maneuvringVehicle_ == nullptr)
        {
            throw cRuntimeError("MECPlatooningControllerApp::handleUpdatePositionTimer - Error maneuvringVehicle_ is nullptr!");
        }
        EV << "MECPlatooningControllerApp::handleUpdatePositionTimer - Added maneuvring vehicle " << endl;
        addressList[maneuvringVehicle_->getProducerAppId()].insert(maneuvringVehicle_->getUeAddress());
    }
    for(const auto& produceApp : addressList)
    {
        requirePlatoonLocations(produceApp.first, produceApp.second);
    }

    // re-schedule controller timer
    if (posTimer->getPeriod() > 0.0 && !posTimer->isScheduled())
        scheduleAt(simTime() + posTimer->getPeriod(), posTimer);
    else if (posTimer->getPeriod() <= 0.0 )
        stopTimer((PlatooningTimerType)posTimer->getType());
}

void MECPlatooningControllerApp::requirePlatoonLocations(int producerAppId, const std::set<inet::L3Address>& ues)
{
    auto producerApp = producerAppEndpoint_.find(producerAppId);
    if(producerApp == producerAppEndpoint_.end())
    {
        throw cRuntimeError("MECPlatooningControllerApp::sendGetRequest(): producerAppId %d not found", producerAppId);
    }
    // get the Address of the UEs
    std::stringstream ueAddresses;
    EV << "MECPlatooningControllerApp::requirePlatoonLocations - " << ues.size() << endl;
    for( std::set<inet::L3Address>::iterator it = ues.begin(); it != ues.end(); ++it)
    {
        EV << "MECPlatooningControllerApp::requirePlatoonLocations - " << *it << endl;
        ueAddresses << "acr:" << *it;

        if(!sendBulk_)
        {
            //    // send GET request
            sendGetRequest(producerAppId, ueAddresses.str());
//            // insert the controller id in the relative requests queue
//            producerApp->second.controllerPendingRequests.push(controllerIndex);
            ueAddresses.str("");
            ueAddresses.clear();
        }
        else
        {

            if(next(it) != ues.end())
                ueAddresses << ",";
        }
    }

    if(sendBulk_)
    {
        // send GET request
        sendGetRequest(producerAppId, ueAddresses.str());

//        //insert the controller id in the relative requests queue
//        producerApp->second.controllerPendingRequests.push(controllerIndex);
    }
}

void MECPlatooningControllerApp::sendGetRequest(int producerAppId, const std::string& ues)
{

    auto producerApp = producerAppEndpoint_.find(producerAppId);
    if(producerApp == producerAppEndpoint_.end())
    {
        throw cRuntimeError("MECPlatooningControllerApp::sendGetRequest(): producerAppId %d not found", producerAppId);
    }

    TcpSocket *sock = producerApp->second.locationServiceSocket;

    //check if the ueAppAddress is specified
    if(sock->getState() == inet::TcpSocket::CONNECTED)
    {
        EV << "MECPlatooningControllerApp::sendGetRequest(): send request to the Location Service" << endl;
        std::stringstream uri;
        uri << "/example/location/v2/queries/users?address=" << ues;
        EV << "MECPlatooningControllerApp::requestLocation(): uri: "<< uri.str() << endl;
        std::string host = sock->getRemoteAddress().str()+":"+std::to_string(sock->getRemotePort());
        Http::sendGetRequest(sock, host.c_str(), uri.str().c_str());
    }
    else
    {
        EV << "MECPlatooningControllerApp::sendGetRequest(): Location Service not connected" << endl;
    }
}

void MECPlatooningControllerApp::established(int connId)
{
    EV <<"MECPlatooningControllerApp::established - connId ["<< connId << "]" << endl;


    if(connId == mp1Socket_->getSocketId())
    {
        EV << "MECPlatooningControllerApp::established - Mp1Socket"<< endl;

        // once the connection with the Service Registry has been established, obtain the
        // endPoint (address+port) of the Location Service
        const char *uri = "/example/mec_service_mgmt/v1/services?ser_name=LocationService";
        std::string host = mp1Socket_->getRemoteAddress().str()+":"+std::to_string(mp1Socket_->getRemotePort());

        Http::sendGetRequest(mp1Socket_, host.c_str(), uri);
    }
//    else if (connId == serviceSocket_->getSocketId())
//    {
//        EV << "MECPlatooningControllerApp::established - conection Id: "<< connId << endl;
//    }
//    else
//    {
//        throw cRuntimeError("MecAppBase::socketEstablished - Socket %d not recognized", connId);
//    }
}



bool MECPlatooningControllerApp::addPlatoonMember(int mecAppId, int producerAppId, inet::Coord position, inet::L3Address ueAddress)
{
    // assign correct positions in the platoon
    // we compare the current position of the new vehicle against the position of other members,
    // starting from the platoon leader. We compute the of coordinates difference between the new vehicle and
    // the current member in the loop. The position of the new vehicle is found as soon as we find that
    // the difference has not the same sign as the direction of the platoon
    bool found = false;
    std::list<int>::iterator pt = platoonPositions_.begin();
    for (; pt != platoonPositions_.end(); ++pt)
    {
        inet::Coord curMemberPos = membersInfo_.at(*pt).getPosition();
        inet::Coord diff = curMemberPos - position;

        if (diff.getSign().x != direction_.getSign().x || diff.getSign().y != direction_.getSign().y)
        {
            // insert the new vehicle in front of the current member
            platoonPositions_.insert(pt, mecAppId);
            found = true;
            break;
        }
        // else current member is in front of the new vehicle, keep scanning
    }
    if (!found)
        platoonPositions_.push_back(mecAppId);

    // add vehicle info to the relevant data structure
    PlatoonVehicleInfo newVehicleInfo;
    newVehicleInfo.setPosition(position);
    newVehicleInfo.setUeAddress(ueAddress);
    newVehicleInfo.setMecAppId(mecAppId);
    newVehicleInfo.setProducerAppId(producerAppId);
    membersInfo_[mecAppId] = newVehicleInfo;

    EV << "PlatoonControllerBase::addPlatoonMember - New member [" << mecAppId << "] added to platoon" << endl;
    return true;
}


std::map<int, std::set<inet::L3Address> > MECPlatooningControllerApp::getUeAddressList()
{
    std::map<int, std::set<inet::L3Address> > ueAddresses;
    PlatoonMembersInfo::const_iterator it = membersInfo_.begin();
    for(; it != membersInfo_.end(); ++it)
    {
        ueAddresses[it->second.getProducerAppId()].insert(it->second.getUeAddress());
    }
    EV << "PlatoonControllerBase::getUeAddressList "<< ueAddresses.size() << endl;

    return ueAddresses;
}

void MECPlatooningControllerApp::adjustPositions()
{
    /*
     * find the most recent timestamp
     * for every older timestamp compute the new position
     */
    double now = simTime().dbl()*1000;

    for(auto &vehicle: membersInfo_)
    {
        double deltaTs = (now - vehicle.second.getTimestamp())/1000; //to ms
        EV << "PlatoonControllerBase::adjustPositions() - deltaTs is " << deltaTs << endl;

        // TODO consider to adjust the position for every coord!
        EV << "PlatoonControllerBase::adjustPositions() - old position: " << vehicle.second.getPosition().str() << endl;
        double newPosition = vehicle.second.getPosition().x + vehicle.second.getSpeed() * deltaTs + 0.5 * vehicle.second.getAcceleration() * deltaTs * deltaTs; // TODO add acceleration part

        double newSpeed = vehicle.second.getSpeed() + vehicle.second.getAcceleration() * deltaTs;
        vehicle.second.setPositionX(newPosition);
        vehicle.second.setSpeed(newSpeed);
        vehicle.second.setTimestamp(now);

        EV << "PlatoonControllerBase::adjustPositions() - new position: " << vehicle.second.getPosition().str() << endl;
    }
}

std::vector<PlatoonVehicleInfo> MECPlatooningControllerApp::getPlatoonMembers()
{
    std::vector<PlatoonVehicleInfo> vehicles;
    for(auto& vec : membersInfo_)
    {
        vehicles.push_back(vec.second);
    }
    return vehicles;
}


void MECPlatooningControllerApp::updatePlatoonPositions(std::vector<UEInfo>* uesInfo)
{
    EV << "PlatoonControllerBase::updatePlatoonPositions - size " << uesInfo->size() << endl;
    auto it = uesInfo->begin();
    for(;it != uesInfo->end(); ++it)
    {
        PlatoonMembersInfo::iterator mit = membersInfo_.begin();
        for(; mit != membersInfo_.end(); ++mit)
        {
            if (mit->second.getUeAddress() == it->address)
            {
                mit->second.setLastPosition(mit->second.getPosition());
                mit->second.setLastSpeed(mit->second.getSpeed());
                mit->second.setLastTimestamp(mit->second.getTimestamp());

                mit->second.setPosition(it->position);
                mit->second.setSpeed(it->speed);
                mit->second.setTimestamp(it->timestamp);

                break;
            }
        }

        if(mit  == membersInfo_.end())
        {
            // check the manuevring
            if(state_ == MANOEUVRE && maneuvringVehicle_ != nullptr)
            {
                EV << "PlatoonControllerBase::updatePlatoonPositions - Updating maneuvring vehicle " << endl;
                maneuvringVehicle_->setPosition(it->position);
                maneuvringVehicle_->setSpeed(it->speed);
                maneuvringVehicle_->setTimestamp(it->timestamp);

            }
            else if(state_ == CRUISE && maneuvringVehicle_ != nullptr)
            {
                // is the case where the pending car is waiting for being inserted in the manoeuvre
                // because its position is not known
                EV << "PlatoonControllerBase::updatePlatoonPositions - Updating waiting vehicle " << endl;
                maneuvringVehicle_->setPosition(it->position);
                maneuvringVehicle_->setSpeed(it->speed);
                maneuvringVehicle_->setTimestamp(it->timestamp);

                // check the distance now!
                checkWaitingCarPosition();
            }

        }

        EV << "UE info:\naddress ["<< it->address.str() << "]\n" <<
                "timestamp: " << it->timestamp << "\n" <<
                "coords: ("<<it->position.x<<"," << it->position.y<<"," << it->position.z<<")\n"<<
                "speed: " << it->speed << " mps" << endl;
    }
}

void MECPlatooningControllerApp::checkWaitingCarPosition()
{
    PlatoonVehicleInfo* lastCar = getLastPlatoonCar();
    if(lastCar != nullptr && maneuvringVehicle_->getPosition().distance(lastCar->getPosition()) > minimumDistanceForManoeuvre_)
    {
        EV << "MECPlatooningControllerApp::checkWaitingCarPosition - The car relative to queue join request is " <<  maneuvringVehicle_->getPosition().distance(lastCar->getPosition()) << " meters  far from the platoon!! The request from MECPlatoonConsumerApp with id [" << maneuvringVehicle_->getMecAppId() << "] is rejected!" << endl;
        finalizeJoinPlatoonRequest(maneuvringVehicleJoinMsg_, false);
    }
    else
    {
        EV << "MECPlatooningControllerApp::checkWaitingCarPosition - The car relative to queue join request is " <<  maneuvringVehicle_->getPosition().distance(lastCar->getPosition()) << " meters  near the platoon!! The request from MECPlatoonConsumerApp with id [" << maneuvringVehicle_->getMecAppId() << "] is accepted!" << endl;
        switchState(MANOEUVRE);
        sendManoeuvreNotification(maneuvringVehicleJoinMsg_);
    }
}


PlatoonVehicleInfo* MECPlatooningControllerApp::getPlatoonLeader()
{
    if(platoonPositions_.size() == 0)
        return nullptr;
    int leader = platoonPositions_.front();
    return &membersInfo_.at(leader);
}
PlatoonVehicleInfo* MECPlatooningControllerApp::getLastPlatoonCar()
{
    if(platoonPositions_.size() == 0)
        return nullptr;
    int back = platoonPositions_.back();
    return &membersInfo_.at(back);
}


