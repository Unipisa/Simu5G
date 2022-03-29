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

#include "apps/mec/PlatooningApp/MECPlatooningProducerApp.h"
#include "apps/mec/PlatooningApp/platoonSelection/PlatoonSelectionSample.h"
#include "apps/mec/PlatooningApp/platoonController/SafePlatoonController.h"
#include "apps/mec/PlatooningApp/platoonController/RajamaniPlatoonController.h"
#include "apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket_Types.h"
#include "apps/mec/PlatooningApp/packets/CurrentPlatoonRequestTimer_m.h"
#include "apps/mec/PlatooningApp/packets/tags/PlatooningPacketTags_m.h"

#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/utils/httpUtils/json.hpp"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.h"

#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/Packet_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"


#include <fstream>

Define_Module(MECPlatooningProducerApp);

using namespace inet;
using namespace omnetpp;

MECPlatooningProducerApp::MECPlatooningProducerApp(): MecAppBase()
{
    platoonSelection_ = nullptr;
    requestCounter_ = 0;
    currentAvailablePlatoons_.clear();
}

MECPlatooningProducerApp::~MECPlatooningProducerApp()
{
    delete platoonSelection_;
    cancelAndDelete(currentPlatoonRequestTimer_);
}


void MECPlatooningProducerApp::initialize(int stage)
{
    MecAppBase::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    // create a new selection algorithm
    // TODO read from parameter
    platoonSelection_ = new PlatoonSelectionSample();

    // set UDP Socket
    platooningConsumerAppsPort = par("platooningConsumerAppsPort");
    platooningConsumerAppsSocket.setOutputGate(gate("socketOut"));
    platooningConsumerAppsSocket.bind(platooningConsumerAppsPort);

    platooningProducerAppsPort = par("platooningProducerAppsPort");
    platooningProducerAppsSocket_.setOutputGate(gate("socketOut"));
    platooningProducerAppsSocket_.bind(platooningProducerAppsPort);

    producerAppId_ = par("producerAppId");
    nextControllerIndex_ = 1000* (1 + producerAppId_);


    adjustPosition_  = par("adjustPosition").boolValue();
    sendBulk_ = par("sendBulk").boolValue();

    received_ = 0;

    updatesDifferences_ = registerSignal("updateDiffs");


    EV << "MECPlatooningProducerApp::initialize - MEC application "<< getClassName() << " with mecAppId["<< mecAppId << "] has started!" << endl;

   // connect with the service registry
   EV << "MECPlatooningProducerApp::initialize - Initialize connection with the Service Registry via Mp1" << endl;

   retrievePlatoonsInfoDuration_ = 0.500; //TODO add parameter!

   // retrieve global PlatoonProducerApp endpoint
   cValueArray *producerAppEndpoints = check_and_cast<cValueArray *>(par("federatedProducerApps").objectValue());
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

       EV <<"MECPlatooningProducerApp::initialize - id: " << id <<" address: "  << producerAppinfo.address.str() <<" port " << producerAppinfo.port << endl;
   }

    currentPlatoonRequestTimer_ = new CurrentPlatoonRequestTimer();
    currentPlatoonRequestTimer_->setPeriod(retrievePlatoonsInfoDuration_);

    mp1Socket_ = addNewSocket();

    // Add local producerApp
    ProducerAppInfo localProducerAppInfo;
    localProducerAppInfo.port = -1; // it is not used
    localProducerAppInfo.address = L3Address(); // it is not used

    localProducerAppInfo.locationServicePort = -1;    // set with service registry
    localProducerAppInfo.locationServiceAddress = L3Address(); // set with service registry
    localProducerAppInfo.locationServiceSocket = addNewSocket();
    producerAppEndpoint_[producerAppId_] = localProducerAppInfo;

   connect(mp1Socket_, mp1Address, mp1Port);
}

void MECPlatooningProducerApp::handleMessage(cMessage *msg)
{
    if (!msg->isSelfMessage())
    {
        if(platooningConsumerAppsSocket.belongsToSocket(msg))
        {
            // determine its source address/port
            auto pk = check_and_cast<Packet *>(msg);
            ueAppAddress = pk->getTag<L3AddressInd>()->getSrcAddress();
            ueAppPort = pk->getTag<L4PortInd>()->getSrcPort();

            auto platooningPkt = pk->peekAtFront<PlatooningAppPacket>();
            EV << "MECPlatooningProducerApp::handleMessage(): messageType " << platooningPkt->getType()  << endl;

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
                throw cRuntimeError("MECPlatooningProducerApp::handleUeMessage - packet from platoonConsumerApp not recognized");
            }

            return;
        }
        else if(platooningProducerAppsSocket_.belongsToSocket(msg))
        {
            // determine its source address/port
            auto pk = check_and_cast<Packet *>(msg);
            auto platooningPkt = pk->peekAtFront<PlatooningAppPacket>();
            EV << "MECPlatooningProducerApp::handleMessage(): messageType " << platooningPkt->getType()  << endl;
            if (platooningPkt->getType() == AVAILABLE_PLATOONS_REQUEST)
            {
                handleAvailablePlatoonsRequest(msg);
            }
            else if (platooningPkt->getType() == AVAILABLE_PLATOONS_RESPONSE)
            {
                handleAvailablePlatoonsResponse(msg);
            }
            else if (platooningPkt->getType() == ADD_MEMBER_REQUEST)
            {
                handleAddMemberRequest(msg);
            }
            else if (platooningPkt->getType() == ADD_MEMBER_RESPONSE)
            {
                handleAddMemberResponse(msg);
            }
            else if (platooningPkt->getType() == REMOVE_MEMBER_REQUEST)
            {
                handleRemoveMemberRequest(msg);
            }
            else if (platooningPkt->getType() == REMOVE_MEMBER_RESPONSE)
            {
                handleRemoveMemberResponse(msg);
            }
            else if (platooningPkt->getType() == PLATOON_CMD)
            {
                handlePlatoonCommand(msg);
            }
            else
            {
                throw cRuntimeError("MECPlatooningProducerApp::handleUeMessage - packet from platoonProviderApp not recognized");
            }
            return;
        }

    }
    MecAppBase::handleMessage(msg);
}

