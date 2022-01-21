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
    nextControllerIndex_ = 1000;
}

MECPlatooningProducerApp::~MECPlatooningProducerApp()
{
    delete platoonSelection_;
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

    EV << "MECPlatooningProducerApp::initialize - MECc application "<< getClassName() << " with mecAppId["<< mecAppId << "] has started!" << endl;

   // connect with the service registry
   EV << "MECPlatooningProducerApp::initialize - Initialize connection with the Service Registry via Mp1" << endl;
   connect(&mp1Socket_, mp1Address, mp1Port);
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

            // TODO use enumerate variables instead of strings for comparison

            if (strcmp(platooningPkt->getType(), REGISTRATION_REQUEST) == 0)
            {
                handleRegistrationRequest(msg);
            }
            else if(strcmp(platooningPkt->getType(), JOIN_REQUEST) == 0)
            {
                handleJoinPlatoonRequest(msg);

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
                throw cRuntimeError("MECPlatooningProducerApp::handleUeMessage - packet not recognized");
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

void MECPlatooningProducerApp::handleMp1Message()
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

void MECPlatooningProducerApp::handleServiceMessage()
{
        if(serviceHttpMessage->getType() == RESPONSE)
        {
            // If the request is associated to a controller no more available, just discard
             int controllerIndex = controllerPendingRequests.front();
             controllerPendingRequests.pop();
             ControllerMap::iterator it = platoonControllers_.find(controllerIndex);
             if(it == platoonControllers_.end())
             {
                 EV << "MECPlatooningProducerApp::handleServiceMessage - platoon controller with index ["<< controllerIndex <<"] no more available"<< endl;
                 return;
             }


            HttpResponseMessage *rspMsg = dynamic_cast<HttpResponseMessage*>(serviceHttpMessage);
            if(rspMsg->getCode() == 200) // in response to a successful GET request
            {
                nlohmann::json jsonBody;
                EV << "MECPlatooningProducerApp::handleServiceMessage - response 200 " << endl;

                try
                {
                  jsonBody = nlohmann::json::parse(rspMsg->getBody()); // get the JSON structure
                  std::vector<UEInfo> uesInfo;
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
            HttpRequestMessage *reqMsg = dynamic_cast<HttpRequestMessage*>(serviceHttpMessage);
            EV << "MECPlatooningProducerApp::handleServiceMessage - response with HTTP code:  " << reqMsg->getMethod() << " discarded"  << endl;
        }
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
    MECAppEndpoint info;
    info.address = mecAppAddress;
    info.port = mecAppPort;
    mecAppEndpoint_[mecAppId] = info;

    // send (n)ack to the MEC app
    inet::Packet* respPacket = new Packet(packet->getName());
    registrationReq->setType(REGISTRATION_RESPONSE);
    respPacket->insertAtFront(registrationReq);
    respPacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    platooningConsumerAppsSocket.sendTo(respPacket, mecAppAddress, mecAppPort);

    EV << "MECPlatooningProducerApp::handleRegistrationRequest - Sending response to MEC app " << mecAppId << " [" << mecAppAddress.str() << ":" << mecAppPort << "]" << endl;

    delete packet;
}

void MECPlatooningProducerApp::handleJoinPlatoonRequest(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto joinReq = packet->removeAtFront<PlatooningJoinPacket>();
    int mecAppId = joinReq->getMecAppId();
    inet::Coord position = joinReq->getLastPosition();
    inet::Coord direction = joinReq->getDirection();

    EV << "MECPlatooningProducerApp::sendJoinPlatoonRequest - Received join request from MEC App " << mecAppId << endl;

    PlatoonControllerBase* platoonController;
    int selectedPlatoon = joinReq->getControllerIndex();
    if (joinReq->getControllerIndex() == -1)
    {
        // the UE did not specify the controller index explicitly
        // find the most suitable platoonController for this new request
        selectedPlatoon = platoonSelection_->findBestPlatoon(platoonControllers_, position, direction);
        if (selectedPlatoon < 0)
        {
            // no platoons found
            // select the index to be assign to the new controller
            selectedPlatoon = nextControllerIndex_++;
        }
    }

    if (platoonControllers_.find(selectedPlatoon) != platoonControllers_.end())
    {
        platoonController = platoonControllers_[selectedPlatoon];
        EV << "MECPlatooningProducerApp::sendJoinPlatoonRequest - MEC App " << mecAppId << " added to platoon " << selectedPlatoon << endl;
    }
    else
    {
        // if no active platoon controller can be used, create one
        platoonController = new RajamaniPlatoonController(this, selectedPlatoon, 0.1, 0.1); // TODO select the controller and set periods
        platoonController->setDirection(direction);
        platoonControllers_[selectedPlatoon] = platoonController;
        EV << "MECPlatooningProducerApp::sendJoinPlatoonRequest - MEC App " << mecAppId << " added to new platoon " << selectedPlatoon << " - dir[" << direction << "]" << endl;

    }

    // add the member to the identified platoonController
    bool success = platoonController->addPlatoonMember(mecAppId, position, joinReq->getUeAddress());

    // prepare response
    inet::Packet* responsePacket = new Packet (packet->getName());
    joinReq->setType(JOIN_RESPONSE);
    if (success)
    {
        // accept the request and send ACK
        joinReq->setResponse(true);
        joinReq->setControllerIndex(selectedPlatoon);
        joinReq->setColor("green");
    }
    else
    {
        joinReq->setResponse(false);
    }

    responsePacket->insertAtFront(joinReq);
    responsePacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    // send response to the UE's MEC app
    platooningConsumerAppsSocket.sendTo(responsePacket, ueAppAddress, ueAppPort);
    EV << "MECPlatooningProducerApp::sendJoinPlatoonRequest - Join response sent to the UE" << endl;

    delete packet;
}

void MECPlatooningProducerApp::handleLeavePlatoonRequest(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto leaveReq = packet->removeAtFront<PlatooningLeavePacket>();
    int mecAppId = leaveReq->getMecAppId();
    int controllerIndex = leaveReq->getControllerIndex();

    EV << "MECPlatooningProducerApp::handleLeavePlatoonRequest - Received leave request from MEC app " << mecAppId << endl;

    // TODO retrieve the platoonController for this member
    if (platoonControllers_.find(controllerIndex) == platoonControllers_.end())
        throw cRuntimeError("MECPlatooningProducerApp::handleLeavePlatoonRequest - platoon with index %d does not exist", controllerIndex);

    PlatoonControllerBase* platoonController = platoonControllers_.at(controllerIndex);
    if (platoonController == nullptr)
    {
        EV << "MECPlatooningProducerApp::handleLeavePlatoonRequest - the UE was not registered to any platoonController " << endl;
        return;
    }

    // remove the member from the identified platoonController
    bool success = platoonController->removePlatoonMember(mecAppId);

    EV << "MECPlatooningProducerApp::sendLeavePlatoonRequest - MEC App " << mecAppId << " left platoon 0" << endl;

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

    responsePacket->insertAtFront(leaveReq);
    responsePacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    // send response to the UE's MEC app
    platooningConsumerAppsSocket.sendTo(responsePacket, ueAppAddress, ueAppPort);
    EV << "MECPlatooningProducerApp::handleLeavePlatoonRequest() - Leave response sent to the UE" << endl;

    delete packet;
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
            double newAcceleration = it->second;

            if (mecAppEndpoint_.find(mecAppId) == mecAppEndpoint_.end())
                throw cRuntimeError("MECPlatooningProducerApp::handleControlTimer - endpoint for MEC app %d not found.", mecAppId);

            MECAppEndpoint endpoint = mecAppEndpoint_.at(mecAppId);

            inet::Packet* pkt = new inet::Packet("PlatooningInfoPacket");
            auto cmd = inet::makeShared<PlatooningInfoPacket>();
            cmd->setType(PLATOON_CMD);
            cmd->setNewAcceleration(newAcceleration);
            cmd->setChunkLength(inet::B( sizeof(double) ));
            cmd->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
            pkt->insertAtBack(cmd);

            platooningConsumerAppsSocket.sendTo(pkt, endpoint.address, endpoint.port);
            EV << "MECPlatooningProducerApp::handleControllerTimer() - Sent new acceleration value[" << newAcceleration << "] to MEC App " << it->first << " [" << endpoint.address.str() << ":" << endpoint.port << "]" << endl;
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
    std::set<inet::L3Address> addressList = platoonController->getUeAddressList();
    requirePlatoonLocations(controllerIndex, &addressList);

    // re-schedule controller timer
    if (posTimer->getPeriod() > 0.0)
        scheduleAt(simTime() + posTimer->getPeriod(), posTimer);
    else
        stopTimer(controllerIndex, (PlatooningTimerType)posTimer->getType());
}

void MECPlatooningProducerApp::requirePlatoonLocations(int controllerIndex, const std::set<inet::L3Address>* ues)
{
    // get the Address of the UEs
    std::stringstream ueAddresses;
    EV << "MECPlatooningProducerApp::requirePlatoonLocations - " << ues->size() << endl;
    for( std::set<inet::L3Address>::iterator it = ues->begin(); it != ues->end(); ++it)
    {
        EV << "MECPlatooningProducerApp::requirePlatoonLocations - " << *it << endl;
        ueAddresses << "acr:" << *it;
        if(next(it) != ues->end())
            ueAddresses << ",";
    }
    // send GET request
    sendGetRequest(ueAddresses.str());

    // insert the controller id in the requests queue
    controllerPendingRequests.push(controllerIndex);
}

void MECPlatooningProducerApp::sendGetRequest(const std::string& ues)
{
    //check if the ueAppAddress is specified
    if(serviceSocket_.getState() == inet::TcpSocket::CONNECTED)
    {
        EV << "MECPlatooningProducerApp::sendGetRequest(): send request to the Location Service" << endl;
        std::stringstream uri;
        uri << "/example/location/v2/queries/users?address=" << ues;
        EV << "MECPlatooningProducerApp::requestLocation(): uri: "<< uri.str() << endl;
        std::string host = serviceSocket_.getRemoteAddress().str()+":"+std::to_string(serviceSocket_.getRemotePort());
        Http::sendGetRequest(&serviceSocket_, host.c_str(), uri.str().c_str());
    }
    else
    {
        EV << "MECPlatooningProducerApp::sendGetRequest(): Location Service not connected" << endl;
    }
}

void MECPlatooningProducerApp::established(int connId)
{
    if(connId == mp1Socket_.getSocketId())
    {
        EV << "MECPlatooningProducerApp::established - Mp1Socket"<< endl;

        // once the connection with the Service Registry has been established, obtain the
        // endPoint (address+port) of the Location Service
        const char *uri = "/example/mec_service_mgmt/v1/services?ser_name=LocationService";
        std::string host = mp1Socket_.getRemoteAddress().str()+":"+std::to_string(mp1Socket_.getRemotePort());

        Http::sendGetRequest(&mp1Socket_, host.c_str(), uri);
    }
    else if (connId == serviceSocket_.getSocketId())
    {
        EV << "MECPlatooningProducerApp::established - Location Service"<< endl;
    }
    else
    {
        throw cRuntimeError("MecAppBase::socketEstablished - Socket %d not recognized", connId);
    }
}






