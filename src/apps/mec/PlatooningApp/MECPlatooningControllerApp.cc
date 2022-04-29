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

}

MECPlatooningControllerApp::~MECPlatooningControllerApp()
{
    delete platoonSelection_;
    delete longitudinalController_;
    delete transitionalController_;
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

////   cModule* vi = this->getParentModule();
//   cModule* localProducerAppModule = this->getModuleByPath("^.bgApp[0]");
//   MECPlatooningProducerApp* localProducerApp = check_and_cast<MECPlatooningProducerApp *>(localProducerAppModule);
//
//   cValueArray *producerAppEndpoints = check_and_cast<cValueArray *>(localProducerApp->par("federatedProducerApps").objectValue());
//   for(int i = 0; i < producerAppEndpoints->size(); ++i)
//   {
//       cValueMap *entry = check_and_cast<cValueMap *>(producerAppEndpoints->get(i).objectValue());
//       ProducerAppInfo producerAppinfo;
//       producerAppinfo.port = entry->get("port");
//       producerAppinfo.address = L3AddressResolver().resolve(entry->get("address").stringValue());
//
//       producerAppinfo.locationServicePort = entry->get("locationServicePort");
//       producerAppinfo.locationServiceAddress = L3AddressResolver().resolve(entry->get("locationServiceAddress").stringValue());
//       producerAppinfo.locationServiceSocket = addNewSocket();
//
//       int id = entry->get("id");
//
//       // TODO create socket
//       producerAppEndpoint_[id] = producerAppinfo;
//
//       EV <<"MECPlatooningControllerApp::initialize - id: " << id <<" address: "  << producerAppinfo.address.str() <<" port " << producerAppinfo.port << endl;
//   }

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

            if (platooningPkt->getType() == REGISTRATION_REQUEST)
            {
                handleRegistrationRequest(msg);
            }
            else if(platooningPkt->getType() == JOIN_REQUEST)
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
            case PLATOON_CONTROL_TIMER:
            {
                ControlTimer* ctrlTimer = check_and_cast<ControlTimer*>(timer);
                handleControlTimer(ctrlTimer);
//                EV << "TEST" << endl;
////                // emit stats about the difference of the updates
////                if(received_ == 2)
////                {
////                    EV << "DIFFERENZA: " << second-first << endl;
//                    if(simTime() > 10)
//                    {
//                        simtime_t diff;
//                        if(producerAppEndpoint_[0].lastResponse > producerAppEndpoint_[1].lastResponse)
//                            diff = producerAppEndpoint_[0].lastResponse - producerAppEndpoint_[1].lastResponse;
//                        else
//                            diff = producerAppEndpoint_[1].lastResponse - producerAppEndpoint_[0].lastResponse;
//
//                        emit(updatesDifferences_, diff);
//                    }
//                    received_ = 0;
//                }
//                else
//                {
//                    //emit(updatesDifferences_, 0);
//                    received_ = 0;
//                }

                break;
            }
            case PLATOON_UPDATE_POSITION_TIMER:
            {
                UpdatePositionTimer* posTimer = check_and_cast<UpdatePositionTimer*>(timer);
                handleUpdatePositionTimer(posTimer);
                break;
            }
            default: throw cRuntimeError("MECPlatooningControllerApp::handleSelfMessage - unrecognized timer type %d", timer->getType());
        }
    }
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
                        EV << "UE INFO " << ue << endl;
                        if(ue.is_string())
                        {
                            continue;
                        }
                        UEInfo ueInfo;
                        std::string address = ue["address"];
                        EV << "MECPlatooningControllerApp::handleServiceMessage: address [" << address << "]" << endl;
                        EV << "AA " << address.substr(address.find(":")+1, address.size()) << endl;
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
              longitudinalController_->updatePlatoonPositions(&uesInfo);

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

   mp1Socket_ = addNewSocket();
   connect(mp1Socket_, mp1Address, mp1Port);


   // plus the local one

    controllerId_ = confMsg->getControllerId();

    // instantiate controllers
    double controlPeriod = confMsg->getControlPeriod();
    double updatePositionPeriod = confMsg->getUpdatePositionPeriod();

    adjustPosition_  = confMsg->getAdjustPosition();
    sendBulk_ = confMsg->getSendBulk();

    if(strcmp( confMsg->getLongitudinalController() , "Rajamani") == 0 )
    {
        longitudinalController_ =   new RajamaniPlatoonController(this, controllerId_, controlPeriod, updatePositionPeriod);
        EV << "Controller is  " << par("longitudinalController").str() << endl;

    }
    else if(strcmp( confMsg->getLongitudinalController(), "Safe") == 0 )
        longitudinalController_ =   new SafePlatoonController(this, controllerId_, controlPeriod, updatePositionPeriod);

    longitudinalController_->setDirection(confMsg->getDirection());

    transitionalController_ = nullptr;

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
    if(isConfigured_ == false)
        throw cRuntimeError("CONF PACKER NOT ARRIVED");
    bool success = longitudinalController_->addPlatoonMember(consumerAppId, producerAppId,  position, carIpAddress);

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