void MECPlatooningProducerApp::finish()
{
    MecAppBase::finish();
    EV << "MECPlatooningProducerApp::finish()" << endl;
    if(gate("socketOut")->isConnected())
    {

    }
}

void MECPlatooningProducerApp::handleSelfMessage(cMessage *msg)
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
                EV << "TEST" << endl;
//                // emit stats about the difference of the updates
//                if(received_ == 2)
//                {
//                    EV << "DIFFERENZA: " << second-first << endl;
                    if(simTime() > 10)
                    {
                        simtime_t diff;
                        if(producerAppEndpoint_[0].lastResponse > producerAppEndpoint_[1].lastResponse)
                            diff = producerAppEndpoint_[0].lastResponse - producerAppEndpoint_[1].lastResponse;
                        else
                            diff = producerAppEndpoint_[1].lastResponse - producerAppEndpoint_[0].lastResponse;

                        emit(updatesDifferences_, diff);
                    }
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
            default: throw cRuntimeError("MECPlatooningProducerApp::handleSelfMessage - unrecognized timer type %d", timer->getType());
        }
    }
}

void MECPlatooningProducerApp::handleHttpMessage(int connId)
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

void MECPlatooningProducerApp::handleMp1Message(int connId)
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

                      // once we obtained the endpoint of the Location Service, establish a connection with it
                      auto localProducerApp = producerAppEndpoint_.find(producerAppId_);
                      localProducerApp->second.locationServiceAddress = locationServiceAddress_;
                      localProducerApp->second.locationServicePort = locationServicePort_;

