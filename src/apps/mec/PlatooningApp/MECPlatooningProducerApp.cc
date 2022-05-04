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
#include "nodes/mec/MECOrchestrator/MECOMessages/MECOrchestratorMessages_m.h"
#include "nodes/mec/MECPlatformManager/MecPlatformManager.h"

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
    platooningConsumerAppsPort_ = par("platooningConsumerAppsPort");
    platooningConsumerAppsSocket_.setOutputGate(gate("socketOut"));
    platooningConsumerAppsSocket_.bind(platooningConsumerAppsPort_);

    platooningProducerAppsPort_ = par("platooningProducerAppsPort");
    platooningProducerAppsSocket_.setOutputGate(gate("socketOut"));
    platooningProducerAppsSocket_.bind(platooningProducerAppsPort_);

    platooningControllerAppsPort_ = par("platooningControllerAppsPort");
    platooningControllerAppsSocket_.setOutputGate(gate("socketOut"));
    platooningControllerAppsSocket_.bind(platooningControllerAppsPort_);


    producerAppId_ = par("producerAppId");
    nextControllerIndex_ = 1000 * (1 + producerAppId_);


    adjustPosition_  = par("adjustPosition").boolValue();
    sendBulk_ = par("sendBulk").boolValue();

    received_ = 0;

    updatesDifferences_ = registerSignal("updateDiffs");


    EV << "MECPlatooningProducerApp::initialize - MEC application "<< getClassName() << " with mecAppId["<< mecAppId << "] has started!" << endl;

   // connect with the service registry
   EV << "MECPlatooningProducerApp::initialize - Initialize connection with the Service Registry via Mp1" << endl;

   retrievePlatoonsInfoDuration_ = 0.100; //TODO add parameter!

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
        if(platooningConsumerAppsSocket_.belongsToSocket(msg))
        {
            // determine its source address/port
            auto pk = check_and_cast<Packet *>(msg);
            ueAppAddress = pk->getTag<L3AddressInd>()->getSrcAddress();
            ueAppPort = pk->getTag<L4PortInd>()->getSrcPort();

            auto platooningPkt = pk->peekAtFront<PlatooningAppPacket>();
            EV << "MECPlatooningProducerApp::handleMessage(): messageType " << platooningPkt->getType()  << endl;

            if (platooningPkt->getType() == DISCOVER_PLATOONS_REQUEST)
            {
                handleDiscoverPlatoonRequest(msg);

            }
            else if(platooningPkt->getType() == DISCOVER_ASSOCIATE_PLATOON_REQUEST)
            {
                handleDiscoverAndAssociatePlatoonRequest(msg);
            }
            else if (platooningPkt->getType() == ASSOCIATE_PLATOON_REQUEST)
            {
                handleAssociatePlatoonRequest(msg);
            }
            else
            {
                throw cRuntimeError("MECPlatooningProducerApp::handleMessage - packet from platoonConsumerApp not recognized");
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
            else
            {
                throw cRuntimeError("MECPlatooningProducerApp::handleMessage - packet from platoonProviderApp not recognized");
            }
            return;
        }
        else if(platooningControllerAppsSocket_.belongsToSocket(msg))
        {
            // determine its source address/port
            auto pk = check_and_cast<Packet *>(msg);
            auto platooningPkt = pk->peekAtFront<PlatooningAppPacket>();
            EV << "MECPlatooningProducerApp::handleMessage(): messageType " << platooningPkt->getType()  << endl;
            if (platooningPkt->getType() == CONTROLLER_NOTIFICATION)
            {
                handleControllerNotification(msg);
            }
            else if (platooningPkt->getType() == CONF_CONTROLLER_RESPONSE)
            {
                handleControllerConfigurationResponse(msg);
            }
            else
            {
                throw cRuntimeError("MECPlatooningProducerApp::handleMessage - packet from platoonControllerApp not recognized");
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
    if(strcmp(msg->getName(), "HeartbeatTimer") == 0)
    {
        controlHeartbeats();

    }
}

void MECPlatooningProducerApp::controlHeartbeats()
{}

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
}

// From MECPlatooningControlleApp

void MECPlatooningProducerApp::handleControllerNotification(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    //    L3Address producerAppAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
    //    int producerAppPort = packet->getTag<L4PortInd>()->getSrcPort();
    auto controllerNotification = packet->peekAtFront<PlatooningControllerNotificationPacket>();
//    int appProducerId = controllerNotification->getProducerAppId();
    auto platoon = platoonControllers_.find(controllerNotification->getControllerId());
    if(platoon == platoonControllers_.end())
    {
        EV << "MECPlatooningProducerApp::handleControllerNotification: platoonId"<< controllerNotification->getControllerId() << "not present" << endl;
    }
    else
    {
        if (controllerNotification->getNotificationType() == NEW_MEMBER)
        {
            EV << "MECPlatooningProducerApp::handleControllerNotification: A new member with MECPlatoonConsumerApp [" << controllerNotification->getConsumerAppId() <<"] has been added to platoon with id "<< controllerNotification->getControllerId() << endl;
            handleNewMemberNotification(msg);
        }
        else if (controllerNotification->getNotificationType() == REMOVED_MEMBER)
        {
            EV << "MECPlatooningProducerApp::handleControllerNotification: A new member with MECPlatoonConsumerApp [" << controllerNotification->getConsumerAppId() <<"] has been removed from platoon with id "<< controllerNotification->getControllerId() << endl;
            handleLeaveMemberNotification(msg);
        }
        else if (controllerNotification->getNotificationType() == NEW_ORDER)
            EV << "MECPlatooningProducerApp::handleControllerNotification: The platoon with id "<< controllerNotification->getControllerId() << " changes the cars order" << endl;
        else if (controllerNotification->getNotificationType() == HEARTBEAT)
        {
            EV << "MECPlatooningProducerApp::handleControllerNotification: Heartbeat received from controller with id "<< controllerNotification->getControllerId() << ". Update timestamps and vehicles" << endl;
            handleHeartbeat(msg);
        }
        platoon->second->vehicles = controllerNotification->getVehiclesOrder();
        platoon->second->lastHeartBeat = controllerNotification->getHeartbeatTimeStamp();

        // TODO if the controller gets empty, remove?
    }

    delete packet;
    return;

}

void MECPlatooningProducerApp::handleNewMemberNotification(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto controllerNotification = packet->peekAtFront<PlatooningControllerNotificationPacket>();

    auto platoon = platoonControllers_.find(controllerNotification->getControllerId());
    if(platoon != platoonControllers_.end())
    {
        platoon->second->vehicles = controllerNotification->getVehiclesOrder();
        platoon->second->lastHeartBeat = controllerNotification->getHeartbeatTimeStamp();
    }
    else
    {
        EV << "MECPlatooningProducerApp::handleNewMemberNotification - Controller with id [" << controllerNotification->getControllerId() << "] not found!" << endl;
    }
    return;
}

void MECPlatooningProducerApp::handleLeaveMemberNotification(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto controllerNotification = packet->peekAtFront<PlatooningControllerNotificationPacket>();

    auto platoon = platoonControllers_.find(controllerNotification->getControllerId());
    platoon->second->vehicles = controllerNotification->getVehiclesOrder();
    platoon->second->lastHeartBeat = controllerNotification->getHeartbeatTimeStamp();

    if(platoon->second->vehicles.size() == 0)
    {
        EV << "MECPlatooningProducerApp::handleLeaveMemberNotification: The platoon managed by the controller with Id ["<< controllerNotification->getControllerId() << "] is empty, remove it from the list." << endl;
        DeleteAppMessage* deleteAppMsg = new DeleteAppMessage();
        deleteAppMsg->setUeAppID(platoon->second->mecAppId);
        // get MEC platform manager and require mec app instantiation
        cModule* mecPlatformManagerModule = this->getModuleByPath("^.mecPlatformManager");
        MecPlatformManager* mecpm = check_and_cast<MecPlatformManager*>(mecPlatformManagerModule);
        bool success  = mecpm->terminateMEApp(deleteAppMsg);

        if(success)
        {
            EV << "MECPlatooningProducerApp::handleLeaveMemberNotification: Controller with Id ["<< controllerNotification->getControllerId() << "] successfully removed" << endl;
            delete platoon->second;
            platoonControllers_.erase(platoon);
        }
        else
        {
            EV << "MECPlatooningProducerApp::handleLeaveMemberNotification: Controller with Id ["<< controllerNotification->getControllerId() << "] not removed. Some error occurred" << endl;

        }

    }

    return;
}

void MECPlatooningProducerApp::handleHeartbeat(cMessage *msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto controllerNotification = packet->peekAtFront<PlatooningControllerNotificationPacket>();

    auto platoon = platoonControllers_.find(controllerNotification->getControllerId());
    if(platoon != platoonControllers_.end())
    {
        platoon->second->vehicles = controllerNotification->getVehiclesOrder();
        platoon->second->lastHeartBeat = controllerNotification->getHeartbeatTimeStamp();
    }
    else
    {
        EV << "MECPlatooningProducerApp::handleHeartbeat - Controller with id [" << controllerNotification->getControllerId() << "] not found!" << endl;
    }
    return;
}

void MECPlatooningProducerApp::handleControllerConfigurationResponse(cMessage *msg)
{
    EV << "MECPlatooningProducerApp::handleControllerConfigurationResponse: controller configuration response arrived" << endl;
    delete msg;
}

// From MECPlatooningConsumerApp

void MECPlatooningProducerApp::handleDiscoverPlatoonRequest(cMessage *msg)
{
    /*
     * This function returns the list of all the available platoons in the federation.
     * Upon this kind of request, the producer app requires the platoons infos to all the
     * other producer apps in the federation.
     *
     */
    if(producerAppEndpoint_.size() > 1)
    {
        if(currentPlatoonRequestTimer_->isScheduled())
        {
            EV << "MECPlatooningProducerApp::handleDiscoverPlatoonRequest - A request for retrieve all the available platoons is already issued. Wait for the response" << endl;
            consumerAppRequests_.insert(msg);
            return;
        }
        else
        {
            EV << "MECPlatooningProducerApp::handleDiscoverPlatoonRequest - New request issued to all the producer apps. Wait for the response" << endl;
            for(auto &prodApp: producerAppEndpoint_)
            {
                if(prodApp.first == producerAppId_)
                    continue; // do not auto send this message
                // create msg and send
                inet::Packet* availablePlatoonPacket = new Packet ("PlatooningAvailablePlatoonsRequestPacket");
                auto availablePlatoonReq = inet::makeShared<PlatooningAvailablePlatoonsPacket>();
                availablePlatoonReq->setType(AVAILABLE_PLATOONS_REQUEST);
                availablePlatoonReq->setProducerAppId(producerAppId_);
                availablePlatoonReq->setRequestId(requestCounter_);

                int chunkLen = 2*sizeof(int) + 1;
                availablePlatoonReq->setChunkLength(inet::B(chunkLen));

                availablePlatoonPacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
                availablePlatoonPacket->insertAtBack(availablePlatoonReq);

                platooningProducerAppsSocket_.sendTo(availablePlatoonPacket, prodApp.second.address , prodApp.second.port);
            }
            consumerAppRequests_.insert(msg);
            currentPlatoonRequestTimer_->setRequestId(requestCounter_);
            currentPlatoonRequestTimer_->setExpectedResponses(producerAppEndpoint_.size() - 1);
            currentPlatoonRequestTimer_->setReceivedResponses(0);

            requestCounter_++;

            scheduleAt(simTime() + currentPlatoonRequestTimer_->getPeriod(), currentPlatoonRequestTimer_);
        }
    }
    else
    {
        handlePendingJoinRequest(msg);
    }
}

void MECPlatooningProducerApp::handleDiscoverAndAssociatePlatoonRequest(cMessage *msg)
{

    /*
     * This function choose the best platoon for a given car.
     * Upon this kind of request, the producer app requires the platoons infos to all the
     * other producer apps in the federation and choose the best one. Than returns the end point
     * to the consumer app
     *
     */

    handleDiscoverPlatoonRequest(msg);

}

void MECPlatooningProducerApp::handleAssociatePlatoonRequest(cMessage * msg)
{

    /*
     * This function return the end point of a specific controller requested by the consumer app.
     * It checks if the request is achievable
     * */

    }


// From MECPlatooningProducerApp

void MECPlatooningProducerApp::handleAvailablePlatoonsRequest(cMessage *msg)
{
    EV << "MECPlatooningProducerApp::handleAvailablePlatoonsRequest():";
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    L3Address producerAppAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
    int producerAppPort = packet->getTag<L4PortInd>()->getSrcPort();
    auto req = packet->removeAtFront<PlatooningAvailablePlatoonsPacket>();
//    int appProducerId = req->getProducerAppId();

//    nlohmann::json jsonPlatoons;

    std::map<int, PlatoonControllerStatus *> platoons;
    for(const auto& platoon : platoonControllers_)
    {
        // TODO imlement this
        platoons[platoon.first] = platoon.second;
    }
    EV << " platoon size: " << platoons.size() << endl;
    req->setType(AVAILABLE_PLATOONS_RESPONSE);
    req->setPlatoons(platoons);
    req->setProducerAppId(producerAppId_);

    int chunkLen = sizeof(int) + 1 + sizeof(platoons) ;
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


// Utils

void MECPlatooningProducerApp::handlePendingJoinRequest(cMessage* msg)
{
    /*
     * Redefine to manage based on the request type, i.e.
     * DISCOVER_PLATOONS, DISCOVER_ASSOCIATE_PLATOON
     */

    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);

    auto request = packet->peekAtFront<PlatooningAppPacket>();

    if(request->getType() == DISCOVER_PLATOONS_REQUEST)
    {
        // send the list of the platoon to the consumer app. Not used for now..
    }

    else if(request->getType() == DISCOVER_ASSOCIATE_PLATOON_REQUEST)
    {
        auto discoverAssociateMsg = packet->removeAtFront<PlatooningDiscoverAndAssociatePlatoonPacket>();
        Coord position = discoverAssociateMsg->getLastPosition();
        Coord direction = discoverAssociateMsg->getDirection();

        PlatoonIndex index;
        index = platoonSelection_->findBestPlatoon(producerAppId_, platoonControllers_, currentAvailablePlatoons_, position, direction);

        // vars for the response message
        bool success = false;
        L3Address controllerAddress;
        int controllerPort = -1;
        int platoonId = -1;


        // check if the selected platoon is managed by the local producer app, or by another producerApp
        if(index.producerApp == producerAppId_)
        {
            ControllerMap::iterator platoonController;

            if(index.platoonIndex == -1)
            {
                // Instantiate a new platoon controller
                int newPlatoonIndex = instantiateLocalPlatoon(direction);
                if(newPlatoonIndex >= 0)
                {
                    platoonController = platoonControllers_.find(newPlatoonIndex);
                    if(platoonController != platoonControllers_.end())
                    {
                       success = true;
                       controllerAddress = platoonController->second->controllerEndpoint.address;
                       controllerPort = platoonController->second->controllerEndpoint.port + (int)mecHost->par("maxMECApps") + 1;
                       platoonId = newPlatoonIndex;
                    }
                }

            }
            else
            {
                // send the controller end point to the controller
                platoonController = platoonControllers_.find(index.platoonIndex);
                if(platoonController != platoonControllers_.end())
                {
                    success = true;
                    controllerAddress = platoonController->second->controllerEndpoint.address;
                    controllerPort = platoonController->second->controllerEndpoint.port + (int)mecHost->par("maxMECApps") + 1;
                    platoonId = index.platoonIndex;
                }

            }
        }
        else
        {
            // find the producer app
            auto remoteControllersList = currentAvailablePlatoons_.find(index.producerApp);
            auto controller = remoteControllersList->second.find(index.platoonIndex);
            if(controller != remoteControllersList->second.end())
            {
                success = true;
                controllerAddress = controller->second->controllerEndpoint.address;
                controllerPort = controller->second->controllerEndpoint.port + + (int)mecHost->par("maxMECApps") + 1;
                platoonId = index.platoonIndex;
            }
            else
            {
                success = false;
            }

        }

        discoverAssociateMsg->setType(DISCOVER_ASSOCIATE_PLATOON_RESPONSE);
        discoverAssociateMsg->setResult(success);
        discoverAssociateMsg->setProducerAppId(producerAppId_);
        discoverAssociateMsg->setControllerId(platoonId);
        discoverAssociateMsg->setControllerAddress(controllerAddress);
        discoverAssociateMsg->setControllerPort(controllerPort);
        discoverAssociateMsg->setChunkLength(B(20));

        // send the response to the consumer app
        inet::Packet* responsePacket = new Packet("PlatooningAssociatedPlatoonPacket");

        responsePacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
        responsePacket->insertAtBack(discoverAssociateMsg);

        L3Address address = packet->getTag<L3AddressInd>()->getSrcAddress();
        int port = packet->getTag<L4PortInd>()->getSrcPort();

        platooningConsumerAppsSocket_.sendTo(responsePacket, address, port);

    }
    else if(request->getType() == ASSOCIATE_PLATOON_REQUEST)
    {
        // TODO define

    }
    else
    {
        throw cRuntimeError("MECPlatooningProducerApp::handlePendingJoinRequest - PlatooningAppPacket type %d not valid in this function.", request->getType());
    }

    delete packet; // delete  join packet, it is not useful anymore
    return;
}

int MECPlatooningProducerApp::instantiateLocalPlatoon(Coord& direction)
{
    int index = nextControllerIndex_++;

    // TODO
    /*
     * Instantiate a new MECApp controller App.
     * Use the MEC to instantiate it. save the endpoint
     * store all in the structure.
     *
     * NOTE:
     * MECPlatooningController apps are instantiated in the same MEC host where
     * the MECPlatooningProducer app runs.
     */

    // get MEC platform manager and require mec app instantiation
    cModule* mecPlatformManagerModule = this->getModuleByPath("^.mecPlatformManager");
    MecPlatformManager* mecpm = check_and_cast<MecPlatformManager*>(mecPlatformManagerModule);

    MecAppInstanceInfo* appInfo = nullptr;
    CreateAppMessage * createAppMsg = new CreateAppMessage();

//    const char* controllerName = par("controllerName"); // It must be in the form of MECPlatooningControllerRajamani;
    string contName = string("MECPlatooningControllerApp");
    string path = string("simu5g.apps.mec.PlatooningApp.");

    stringstream moduleType;
    moduleType << path << contName;

    createAppMsg->setUeAppID(mecAppId);
    createAppMsg->setMEModuleName(contName.c_str());
    createAppMsg->setMEModuleType(moduleType.str().c_str()); //path

    createAppMsg->setRequiredCpu(5000.);
    createAppMsg->setRequiredRam(10.);
    createAppMsg->setRequiredDisk(10.);
    createAppMsg->setRequiredService("NULL"); // insert OMNeT like services, NULL if not
    createAppMsg->setContextId(index);

    appInfo = mecpm->instantiateMEApp(createAppMsg);

    if(!appInfo->status)
    {
        EV << "MECPlatooningProducerApp::manageLocalPlatoon - something went wrong during MEC app instantiation"<< endl;
        return -1;
    }

    else
    {
        // The MEC APP has been instantiated correctly, send to it a
        // configuration message

        // TODO also send the new member of the platoon

        inet::Packet* pkt = new inet::Packet("PlatooningConfigurationPacket");
        auto confMsg = inet::makeShared<PlatooningConfigureControllerPacket>();
        confMsg->setType(CONF_CONTROLLER);
        confMsg->setDirection(direction);
        confMsg->setProducerAppId(producerAppId_);
        confMsg->setControllerId(index);
        confMsg->setHeartbeatPeriod(par("heartbeatPeriod").doubleValue());
        confMsg->setLongitudinalController(par("longitudinalController").stringValue());
        confMsg->setTraversalController(par("transitionalController").stringValue());
        confMsg->setControlPeriod(par("controlPeriod").doubleValue());
        confMsg->setUpdatePositionPeriod(par("updatePositionPeriod").doubleValue());
        confMsg->setAdjustPosition(par("adjustPosition").boolValue());
        confMsg->setSendBulk(par("sendBulk").boolValue());


        confMsg->setChunkLength(inet::B( 2*sizeof(double) + 2*sizeof(int) + 20 ));
        confMsg->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
        pkt->insertAtBack(confMsg);

        platooningControllerAppsSocket_.sendTo(pkt, appInfo->endPoint.addr, appInfo->endPoint.port);

        // add the new controller to the structure
        PlatoonControllerStatus* newController = new PlatoonControllerStatus;
        newController->controllerId = index;
        newController->controllerEndpoint.address = appInfo->endPoint.addr;
        newController->controllerEndpoint.port = appInfo->endPoint.port;
        newController->direction = direction;
        newController->pointerToMECPlatooningControllerApp = appInfo->module;
        newController->mecAppId  = appInfo->instanceIntegerId
                ;

        platoonControllers_[index] = newController;
        return index;
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