void MECPlatooningControllerApp::handleRegistrationRequest(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    L3Address mecAppAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
    int mecAppPort = packet->getTag<L4PortInd>()->getSrcPort();

    auto registrationReq = packet->removeAtFront<PlatooningRegistrationPacket>();
    int mecAppId = registrationReq->getConsumerAppId();

    EV << "MECPlatooningControllerApp::handleRegistrationRequest - Received registration request from MEC app " << mecAppId << " [" << mecAppAddress.str() << ":" << mecAppPort << "]" << endl;

    // save info in some data structure
    ConsumerAppInfo info;
    info.address = mecAppAddress;
    info.port = mecAppPort;
    info.producerAppId = -1; //for now, after the join it will be set

    consumerAppEndpoint_[mecAppId] = info;

    // send (n)ack to the MEC app
    inet::Packet* respPacket = new Packet(packet->getName());
    registrationReq->setType(REGISTRATION_RESPONSE);
    respPacket->insertAtBack(registrationReq);
    respPacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    platooningConsumerAppsSocket_.sendTo(respPacket, mecAppAddress, mecAppPort);

    EV << "MECPlatooningControllerApp::handleRegistrationRequest - Sending response to MEC app " << mecAppId << " [" << mecAppAddress.str() << ":" << mecAppPort << "]" << endl;

    delete packet;
}

void MECPlatooningControllerApp::handleJoinPlatoonRequest(cMessage* msg)
{
    /*
     * We should check where the car is. For now, we should only add the car to the platoon and start the CRUISE phase
     */
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    L3Address consumerAppAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
    int consumerAppPort = packet->getTag<L4PortInd>()->getSrcPort();
    auto req = packet->removeAtFront<PlatooningJoinPacket>();

    int consumerAppId = req->getConsumerAppId();
    int producerAppId = req->getProducerAppId();
    L3Address carIpAddress = req->getUeAddress();
    Coord position = req->getLastPosition();

    // suppose all checks are ok, add the car to the platoon

    bool success = addMember(consumerAppId, carIpAddress, position, producerAppId);

    req->setType(JOIN_RESPONSE);

    if(success)
    {
        req->setColor("green");
        // save info in some data structure
        ConsumerAppInfo info;
        info.address = consumerAppAddress;
        info.port = consumerAppPort;
        info.producerAppId = producerAppId; //for now, after the join it will be set

        consumerAppEndpoint_[consumerAppId] = info;

        // add CONTROLLER_NOTIFICATION TO THE PRODUCER APP
        inet::Packet* pkt = new inet::Packet("PlatooningControllerNotificationPacket");
        auto notification = inet::makeShared<PlatooningControllerNotificationPacket>();

        notification->setType(CONTROLLER_NOTIFICATION);
        notification->setNotificationType(NEW_MEMBER);
        notification->setConsumerAppId(consumerAppId);

        notification->setControllerId(controllerId_);
        notification->setProducerAppId(producerAppId);

        notification->setVehiclesOrder(longitudinalController_->getPlatoonMembers());
        notification->setHeartbeatTimeStamp(simTime());


        notification->setChunkLength(inet::B(20 +  sizeof(members)));
        notification->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
        pkt->insertAtBack(notification);

        platooningProducerAppSocket_.sendTo(pkt, platooningProducerAppEndpoint_.address, platooningProducerAppEndpoint_.port);

        EV << "MECPlatooningControllerApp::handleJoinPlatoonRequest - MECPlatooningConsumerApp with id [" << consumerAppId << "] added to platoon" << endl;
    }

    req->setResponse(success);

    inet::Packet* responsePacket = new Packet(packet->getName());
    responsePacket->insertAtBack(req);
    // TODO look for endpoint in the structure?
    platooningConsumerAppsSocket_.sendTo(responsePacket, consumerAppAddress, consumerAppPort);
    EV << "MECPlatooningControllerApp::sendJoinPlatoonRequest - Join response sent to the UE" << endl;


    delete packet;
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
    EV << "MECPlatooningControllerApp::sendJoinPlatoonRequest - Join response sent to the UE" << endl;

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
            // add CONTROLLER_NOTIFICATION TO THE PRODUCER APP
            inet::Packet* pkt = new inet::Packet("PlatooningControllerNotificationPacket");
            auto notification = inet::makeShared<PlatooningControllerNotificationPacket>();

            notification->setType(CONTROLLER_NOTIFICATION);
            notification->setNotificationType(REMOVED_MEMBER);
            notification->setConsumerAppId(leaveReq->getConsumerAppId());

            notification->setControllerId(controllerId_);
            notification->setProducerAppId(leaveReq->getProducerAppId());

            auto members = longitudinalController_->getPlatoonMembers();
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
    EV << "MECPlatooningControllerApp::removePlatoonMember - Received leave request from MEC app " << mecAppId << endl;

   // remove the member from the identified platoonController
   bool success = longitudinalController_->removePlatoonMember(mecAppId);

   return success;
}