//                      connect(localProducerApp->second.locationServiceSocket , locationServiceAddress_, locationServicePort_);
                      // connect to all the federated location services
                      for(auto const& producerApp : producerAppEndpoint_)
                      {
                          EV << "MECPlatooningProducerApp::handleMp1Message(): connecting to Location Service of producer app with ID: "<< producerApp.first <<
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

void MECPlatooningProducerApp::handleServiceMessage(int connId)
{
    HttpBaseMessage* httpMessage = nullptr;
    int controllerIndex;
    ControllerMap::iterator it;

    for(auto& producerApp : producerAppEndpoint_)
    {
        if(producerApp.second.locationServiceSocket->getSocketId() == connId)
        {
            /*
             * bad code, just for quick result
             * store the last time you received the update
             *
             */
            producerApp.second.lastResponse = simTime();

            HttpMessageStatus* msgStatus = (HttpMessageStatus*)producerApp.second.locationServiceSocket->getUserData();
            httpMessage = (HttpBaseMessage*)msgStatus->httpMessageQueue.front();
            // If the request is associated to a controller no more available, just discard
             controllerIndex = producerApp.second.controllerPendingRequests.front();
             producerApp.second.controllerPendingRequests.pop();
             it = platoonControllers_.find(controllerIndex);
             if(it == platoonControllers_.end())
             {
                 EV << "MECPlatooningProducerApp::handleServiceMessage - platoon controller with index ["<< controllerIndex <<"] no more available"<< endl;
                 return;
             }
            break;
        }
    }

    if(httpMessage == nullptr)
    {
        throw cRuntimeError("MECPlatooningProducerApp::handleServiceMessage() - httpMessage is null!");
    }

    if(httpMessage->getType() == RESPONSE)
    {
        HttpResponseMessage *rspMsg = dynamic_cast<HttpResponseMessage*>(httpMessage);
        if(rspMsg->getCode() == 200) // in response to a successful GET request
        {
            nlohmann::json jsonBody;
            EV << "MECPlatooningProducerApp::handleServiceMessage - response 200 " << endl;

            received_++;

            try
            {
              jsonBody = nlohmann::json::parse(rspMsg->getBody()); // get the JSON structure
              std::vector<UEInfo> uesInfo;
//              EV << "BODY" <<jsonBody.dump(2) << endl;
              //get the positions of all ues
              if(!jsonBody.contains("userInfo"))
              {
                  EV << "MECPlatooningProducerApp::handleServiceMessage: ues not found" << endl;
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
                        EV << "MECPlatooningProducerApp::handleServiceMessage: address [" << address << "]" << endl;
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

              EV << "MECPlatooningProducerApp::handleServiceMessage - update the platoon positions for controller ["<< controllerIndex << "]" << endl;

              // notify the controller
              PlatoonControllerBase* platoonController = it->second;
              platoonController->updatePlatoonPositions(&uesInfo);

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
            EV << "MECPlatooningProducerApp::handleServiceMessage - response with HTTP code:  " << rspMsg->getCode() << endl;
        }
    }
    // it is a request. It should not arrive, for now (think about sub-not)
    else
    {
        HttpRequestMessage *reqMsg = dynamic_cast<HttpRequestMessage*>(httpMessage);
        EV << "MECPlatooningProducerApp::handleServiceMessage - response with HTTP code:  " << reqMsg->getMethod() << " discarded"  << endl;
    }
}

void MECPlatooningProducerApp::handleAddMemberRequest(cMessage *msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    L3Address producerAppAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
    int producerAppPort = packet->getTag<L4PortInd>()->getSrcPort();
    auto req = packet->removeAtFront<PlatooningAddMemberPacket>();
    int index = req->getPlatoonIndex();
    int consumerAppId = req->getMecAppId();
    int producerAppId = req->getProducerAppId();
    inet::Coord position = req->getPosition();


    PlatoonControllerBase* platoonController;
    bool success;
    if (platoonControllers_.find(index) != platoonControllers_.end())
    {
        platoonController = platoonControllers_[index];
        success = platoonController->addPlatoonMember(consumerAppId, producerAppId,  position, req->getUeAddress());
    }
    else
    {
        // if no active platoon controller can be used, create one
//        platoonController = new RajamaniPlatoonController(this, index, 0.1, 0.1); // TODO select the controller and set periods
//        platoonController->setDirection(direction);
//        platoonControllers_[index] = platoonController;
//        EV << "MECPlatooningProducerApp::sendJoinPlatoonRequest - MEC App " << mecAppId << " added to new platoon " << index << " - dir[" << direction << "]" << endl;
        success = false;
    }

    if(success)
    {
        EV << "MECPlatooningProducerApp::sendJoinPlatoonRequest - MEC App " << consumerAppId << " added to platoon " << index << endl;
        remoteConsumerAppToProducerApp_[consumerAppId] = producerAppId;
    }
    else
    {
        EV << "MECPlatooningProducerApp::sendJoinPlatoonRequest - MEC App " << consumerAppId << "is not added to any platoon" << endl;

    }

    req->setType(ADD_MEMBER_RESPONSE);
    req->setResponse(success);
    req->setProducerAppId(producerAppId_);

    inet::Packet* responsePacket = new Packet(packet->getName());
    responsePacket->insertAtBack(req);

    // TODO look for endpoint in the structure?
    platooningProducerAppsSocket_.sendTo(responsePacket, producerAppAddress, producerAppPort);

    delete packet;
}

void MECPlatooningProducerApp::handleAddMemberResponse(cMessage *msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto res = packet->removeAtFront<PlatooningAddMemberPacket>();
    bool success = res->getResponse();

    auto endpoint = consumerAppEndpoint_.find(res->getMecAppId());
    if(endpoint == consumerAppEndpoint_.end())
    {
        throw cRuntimeError("MECPlatooningProducerApp::handleAddMemberResponse - mecAppId %d not present!", mecAppId);
    }

    auto joinRes = inet::makeShared<PlatooningJoinPacket>();
    joinRes->setType(JOIN_RESPONSE);
    if (success)
    {
        endpoint->second.producerAppId = res->getProducerAppId();
       // accept the request and send ACK
        joinRes->setResponse(true);
        joinRes->setControllerIndex(res->getPlatoonIndex());
        joinRes->setColor("green");
    }
    else
    {
        joinRes->setResponse(false);
    }

    inet::Packet* responsePacket = new Packet("PlatooningJoinPacket");

    int chunkLen = sizeof(getId())+sizeof(platooningProducerAppsPort);
    joinRes->setChunkLength(inet::B(chunkLen));
    responsePacket->insertAtBack(joinRes);
    responsePacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    // send response to the UE's MEC app
    platooningConsumerAppsSocket.sendTo(responsePacket, endpoint->second.address, endpoint->second.port);
    EV << "MECPlatooningProducerApp::handleAddMemberResponse - Join response sent to the consumer app with id[" << res->getMecAppId() << "]" << endl;
    delete packet;
}

void MECPlatooningProducerApp::handleRemoveMemberRequest(cMessage *msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    L3Address producerAppAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
    int producerAppPort = packet->getTag<L4PortInd>()->getSrcPort();
    auto req = packet->removeAtFront<PlatooningLeavePacket>();

    int mecAppId = req->getMecAppId();
    int controllerIndex = req->getControllerIndex();

    bool success = removePlatoonMember(controllerIndex, mecAppId);

    // TODO remove?
    if(success)
        remoteConsumerAppToProducerApp_.erase(mecAppId);

    req->setResponse(success);
    req->setType(REMOVE_MEMBER_RESPONSE);

    inet::Packet* responsePacket = new Packet ("PlatooningLeavePacket");
    responsePacket->insertAtBack(req);
    responsePacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    // send response to the UE's consumer MEC app or to the producer MEC app
    platooningProducerAppsSocket_.sendTo(responsePacket, producerAppAddress, producerAppPort);

    delete packet;
}

void MECPlatooningProducerApp::handleRemoveMemberResponse(cMessage *msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
//    L3Address producerAppAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
//    int producerAppPort = packet->getTag<L4PortInd>()->getSrcPort();
    auto res = packet->removeAtFront<PlatooningLeavePacket>();

    auto endpoint = consumerAppEndpoint_.find(res->getMecAppId());
    if(endpoint == consumerAppEndpoint_.end())
    {
        throw cRuntimeError("MECPlatooningProducerApp::handleAddMemberResponse - mecAppId %d not present!", mecAppId);
    }

    inet::Packet* responsePacket = new Packet ("PlatooningLeavePacket");
    res->setType(LEAVE_RESPONSE);
    bool success = res->getResponse();
    if (success)
    {
       // accept the request and send ACK
        res->setResponse(true);
    }
    else
    {
        res->setResponse(false);
    }

    int chunkLen = sizeof(getId())+sizeof(platooningProducerAppsPort);
    res->setChunkLength(inet::B(chunkLen));
    responsePacket->insertAtBack(res);
    responsePacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    // send response to the UE's consumer MEC app
    platooningConsumerAppsSocket.sendTo(responsePacket, endpoint->second.address, endpoint->second.port);
    EV << "MECPlatooningProducerApp::handleLeavePlatoonRequest() - Leave response sent to the UE" << endl;

    delete packet;
}

void MECPlatooningProducerApp::handleRegistrationRequest(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    L3Address mecAppAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
    int mecAppPort = packet->getTag<L4PortInd>()->getSrcPort();

    auto registrationReq = packet->removeAtFront<PlatooningRegistrationPacket>();
    int mecAppId = registrationReq->getMecAppId();

    EV << "MECPlatooningProducerApp::handleRegistrationRequest - Received registration request from MEC app " << mecAppId << " [" << mecAppAddress.str() << ":" << mecAppPort << "]" << endl;

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
    platooningConsumerAppsSocket.sendTo(respPacket, mecAppAddress, mecAppPort);

    EV << "MECPlatooningProducerApp::handleRegistrationRequest - Sending response to MEC app " << mecAppId << " [" << mecAppAddress.str() << ":" << mecAppPort << "]" << endl;

    delete packet;
}

void MECPlatooningProducerApp::handleJoinPlatoonRequest(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    // do not remove, just peek it. only for the response it will be removed
    auto joinReq = packet->peekAtFront<PlatooningJoinPacket>();
    int mecAppId = joinReq->getMecAppId();
    inet::Coord position = joinReq->getLastPosition();
    inet::Coord direction = joinReq->getDirection();

    EV << "MECPlatooningProducerApp::sendJoinPlatoonRequest - Received join request from MEC App " << mecAppId << endl;

    int selectedPlatoon = joinReq->getControllerIndex();
    bool isSelectedLocal = false;

    // if the user requested for a specific platoon and the it is already present in the local platoons, do not require
    // platoons to the federated producer mec app
    if(selectedPlatoon != -1)
    {
        if(platoonControllers_.find(selectedPlatoon) != platoonControllers_.end())
        {
            EV << "MECPlatooningProducerApp::sendJoinPlatoonRequest - The requested platoon it id "<<  selectedPlatoon << " is already present in the local platoons" << endl;
            isSelectedLocal = true;
        }
    }

    // ask for the available platoons to the federation (if any)
    if(producerAppEndpoint_.size() > 1 && !isSelectedLocal)
    {
        if(currentPlatoonRequestTimer_->isScheduled())
        {
            EV << "MECPlatooningProducerApp::handleJoinPlatoonRequest - a request for all the platoons is already issued. Putting this request in the waiting queue"<< endl;
        }
        else
        {
            EV << "MECPlatooningProducerApp::handleJoinPlatoonRequest - available platoon request issued to all the federated platooningProducerApps. Putting this request in the waiting queue"<< endl;
            currentPlatoonRequestTimer_->setExpectedResponses(producerAppEndpoint_.size() - 1); // do not count the local producerApp
            currentPlatoonRequestTimer_->setReceivedResponses(0);
            currentPlatoonRequestTimer_->setPendingResponses(producerAppEndpoint_.size() - 1); // do not count the local producerApp
            currentPlatoonRequestTimer_->setRequestId(requestCounter_);

            //send request to all app
            for(auto const& producerApp : producerAppEndpoint_)
            {
                if(producerApp.first == producerAppId_)
                    continue;
                inet::Packet* pkt = new inet::Packet("PlatooningAvailablePlatoonsPacket");
                auto platReq = inet::makeShared<PlatooningAvailablePlatoonsPacket>();
                platReq->setType(AVAILABLE_PLATOONS_REQUEST);
                platReq->setRequestId(requestCounter_);
                platReq->setProducerAppId(producerAppId_);

                int chunkLen = sizeof(getId())+sizeof(platooningProducerAppsPort);
                platReq->setChunkLength(inet::B(chunkLen));
                platReq->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
                pkt->insertAtBack(platReq);

                platooningProducerAppsSocket_.sendTo(pkt, producerApp.second.address, producerApp.second.port);
            }
            requestCounter_++;
            scheduleAt(simTime()+currentPlatoonRequestTimer_->getPeriod(), currentPlatoonRequestTimer_);
        }
        consumerAppRequests_.insert(msg);
        return;
    }
    // choose a platoon upon of the local ones
    else
    {
        if(selectedPlatoon == -1)
            selectedPlatoon = platoonSelection_->findBestPlatoon(platoonControllers_, position, direction);

        bool success = manageLocalPlatoon(selectedPlatoon, msg);
        sendJoinPlatoonResponse(success, selectedPlatoon, msg);

    }
}

void MECPlatooningProducerApp::handleAvailablePlatoonsRequest(cMessage *msg)
{
    EV << "MECPlatooningProducerApp::handleAvailablePlatoonsRequest():";
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    L3Address producerAppAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
    int producerAppPort = packet->getTag<L4PortInd>()->getSrcPort();
    auto req = packet->removeAtFront<PlatooningAvailablePlatoonsPacket>();
//    int appProducerId = req->getProducerAppId();

//    nlohmann::json jsonPlatoons;

    std::map<int, std::vector<PlatoonVehicleInfo *>> platoons;
    for(const auto& platoon : platoonControllers_)
    {
        platoons[platoon.first] = platoon.second->getPlatoonMembers();
//        jsonPlatoons["platoonsInfo"].push_back(platoon.second->dumpPlatoonToJSON());
    }
    EV << " platoon size: " << platoons.size() << endl;
    req->setType(AVAILABLE_PLATOONS_RESPONSE);
    req->setPlatoons(platoons);
    req->setProducerAppId(producerAppId_);

    int chunkLen = sizeof(getId())+sizeof(platooningProducerAppsPort);
    req->setChunkLength(inet::B(chunkLen));
    inet::Packet* responsePacket = new Packet (packet->getName());
    responsePacket->insertAtBack(req);
    responsePacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    platooningProducerAppsSocket_.sendTo(responsePacket, producerAppAddress, producerAppPort);

    delete packet;
}

void MECPlatooningProducerApp::handleAvailablePlatoonsResponse(cMessage *msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    L3Address producerAppAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
//    int producerAppPort = packet->getTag<L4PortInd>()->getSrcPort();
    auto response = packet->removeAtFront<PlatooningAvailablePlatoonsPacket>();
    int appProducerId = response->getProducerAppId();

    // response arrived to late, the request timer already expired
    if(currentPlatoonRequestTimer_ == nullptr)
    {
        EV << "MECPlatooningProducerApp::handleAvailablePlatoonsResponse - PlatooningAvailablePlatoonsResponsePacket arrived to late" << endl;
        delete packet;
        return;
    }
    // response arrived to late, the request id is higher than the one arrived
    if(currentPlatoonRequestTimer_->getRequestId() > response->getRequestId())
    {
        EV << "MECPlatooningProducerApp::handleAvailablePlatoonsResponse - PlatooningAvailablePlatoonsResponsePacket arrived to late, another request has been already sent" << endl;
        delete packet;
        return;
    }

    // the response is correct
    int recvResponses = currentPlatoonRequestTimer_->getReceivedResponses();
    currentPlatoonRequestTimer_->setReceivedResponses(recvResponses + 1);

    //TODO too many copies?
    auto platoons = response->getPlatoons();
    if(platoons.size() != 0)
       currentAvailablePlatoons_[appProducerId] = platoons;

    if(recvResponses + 1 == currentPlatoonRequestTimer_->getExpectedResponses())
    {
        EV << "MECPlatooningProducerApp::handleAvailablePlatoonsResponse - all platoons arrived. finding the best platoon..." << endl;
        // all the requests arrived, stop timer and find the best platoon
        cancelEvent(currentPlatoonRequestTimer_);
        while(!consumerAppRequests_.isEmpty())
        {
            cMessage *req = (cMessage *)consumerAppRequests_.front();
            consumerAppRequests_.pop();
            handlePendingJoinRequest(req);
        }

        // clean structures
        currentPlatoonRequestTimer_->setExpectedResponses(0);
        currentPlatoonRequestTimer_->setReceivedResponses(0);
        currentPlatoonRequestTimer_->setRequestId(0);
        currentAvailablePlatoons_.clear();
    }

    delete packet;
}


void MECPlatooningProducerApp::handlePendingJoinRequest(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto joinReq = packet->peekAtFront<PlatooningJoinPacket>();
    int mecAppId = joinReq->getMecAppId();
    inet::Coord position = joinReq->getLastPosition();
    inet::Coord direction = joinReq->getDirection();
    int platoonControllerIndex = joinReq->getControllerIndex();

    PlatoonIndex index;
    if(platoonControllerIndex == -1)
    {
        EV << "MECPlatooningProducerApp::handlePendingJoinRequest(): A specific platoon is not required, finding the best among all the available (if any)" << endl;
        index = platoonSelection_->findBestPlatoon(producerAppId_, platoonControllers_, currentAvailablePlatoons_, position, direction);
    }
    else
    {
        EV << "MECPlatooningProducerApp::handlePendingJoinRequest(): A specific platoon with id "<< platoonControllerIndex << " is requested";

        bool found = false;
        for(auto const& platoon: platoonControllers_ )
        {
            if(platoon.first == platoonControllerIndex )
            {
                index.producerApp = producerAppId_;
                found = true;
                EV << " and it has been found in one of the local ones" << endl;
                break;
            }
        }
        if(!found)
        {
            for(auto const& platoon: currentAvailablePlatoons_ )
            {
                if(platoon.second.find(platoonControllerIndex) != platoon.second.end())
                {
                    index.producerApp = platoon.first;
                    found = true;
                    EV << " and it has been found in one of the federated producer App with Id: "<<  index.producerApp << endl;
                    break;
                }
            }
        }

        if(!found)
        {
            EV << " but it has been found anywhere. It will be created in this producer app " << endl;
            index.producerApp = producerAppId_; // create a new platoon in the local producerApp
        }

        index.platoonIndex = platoonControllerIndex;
    }

    bool success;

    if(index.producerApp == producerAppId_) // local
    {
        success = manageLocalPlatoon(index.platoonIndex, msg);
        sendJoinPlatoonResponse(success, index.platoonIndex, msg);
    }
    else
    {
        // send add member notification to the selected producer mec App
        inet::Packet* responsePacket = new Packet ("PlatooningAddMemberPacket");
        auto addMemberReq = inet::makeShared<PlatooningAddMemberPacket>();
        addMemberReq->setType(ADD_MEMBER_REQUEST);
        addMemberReq->setProducerAppId(producerAppId_);
        addMemberReq->setMecAppId(mecAppId);
        addMemberReq->setPosition(position);
        addMemberReq->setPlatoonIndex(index.platoonIndex);
        addMemberReq->setUeAddress(joinReq->getUeAddress());

        int chunkLen = sizeof(getId())+sizeof(platooningProducerAppsPort);
        addMemberReq->setChunkLength(inet::B(chunkLen));

        responsePacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
        responsePacket->insertAtBack(addMemberReq);

        ProducerAppInfo producerAppEndpoint = producerAppEndpoint_[index.producerApp];
        platooningProducerAppsSocket_.sendTo(responsePacket, producerAppEndpoint.address, producerAppEndpoint.port);

        delete packet; // delete  join packet, it is not useful anymore
        return;
    }
}


bool MECPlatooningProducerApp::manageLocalPlatoon(int& index, cMessage* req)
{

    inet::Packet* packet = check_and_cast<inet::Packet*>(req);
    auto joinReq = packet->peekAtFront<PlatooningJoinPacket>();
    int mecAppId = joinReq->getMecAppId();
    inet::Coord position = joinReq->getLastPosition();
    inet::Coord direction = joinReq->getDirection();

    PlatoonControllerBase* platoonController;

    if (index < 0)
    {
        // no platoons found
        // select the index to be assign to the new controller
        index = nextControllerIndex_++;
    }
    if (platoonControllers_.find(index) != platoonControllers_.end())
    {
        platoonController = platoonControllers_[index];
        EV << "MECPlatooningProducerApp::sendJoinPlatoonRequest - MEC App " << mecAppId << " added to platoon " << index << endl;
    }
    else
    {
        // if no active platoon controller can be used, create one
        if(!strcmp(par("controller").stringValue(), "rajamani"))
            platoonController = new RajamaniPlatoonController(this, index, 0.1, 0.1); // TODO select the controller and set periods
        else if(!strcmp(par("controller").stringValue(), "safe"))
            platoonController = new SafePlatoonController(this, index, 0.1, 0.1); // TODO select the controller and set periods
        else
            throw cRuntimeError("MECPlatooningProducerApp::manageLocalPlatoon(): Controller with name %s not found", par("controller").stringValue());

        platoonController->setDirection(direction);
        platoonControllers_[index] = platoonController;
        EV << "MECPlatooningProducerApp::sendJoinPlatoonRequest - MEC App " << mecAppId << " added to new platoon " << index << " - dir[" << direction << "]" << endl;
    }

    bool success = platoonController->addPlatoonMember(mecAppId, producerAppId_, position, joinReq->getUeAddress());
    if(success)
    {
        auto mecAppInfo = consumerAppEndpoint_.find(mecAppId);
        mecAppInfo->second.producerAppId = producerAppId_;
    }

    return success;
}


void MECPlatooningProducerApp::sendJoinPlatoonResponse(bool success, int platoonIndex, cMessage* msg)
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
        joinReq->setControllerIndex(platoonIndex);
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
    platooningConsumerAppsSocket.sendTo(responsePacket, address, port);
    EV << "MECPlatooningProducerApp::sendJoinPlatoonRequest - Join response sent to the UE" << endl;

    delete packet;
}

//void MECPlatooningProducerApp::handleJoinPlatoonRequest(cMessage* msg)
//{
//    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
//    auto joinReq = packet->removeAtFront<PlatooningJoinPacket>();
//    int mecAppId = joinReq->getMecAppId();
//    inet::Coord position = joinReq->getLastPosition();
//    inet::Coord direction = joinReq->getDirection();
//
//    EV << "MECPlatooningProducerApp::sendJoinPlatoonRequest - Received join request from MEC App " << mecAppId << endl;
//
//    PlatoonControllerBase* platoonController;
//    int selectedPlatoon = joinReq->getControllerIndex();
//    if (joinReq->getControllerIndex() == -1)
//    {
//        // the UE did not specify the controller index explicitly
//        // find the most suitable platoonController for this new request
//        selectedPlatoon = platoonSelection_->findBestPlatoon(platoonControllers_, position, direction);
//        if (selectedPlatoon < 0)
//        {
//            // no platoons found
//            // select the index to be assign to the new controller
//            selectedPlatoon = nextControllerIndex_++;
//        }
//    }
//
//    if (platoonControllers_.find(selectedPlatoon) != platoonControllers_.end())
//    {
//        platoonController = platoonControllers_[selectedPlatoon];
//        EV << "MECPlatooningProducerApp::sendJoinPlatoonRequest - MEC App " << mecAppId << " added to platoon " << selectedPlatoon << endl;
//    }
//    else
//    {
//        // if no active platoon controller can be used, create one
//        platoonController = new RajamaniPlatoonController(this, selectedPlatoon, 0.1, 0.1); // TODO select the controller and set periods
//        platoonController->setDirection(direction);
//        platoonControllers_[selectedPlatoon] = platoonController;
//        EV << "MECPlatooningProducerApp::sendJoinPlatoonRequest - MEC App " << mecAppId << " added to new platoon " << selectedPlatoon << " - dir[" << direction << "]" << endl;
//    }
//
//    bool success = platoonController->addPlatoonMember(mecAppId, position, joinReq->getUeAddress());
//
//
//    // prepare response
//    inet::Packet* responsePacket = new Packet (packet->getName());
//    joinReq->setType(JOIN_RESPONSE);
//    if (success)
//    {
//        // accept the request and send ACK
//        joinReq->setResponse(true);
//        joinReq->setControllerIndex(selectedPlatoon);
//        joinReq->setColor("green");
//    }
//    else
//    {
//        joinReq->setResponse(false);
//    }
//
//    responsePacket->insertAtBack(joinReq);
//    responsePacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
//
//    // send response to the UE's MEC app
//    platooningConsumerAppsSocket.sendTo(responsePacket, ueAppAddress, ueAppPort);
//    EV << "MECPlatooningProducerApp::sendJoinPlatoonRequest - Join response sent to the UE" << endl;
//
//    delete packet;
//}

void MECPlatooningProducerApp::handlePlatoonCommand(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto cmdMsg = packet->removeAtFront<PlatooningInfoPacket>();
    int consumerMecAppId = cmdMsg->getMecAppId();

    if (consumerAppEndpoint_.find(consumerMecAppId) == consumerAppEndpoint_.end())
    {
        throw cRuntimeError("MECPlatooningProducerApp::handleControlTimer - endpoint for MEC app %d not found.", mecAppId);
    }

    inet::Packet* pkt = new Packet(packet->getName());
    pkt->insertAtBack(cmdMsg);
    ConsumerAppInfo endpoint = consumerAppEndpoint_.at(consumerMecAppId);
    platooningConsumerAppsSocket.sendTo(pkt, endpoint.address, endpoint.port);
    EV << "MECPlatooningProducerApp::handleControllerTimer() - Sent new acceleration value[" << cmdMsg->getNewAcceleration() << "] to MEC App " << consumerMecAppId << " [" << endpoint.address.str() << ":" << endpoint.port << "]" << endl;

    delete packet;
}


void MECPlatooningProducerApp::handleLeavePlatoonRequest(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto leaveReq = packet->removeAtFront<PlatooningLeavePacket>();
    L3Address mecAppAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
    int mecAppPort = packet->getTag<L4PortInd>()->getSrcPort();

    int mecAppId = leaveReq->getMecAppId();
    int controllerIndex = leaveReq->getControllerIndex();

    // check if the MEC consumer app is in a local platoon or  not
    auto consApp = consumerAppEndpoint_.find(mecAppId);

    if(consApp == consumerAppEndpoint_.end())
        throw cRuntimeError("MECPlatooningProducerApp::handleLeavePlatoonRequest - MEC consumer app with index %d does not exist", mecAppId);

    if(consApp->second.producerAppId != producerAppId_)
    {
        // send request to correct producer App
        inet::Packet* reqPacket = new Packet (packet->getName());
        leaveReq->setType(REMOVE_MEMBER_REQUEST);
        leaveReq->setProducerAppId(producerAppId_);
        reqPacket->insertAtBack(leaveReq);
        reqPacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

        auto pAppEp = producerAppEndpoint_[consApp->second.producerAppId];
        EV << " MECPlatooningProducerApp::handleLeavePlatoonRequest: forward LeavePlatoonRequest to platoon producer App with Id: " << consApp->second.producerAppId << endl;
        platooningProducerAppsSocket_.sendTo(reqPacket, pAppEp.address , pAppEp.port);
    }
    else
    {
        bool success = removePlatoonMember(controllerIndex, mecAppId);

        inet::Packet* responsePacket = new Packet (packet->getName());
        leaveReq->setType(LEAVE_RESPONSE);
        if (success)
        {
           // accept the request and send ACK
           leaveReq->setResponse(true);
        }
        else
        {
           leaveReq->setResponse(false);
        }

        responsePacket->insertAtBack(leaveReq);
        responsePacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

        // send response to the UE's consumer MEC app
        platooningConsumerAppsSocket.sendTo(responsePacket, mecAppAddress, mecAppPort);
        EV << "MECPlatooningProducerApp::handleLeavePlatoonRequest - Leave response sent to the UE" << endl;
    }

    delete packet;
}


bool MECPlatooningProducerApp::removePlatoonMember(int controllerIndex, int mecAppId)
{
    EV << "MECPlatooningProducerApp::removePlatoonMember - Received leave request from MEC app " << mecAppId << endl;

   // TODO retrieve the platoonController for this member
   if (platoonControllers_.find(controllerIndex) == platoonControllers_.end())
       throw cRuntimeError("MECPlatooningProducerApp::removePlatoonMember - platoon with index %d does not exist", controllerIndex);

   PlatoonControllerBase* platoonController = platoonControllers_.at(controllerIndex);
   if (platoonController == nullptr)
   {
       EV << "MECPlatooningProducerApp::removePlatoonMember - the UE was not registered to any platoonController " << endl;
       return false;
   }

   // remove the member from the identified platoonController
   bool success = platoonController->removePlatoonMember(mecAppId);

   return success;
}

void MECPlatooningProducerApp::startTimer(cMessage* msg, double timeOffset)
{
    if (timeOffset < 0.0)
        throw cRuntimeError("MECPlatooningProducerApp::startTimer - time offset not valid[%f]", timeOffset);

    PlatooningTimer* timer = check_and_cast<PlatooningTimer*>(msg);
    int controllerIndex = timer->getControllerIndex();
    if (platoonControllers_.find(controllerIndex) == platoonControllers_.end())
        throw cRuntimeError("MECPlatooningProducerApp::startTimer - controller with index %d not found", controllerIndex);

    if (timer->getType() == PLATOON_CONTROL_TIMER)
    {
        // if a control timer was already set for this controller, cancel it
        if (activeControlTimer_.find(controllerIndex) != activeControlTimer_.end())
            cancelAndDelete(activeControlTimer_.at(controllerIndex));

        // store timer into the timers' data structure. This is necessary to retrieve the timer in case it needs to be canceled
        activeControlTimer_[controllerIndex] = timer;

        EV << "MECPlatooningProducerApp::startTimer() - New ControlTimer set for controller " << controllerIndex << " in " << timeOffset << "s" << endl;
    }
    else if (timer->getType() == PLATOON_UPDATE_POSITION_TIMER)
    {
        // if a position timer was already set for this controller, cancel it
        if (activeUpdatePositionTimer_.find(controllerIndex) != activeUpdatePositionTimer_.end())
            cancelAndDelete(activeUpdatePositionTimer_.at(controllerIndex));

        // store timer into the timers' data structure. This is necessary to retrieve the timer in case it needs to be canceled
        activeUpdatePositionTimer_[controllerIndex] = timer;

        EV << "MECPlatooningProducerApp::startTimer() - New UpdatePositionTimer set for controller " << controllerIndex << " in " << timeOffset << "s" << endl;
    }

    //schedule timer
    scheduleAt(simTime() + timeOffset, timer);
}

void MECPlatooningProducerApp::stopTimer(int controllerIndex, PlatooningTimerType timerType)
{
    if (timerType == PLATOON_CONTROL_TIMER)
    {
        if (activeControlTimer_.find(controllerIndex) == activeControlTimer_.end())
            throw cRuntimeError("MECPlatooningProducerApp::stopTimer - no active ControlTimer with index %d", controllerIndex);

        cancelAndDelete(activeControlTimer_.at(controllerIndex));
        EV << "MECPlatooningProducerApp::stopTimer - ControlTimer for controller " << controllerIndex << "has been stopped" << endl;
    }
    else if (timerType == PLATOON_UPDATE_POSITION_TIMER)
    {
        if (activeUpdatePositionTimer_.find(controllerIndex) == activeUpdatePositionTimer_.end())
            throw cRuntimeError("MECPlatooningProducerApp::stopTimer - no active UpdatePositionTimer with index %d", controllerIndex);

        cancelAndDelete(activeUpdatePositionTimer_.at(controllerIndex));
        EV << "MECPlatooningProducerApp::stopTimer - UpdatePositionTimer for controller " << controllerIndex << "has been stopped" << endl;
    }
    else
        throw cRuntimeError("MECPlatooningProducerApp::stopTimer - unrecognized timer type %d", timerType);
}

void MECPlatooningProducerApp::handleControlTimer(ControlTimer* ctrlTimer)
{
    int controllerIndex = ctrlTimer->getControllerIndex();

    EV << "MECPlatooningProducerApp::handleControlTimer - Control platoon with index " << controllerIndex << endl;

    if (platoonControllers_.find(controllerIndex) == platoonControllers_.end())
        throw cRuntimeError("MECPlatooningProducerApp::handleControlTimer - controller with index %d not found", controllerIndex);

    // retrieve platoon controller and run the control algorithm
    PlatoonControllerBase* platoonController = platoonControllers_.at(controllerIndex);
    const CommandList* cmdList = platoonController->controlPlatoon();

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
            bool isRemote = false;
            int remoteProducerAppId;

            if (consumerAppEndpoint_.find(mecAppId) == consumerAppEndpoint_.end())
            {
                // check if the the mecAppId belongs to a different producerApp
                auto it = remoteConsumerAppToProducerApp_.find(mecAppId);
                if( it == remoteConsumerAppToProducerApp_.end())
                {
                    throw cRuntimeError("MECPlatooningProducerApp::handleControlTimer - endpoint for MEC app %d not found.", mecAppId);
                }
                else
                {
                    remoteProducerAppId = it->second;
                    isRemote = true;
                }
            }
            inet::Packet* pkt = new inet::Packet("PlatooningInfoPacket");

            auto cmd = inet::makeShared<PlatooningInfoPacket>();
            cmd->setType(PLATOON_CMD);
            cmd->setMecAppId(mecAppId); // used in case of it is sent to an other produceApp
            cmd->setNewAcceleration(newAcceleration);
            cmd->setChunkLength(inet::B( sizeof(double) ));
            cmd->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
            cmd->addTagIfAbsent<inet::PrecedingVehicleTag>()->setUeAddress(precUeAddress);
            pkt->insertAtBack(cmd);

            if(!isRemote)
            {
                ConsumerAppInfo endpoint = consumerAppEndpoint_.at(mecAppId);
                platooningConsumerAppsSocket.sendTo(pkt, endpoint.address, endpoint.port);
                EV << "MECPlatooningProducerApp::handleControllerTimer() - Sent new acceleration value[" << newAcceleration << "] to MEC App " << it->first << " [" << endpoint.address.str() << ":" << endpoint.port << "]" << endl;
            }
            else
            {
                ProducerAppInfo pai = producerAppEndpoint_.at(remoteProducerAppId);
                platooningProducerAppsSocket_.sendTo(pkt, pai.address, pai.port);
                EV << "MECPlatooningProducerApp::handleControllerTimer() - Sent new acceleration value[" << newAcceleration << "] to producer App " << remoteProducerAppId << " for MEC App " <<  it->first << endl;
            }
        }
        delete cmdList;
    }

    // re-schedule controller timer
    if (ctrlTimer->getPeriod() > 0.0)
        scheduleAt(simTime() + ctrlTimer->getPeriod(), ctrlTimer);
    else
        stopTimer(controllerIndex, (PlatooningTimerType)ctrlTimer->getType());
}