void MECPlatooningControllerApp::startTimer(cMessage* msg, double timeOffset)
{
    if (timeOffset < 0.0)
        throw cRuntimeError("MECPlatooningControllerApp::startTimer - time offset not valid[%f]", timeOffset);

    PlatooningTimer* timer = check_and_cast<PlatooningTimer*>(msg);

    if (timer->getType() == PLATOON_CONTROL_TIMER)
    {
        // if a control timer was already set for this controller, cancel it
        if(controlTimer_ != nullptr && controlTimer_->isScheduled())
            cancelAndDelete(controlTimer_);

        EV << "MECPlatooningControllerApp::startTimer() - New ControlTimer set for controller " << controllerId_ << " in " << timeOffset << "s" << endl;

        controlTimer_ = check_and_cast<ControlTimer*>(msg);
        //schedule timer
        scheduleAt(simTime() + timeOffset, controlTimer_);
    }
    else if (timer->getType() == PLATOON_UPDATE_POSITION_TIMER)
    {
        // if a position timer was already set for this controller, cancel it
        if(updatePositionTimer_ != nullptr && updatePositionTimer_->isScheduled())
            cancelAndDelete(updatePositionTimer_);

        EV << "MECPlatooningControllerApp::startTimer() - New UpdatePositionTimer set for controller " << controllerId_ << " in " << timeOffset << "s" << endl;

        updatePositionTimer_ = check_and_cast<UpdatePositionTimer*>(msg);
        //schedule timer
        scheduleAt(simTime() + timeOffset, updatePositionTimer_);
    }

}

void MECPlatooningControllerApp::stopTimer(PlatooningTimerType timerType)
{
    if (timerType == PLATOON_CONTROL_TIMER)
    {
        if (controlTimer_ == nullptr || !controlTimer_->isScheduled())
            throw cRuntimeError("MECPlatooningControllerApp::stopTimer - no active ControlTimer with index %d", controllerId_);

        cancelAndDelete(controlTimer_);
        EV << "MECPlatooningControllerApp::stopTimer - ControlTimer for controller " << controllerId_ << "has been stopped" << endl;
    }
    else if (timerType == PLATOON_UPDATE_POSITION_TIMER)
    {
        if (updatePositionTimer_ == nullptr || !updatePositionTimer_->isScheduled())
            throw cRuntimeError("MECPlatooningControllerApp::stopTimer - no active UpdatePositionTimer with index %d", controllerId_);

        cancelAndDelete(updatePositionTimer_);
        EV << "MECPlatooningControllerApp::stopTimer - UpdatePositionTimer for controller " << controllerId_ << "has been stopped" << endl;
    }
    else
        throw cRuntimeError("MECPlatooningControllerApp::stopTimer - unrecognized timer type %d", timerType);
}

void MECPlatooningControllerApp::handleControlTimer(ControlTimer* ctrlTimer)
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
//            bool isRemote = false;
//            int remoteProducerAppId;

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

void MECPlatooningControllerApp::handleUpdatePositionTimer(UpdatePositionTimer* posTimer)
{

    EV << "MECPlatooningControllerApp::handleUpdatePositionTimer - Request position updates for platoon with index " << controllerId_ << endl;

    // retrieve platoon controller and run the control algorithm
    std::map<int, std::set<inet::L3Address> > addressList = longitudinalController_->getUeAddressList();

    for(const auto& produceApp : addressList)
    {
        requirePlatoonLocations(produceApp.first, produceApp.second);
    }

    // re-schedule controller timer
    if (posTimer->getPeriod() > 0.0)
        scheduleAt(simTime() + posTimer->getPeriod(), posTimer);
    else
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