void MECPlatooningProducerApp::handleUpdatePositionTimer(UpdatePositionTimer* posTimer)
{
    int controllerIndex = posTimer->getControllerIndex();

    EV << "MECPlatooningProducerApp::handleUpdatePositionTimer - Request position updates for platoon with index " << controllerIndex << endl;

    if (platoonControllers_.find(controllerIndex) == platoonControllers_.end())
        throw cRuntimeError("MECPlatooningProducerApp::handleUpdatePositionTimer - controller with index %d not found", controllerIndex);

    // retrieve platoon controller and run the control algorithm
    PlatoonControllerBase* platoonController = platoonControllers_.at(controllerIndex);
    std::map<int, std::set<inet::L3Address> > addressList = platoonController->getUeAddressList();
    for(const auto& produceApp : addressList)
    {
        requirePlatoonLocations(produceApp.first, controllerIndex, produceApp.second);
    }


    // re-schedule controller timer
    if (posTimer->getPeriod() > 0.0)
        scheduleAt(simTime() + posTimer->getPeriod(), posTimer);
    else
        stopTimer(controllerIndex, (PlatooningTimerType)posTimer->getType());
}

void MECPlatooningProducerApp::requirePlatoonLocations(int producerAppId, int controllerIndex, const std::set<inet::L3Address>& ues)
{
    auto producerApp = producerAppEndpoint_.find(producerAppId);
    if(producerApp == producerAppEndpoint_.end())
    {
        throw cRuntimeError("MECPlatooningProducerApp::sendGetRequest(): producerAppId %d not found", producerAppId);
    }
    // get the Address of the UEs
    std::stringstream ueAddresses;
    EV << "MECPlatooningProducerApp::requirePlatoonLocations - " << ues.size() << endl;
    for( std::set<inet::L3Address>::iterator it = ues.begin(); it != ues.end(); ++it)
    {
        EV << "MECPlatooningProducerApp::requirePlatoonLocations - " << *it << endl;
        ueAddresses << "acr:" << *it;

        if(!sendBulk_)
        {
            //    // send GET request
            sendGetRequest(producerAppId, ueAddresses.str());
            // insert the controller id in the relative requests queue
            producerApp->second.controllerPendingRequests.push(controllerIndex);
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

        //insert the controller id in the relative requests queue
        producerApp->second.controllerPendingRequests.push(controllerIndex);
    }
}

void MECPlatooningProducerApp::sendGetRequest(int producerAppId, const std::string& ues)
{

    auto producerApp = producerAppEndpoint_.find(producerAppId);
    if(producerApp == producerAppEndpoint_.end())
    {
        throw cRuntimeError("MECPlatooningProducerApp::sendGetRequest(): producerAppId %d not found", producerAppId);
    }

    TcpSocket *sock = producerApp->second.locationServiceSocket;

    //check if the ueAppAddress is specified
    if(sock->getState() == inet::TcpSocket::CONNECTED)
    {
        EV << "MECPlatooningProducerApp::sendGetRequest(): send request to the Location Service" << endl;
        std::stringstream uri;
        uri << "/example/location/v2/queries/users?address=" << ues;
        EV << "MECPlatooningProducerApp::requestLocation(): uri: "<< uri.str() << endl;
        std::string host = sock->getRemoteAddress().str()+":"+std::to_string(sock->getRemotePort());
        Http::sendGetRequest(sock, host.c_str(), uri.str().c_str());
    }
    else
    {
        EV << "MECPlatooningProducerApp::sendGetRequest(): Location Service not connected" << endl;
    }
}

void MECPlatooningProducerApp::established(int connId)
{
    EV <<"MECPlatooningProducerApp::established - connId ["<< connId << "]" << endl;


    if(connId == mp1Socket_->getSocketId())
    {
        EV << "MECPlatooningProducerApp::established - Mp1Socket"<< endl;

        // once the connection with the Service Registry has been established, obtain the
        // endPoint (address+port) of the Location Service
        const char *uri = "/example/mec_service_mgmt/v1/services?ser_name=LocationService";
        std::string host = mp1Socket_->getRemoteAddress().str()+":"+std::to_string(mp1Socket_->getRemotePort());

        Http::sendGetRequest(mp1Socket_, host.c_str(), uri);
    }
//    else if (connId == serviceSocket_->getSocketId())
//    {
//        EV << "MECPlatooningProducerApp::established - conection Id: "<< connId << endl;
//    }
//    else
//    {
//        throw cRuntimeError("MecAppBase::socketEstablished - Socket %d not recognized", connId);
//    }
}






