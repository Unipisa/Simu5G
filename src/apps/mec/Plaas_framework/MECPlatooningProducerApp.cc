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

#include "../Plaas_framework/MECPlatooningProducerApp.h"

#include "apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket_Types.h"
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
#include "../Plaas_framework/platoonController/RajamaniPlatoonController.h"
#include "../Plaas_framework/platoonController/SafePlatoonController.h"
#include "../Plaas_framework/platoonSelection/PlatoonSelectionSample.h"

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

    /*
     * I am a background MEC App, so the MEC Orchestrator is not used.
     * I call myself the VIM to register the mec app
     */
    if(vim != nullptr)
    {
        bool res = vim->registerMecApp(mecAppId, requiredRam, requiredDisk, requiredCpu);
        if(res  == false)
            throw cRuntimeError("MECPlatooningProducerApp::initialize - The Mec App cannot by instantiated!");
    }
    mecPlatoonControllerAppIdCounter_ = mecAppId + 1;

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
    EV << "ADJ" << adjustPosition_ << endl;
    sendBulk_ = par("sendBulk").boolValue();

    received_ = 0;

    updatesDifferences_ = registerSignal("updateDiffs");

    mecAppInstantiationTime_ = par("mecAppInstantiationTime").doubleValue();

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
//       producerAppinfo.locationServiceSocket = addNewSocket();

       int id = entry->get("id");

       // TODO create socket
       producerAppEndpoint_[id] = producerAppinfo;

       EV <<"MECPlatooningProducerApp::initialize - id: " << id <<" address: "  << producerAppinfo.address.str() <<" port " << producerAppinfo.port << endl;
   }

    currentPlatoonRequestTimer_ = new CurrentPlatoonRequestTimer();
    currentPlatoonRequestTimer_->setPeriod(retrievePlatoonsInfoDuration_);

//    mp1Socket_ = addNewSocket();

    // Add local producerApp
    ProducerAppInfo localProducerAppInfo;
    localProducerAppInfo.port = -1; // it is not used
    localProducerAppInfo.address = L3Address(); // it is not used

    localProducerAppInfo.locationServicePort = -1;    // set with service registry
    localProducerAppInfo.locationServiceAddress = L3Address(); // set with service registry
//    localProducerAppInfo.locationServiceSocket = addNewSocket();
    producerAppEndpoint_[producerAppId_] = localProducerAppInfo;

//   connect(mp1Socket_, mp1Address, mp1Port);
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
    else if(strcmp(msg->getName(), "MecAppInstantiationTimer") == 0)
    {
        MecAppInstantiationTimer *timer = check_and_cast<MecAppInstantiationTimer*>(msg);
        int controllerId = timer->getControllerId();
        Coord direction = timer->getDirection();

        auto controller = platoonControllers_.find(controllerId);
           if(controller == platoonControllers_.end())
               throw cRuntimeError("MECPlatooningProducerApp::handleSelfMessage - Controller with id [%d] not found!", controllerId);

        MecAppInstanceInfo* appInfo = instantiateLocalPlatoon(direction, controllerId);
        // response
        L3Address address = appInfo->endPoint.addr;
        int port = appInfo->endPoint.port;
        bool result = appInfo->status;

        if(result)
        {
            EV << "MECPlatooningProducerApp::handleSelfMessage - MecPlatoonControllerApp for controller with id [" << controllerId << "] has been instantiated" << endl;
            controller->second->isInstantiated = true;
            controller->second->controllerEndpoint.address = address;
            controller->second->controllerEndpoint.port = port;
            controller->second->mecAppId = appInfo->instanceIntegerId;
            controller->second->pointerToMECPlatooningControllerApp = appInfo->module;
         }
        else
        {
            EV << "MECPlatooningProducerApp::handleSelfMessage - MecPlatooninControllerApp for controller with id [" << controllerId << "] has not been instantiated!" << endl;
            platoonControllers_.erase(controller);
        }

        /**
         * For all the pending request, respond to the one related to the controller id
         */

        auto pendingReqs = instantiatingPendingControllersQueue_.find(controllerId);
        if(pendingReqs == instantiatingPendingControllersQueue_.end())
            throw cRuntimeError("MECPlatooningProducerApp::handleSelfMessage: instantiatingPendingControllersQueue_ has not requests associated to controller with id [%d]", controllerId);

        int socketPort = port +(int)mecHost->par("maxMECApps") + 1;
        for(auto& req : pendingReqs->second)
            finalizeJoinRequest(req, result, address, socketPort, controllerId);

        instantiatingPendingControllersQueue_.erase(controllerId);


        // send the notification CONTROLLER_INSTANTIATION_RESPONSE to the producer apps waiting for the instantiation

        auto pendingNots = instantiatingControllersQueue_.find(controllerId);

        if(pendingNots != instantiatingControllersQueue_.end())
        {
            for(int prodApp : pendingNots->second)
            {
                inet::Packet* packet = new Packet("PlatooningControllerInstantiationNotificationPacket");
                auto notification = inet::makeShared<PlatooningControllerInstantiationNotificationPacket>();
                notification->setType(CONTROLLER_INSTANTIATION_RESPONSE);
                notification->setControllerId(controllerId);
                notification->setResult(true);
                notification->setControllerAddress(controller->second->controllerEndpoint.address);
                notification->setControllerPort(controller->second->controllerEndpoint.port + (int)mecHost->par("maxMECApps") + 1);
                notification->setChunkLength(inet::B(10));

                packet->insertAtBack(notification);
                packet->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

                auto address = producerAppEndpoint_[prodApp].address;
                int port  = producerAppEndpoint_[prodApp].port;
                EV << "MECPlatooningProducerApp::MecAppInstantiationTimer - platoon controller with id [" << controllerId <<  "] has been instantiated."<<
                " Send response to the MecPlatooningProducerApp with id [" << prodApp << "]"<< endl;
                platooningProducerAppsSocket_.sendTo(packet, address, port);
            }

            instantiatingControllersQueue_.erase(controllerId);
        }

        delete appInfo;
        delete msg;
    }
}

void MECPlatooningProducerApp::handleProcessedMessage(cMessage *msg)
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
            else if (platooningPkt->getType() == CONTROLLER_INSTANTIATION_REQUEST)
            {
                handleInstantiationNotificationRequest(msg);
            }
            else if (platooningPkt->getType() == CONTROLLER_INSTANTIATION_RESPONSE)
            {
                handleInstantiationNotificationResponse(msg);
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
    MecAppBase::handleProcessedMessage(msg);
}

void MECPlatooningProducerApp::handleHttpMessage(int connId)
{
}

void MECPlatooningProducerApp::handleMp1Message(int connId)
{
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
            //handleLeaveMemberNotification(msg);
        }
        else if (controllerNotification->getNotificationType() == NEW_ORDER)
            EV << "MECPlatooningProducerApp::handleControllerNotification: The platoon with id "<< controllerNotification->getControllerId() << " changes the cars order" << endl;
        else if (controllerNotification->getNotificationType() == HEARTBEAT)
        {
            EV << "MECPlatooningProducerApp::handleControllerNotification: Heartbeat received from controller with id "<< controllerNotification->getControllerId() << ". Update timestamps and vehicles" << endl;
//            handleHeartbeat(msg);
        }

        else if (controllerNotification->getNotificationType() == STOPPED)
        {
            EV << "MECPlatooningProducerApp::handleControllerNotification: stopped notification received from controller with id "<< controllerNotification->getControllerId() << ". Update timestamps and vehicles" << endl;
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
        platoon->second->queuedJoinRequests = controllerNotification->getQueuedJoinRequests();
        platoon->second->lastHeartBeat = controllerNotification->getHeartbeatTimeStamp();

        int consumerAppId =  controllerNotification->getConsumerAppId();
        auto address = controllerNotification->getConsumerAddress();
        int port = controllerNotification->getConsumerPort();
        platoon->second->associatedConsumerApps[consumerAppId] = {address, port, -1};
        EV << "MECPlatooningProducerApp::handleLeaveMemberNotification - MecPlatooningConsumerApp with id ["<< consumerAppId << "] added to the controller with id [" << controllerNotification->getControllerId() << "]" << endl;
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

    if(platoon != platoonControllers_.end())
    {
        platoon->second->vehicles = controllerNotification->getVehiclesOrder();
        platoon->second->queuedJoinRequests = controllerNotification->getQueuedJoinRequests();
        platoon->second->lastHeartBeat = controllerNotification->getHeartbeatTimeStamp();
        int consumerAppId =  controllerNotification->getConsumerAppId();
        platoon->second->associatedConsumerApps.erase(consumerAppId);
        EV << "MECPlatooningProducerApp::handleLeaveMemberNotification - MecPlatooningConsumerApp with id ["<< consumerAppId << "] removed from the controller with id [" << controllerNotification->getControllerId() << "]" << endl;
    }
    else
    {
        EV << "MECPlatooningProducerApp::handleNewMemberNotification - Controller with id [" << controllerNotification->getControllerId() << "] not found!" << endl;
    }


    if(platoon->second->vehicles.size() == 0 && platoon->second->queuedJoinRequests == 0)
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
        platoon->second->queuedJoinRequests = controllerNotification->getQueuedJoinRequests();


        if(platoon->second->vehicles.size() == 0 && platoon->second->queuedJoinRequests == 0)
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
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    L3Address consumerAppAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
//    int consumerAppPort = packet->getTag<L4PortInd>()->getSrcPort();
//    auto req = packet->removeAtFront<PlatooningAppPacket>();
//
//    int consumerAppId = req->getConsumerAppId();

    handleDiscoverPlatoonRequest(msg);

}

void MECPlatooningProducerApp::handleAssociatePlatoonRequest(cMessage * msg)
{
    /*
     * This function return the end point of a specific controller requested by the consumer app.
     * It checks if the request is achievable
     * */
//    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
//    L3Address consumerAppAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
//    int consumerAppPort = packet->getTag<L4PortInd>()->getSrcPort();
//    auto req = packet->removeAtFront<PlatooningAppPacket>();
//
//    int consumerAppId = req->getConsumerAppId();
// TODO FINISH
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

    req->setType(AVAILABLE_PLATOONS_RESPONSE);
    req->setPlatoons(&platoonControllers_);
    req->setProducerAppId(producerAppId_);

    int chunkLen = sizeof(int) + 1 + sizeof(platoonControllers_.size()) ;
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
    if(platoons->size() != 0)
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

void MECPlatooningProducerApp::handleInstantiationNotificationRequest(cMessage *msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto request = packet->removeAtFront<PlatooningControllerInstantiationNotificationPacket>();

    int controllerId = request->getControllerId();
    auto controller = platoonControllers_.find(controllerId);
    if(controller == platoonControllers_.end())
    {
        EV << "MECPlatooningProducerApp::handleInstantiationNotificationRequest - platoon controller with id [" << controllerId <<  "] not present." << endl;
        request->setType(CONTROLLER_INSTANTIATION_RESPONSE);
        request->setResult(false);

        inet::Packet* pkt = new inet::Packet("PlatooningControllerInstantiationNotificationPacket");

        pkt->insertAtBack(request);
        pkt->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

        auto address = packet->getTag<L3AddressInd>()->getSrcAddress();
        int port  = packet->getTag<L4PortInd>()->getSrcPort();

        platooningProducerAppsSocket_.sendTo(pkt, address, port);

    }
    else
    {
        // if it is already instantiated, send the response
        if(controller->second->isInstantiated)
        {
            request->setType(CONTROLLER_INSTANTIATION_RESPONSE);
            request->setResult(true);
            request->setControllerAddress(controller->second->controllerEndpoint.address);
            request->setControllerPort(controller->second->controllerEndpoint.port + (int)mecHost->par("maxMECApps") + 1);

            inet::Packet* pkt = new inet::Packet("PlatooningControllerInstantiationNotificationPacket");

            pkt->insertAtBack(request);
            pkt->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

            auto address = packet->getTag<L3AddressInd>()->getSrcAddress();
            int port  = packet->getTag<L4PortInd>()->getSrcPort();

            platooningProducerAppsSocket_.sendTo(pkt, address, port);

            EV << "MECPlatooningProducerApp::handleInstantiationNotificationRequest - platoon controller with id [" << controllerId <<  "] has been instantiated."<<
                    " Send response to the MecPlatooningProducerApp with id [" << request->getProducerAppId() << "]"<< endl;
        }
        else
        {
         // the controller is still under instantiation
            EV << "MECPlatooningProducerApp::handleInstantiationNotificationRequest - platoon controller with id [" << controllerId <<  "] is still under instantiation."<<
                   " Wait for its completion"<< endl;
            packet->insertAtBack(request); // it has been removed at the begin of this method
            if(instantiatingControllersQueue_.find(controllerId) == instantiatingControllersQueue_.end())
                instantiatingControllersQueue_[controllerId].push_back(request->getProducerAppId());
            else
            {
                EV << "MECPlatooningProducerApp::handleInstantiationNotificationRequest - An instantiation notification for controller with Id [" << controllerId << "] is already present" << endl;
            }
        }
    }
    delete packet;
}

void MECPlatooningProducerApp::handleInstantiationNotificationResponse(cMessage *msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto request = packet->removeAtFront<PlatooningControllerInstantiationNotificationPacket>();

    int controllerId = request->getControllerId();

    /**
     * For all the pending request, respond to the one related to the controller id
     */

    auto pendingReqs = instantiatingPendingControllersQueue_.find(controllerId);
    if(pendingReqs == instantiatingPendingControllersQueue_.end())
        throw cRuntimeError("MECPlatooningProducerApp::handleSelfMessage: instantiatingPendingControllersQueue_ has not requests associated to controller with id [%d]", controllerId);

    int socketPort = request->getControllerPort();
    auto address = request->getControllerAddress();
    EV << "MECPlatooningProducerApp::handleInstantiationNotificationResponse - Instantiation of controller with id[" << controllerId << "] on producer App with id [" << request->getProducerAppId() <<  "] arrived" << endl;
    for(auto& req : pendingReqs->second)
        finalizeJoinRequest(req, request->getResult(), address, socketPort, controllerId);

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
        auto discoverAssociateMsg = packet->peekAtFront<PlatooningDiscoverAndAssociatePlatoonPacket>();
        Coord position = discoverAssociateMsg->getLastPosition();
        Coord direction = discoverAssociateMsg->getDirection();

        PlatoonIndex index;
        index = platoonSelection_->findBestPlatoon(producerAppId_, platoonControllers_, currentAvailablePlatoons_, position, direction);

        // vars for the response message
        bool success = false;
        L3Address controllerAddress = L3Address();
        int controllerPort = -1;
        int platoonId = -1;


        // check if the selected platoon is managed by the local producer app, or by another producerApp
        if(index.producerApp == producerAppId_)
        {
            ControllerMap::iterator platoonController;

            if(index.platoonIndex == -1)
            {
                // Instantiate a new platoon controller
                int index = nextControllerIndex_++;
                // add the new controller to the structure
                PlatoonControllerStatus* newController = new PlatoonControllerStatus;
                newController->controllerId = index;
//                newController->controllerEndpoint.address = appInfo->endPoint.addr;
//                newController->controllerEndpoint.port = appInfo->endPoint.port;
                newController->direction = direction;
//                newController->pointerToMECPlatooningControllerApp = appInfo->module;
//                newController->mecAppId  = appInfo->instanceIntegerId;
                newController->isInstantiated = false;
                platoonControllers_[index] = newController;

                packet->insertAtBack(discoverAssociateMsg);

                instantiatingPendingControllersQueue_[index].push_back(msg);

                MecAppInstantiationTimer* timer = new MecAppInstantiationTimer("MecAppInstantiationTimer");
                timer->setDirection(direction);
                timer->setControllerId(index);

                scheduleAt(simTime() + mecAppInstantiationTime_, timer);
                EV << "MECPlatooningProducerApp::handlePendingJoinRequest - A MecPlatoonControllerApp for controller with id [" << index << "] will be instantiated." << endl;
                return;

            }
            else
            {
                // send the controller end point to the controller
                platoonController = platoonControllers_.find(index.platoonIndex);
                if(platoonController != platoonControllers_.end())
                {
                    if(platoonController->second->isInstantiated)
                    {
                        success = true;
                        controllerAddress = platoonController->second->controllerEndpoint.address;
                        controllerPort = platoonController->second->controllerEndpoint.port + (int)mecHost->par("maxMECApps") + 1;
                        platoonId = index.platoonIndex;
                    }
                    else
                    {
                        EV << "MECPlatooningProducerApp::handlePendingJoinRequest - The controller with id [" << index.platoonIndex << "] is in instantiation phase. Wait for its completion." << endl;
                        // the controller is being instantiated, put the request in the queue
                        instantiatingPendingControllersQueue_[index.platoonIndex].push_back(msg);
                        return;
                    }
                }
            }
        }
        else
        {
            // find the federated producer app
            auto remoteControllersList = currentAvailablePlatoons_.find(index.producerApp);
            auto controller = remoteControllersList->second->find(index.platoonIndex);
            if(controller != remoteControllersList->second->end())
            {
                if(controller->second->isInstantiated)
                {
                    success = true;
                    controllerAddress = controller->second->controllerEndpoint.address;
                    controllerPort = controller->second->controllerEndpoint.port + (int)mecHost->par("maxMECApps") + 1;
                    platoonId = index.platoonIndex;
                }
                else
                {
                    EV << "MECPlatooningProducerApp::handlePendingJoinRequest - The MecPlatoonControllerApp for controller with id [" << index.platoonIndex << "] will be instantiated on the MEC system of MecPlatooningProducerApp with id [" << index.producerApp << "].";

                    if(instantiatingControllersQueue_.find(index.platoonIndex) == instantiatingControllersQueue_.end())
                    {
                        EV << " Instantiation notification sent."<< endl;

                        // ask the producer app to notify the completion of the controller instantiation
                        inet::Packet* packet = new Packet("PlatooningControllerInstantiationNotificationPacket");
                        auto notification = inet::makeShared<PlatooningControllerInstantiationNotificationPacket>();
                        notification->setType(CONTROLLER_INSTANTIATION_REQUEST);
                        notification->setConsumerAppId(request->getConsumerAppId());
                        notification->setProducerAppId(producerAppId_);

                        notification->setControllerId(index.platoonIndex);
                        notification->setChunkLength(B(20));

                        packet->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
                        packet->insertAtBack(notification);

                        auto producerApp =  producerAppEndpoint_.find(index.producerApp);

                        platooningProducerAppsSocket_.sendTo(packet, producerApp->second.address, producerApp->second.port);

                    }
                   else
                   {
                       EV << " Instantiation notification already sent."<< endl;
                   }
                    instantiatingPendingControllersQueue_[index.platoonIndex].push_back(msg);
                    return;
                }
            }
            else
            {
                EV << "MECPlatooningProducerApp::handlePendingJoinRequest - Controller with id [" << index.platoonIndex << "] not found in the federated MecPlatooningProducerApp with id [" << index.producerApp << "]" << endl;
                success = false;
            }
        }

        finalizeJoinRequest(msg, success, controllerAddress, controllerPort, platoonId);
    }
    else if(request->getType() == ASSOCIATE_PLATOON_REQUEST)
    {
        // TODO define

    }
    else
    {
        throw cRuntimeError("MECPlatooningProducerApp::handlePendingJoinRequest - PlatooningAppPacket type %d not valid in this function.", request->getType());
    }
    return;
}

void MECPlatooningProducerApp::finalizeJoinRequest(cMessage* msg, bool result, L3Address& address, int port, int controllerId)
{
    EV << "MECPlatooningProducerApp::finalizeJoinRequest - result: " << result << endl;
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto baseMsg = packet->peekAtFront<PlatooningAppPacket>();

    // send the response to the consumer app
    inet::Packet* responsePacket = new Packet("PlatooningAssociatedPlatoonPacket");

    if(baseMsg->getType() == ASSOCIATE_PLATOON_REQUEST)
    {
        auto assMsg = packet->removeAtFront<PlatooningAssociatePlatoonPacket>();
        assMsg->setType(ASSOCIATE_PLATOON_RESPONSE);
        assMsg->setResult(result);
        if(result){
            assMsg->setProducerAppId(producerAppId_);
            assMsg->setControllerId(controllerId);
            assMsg->setControllerAddress(address);
            assMsg->setControllerPort(port);
        }

        assMsg->setChunkLength(B(20));
        responsePacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
        responsePacket->insertAtBack(assMsg);
    }

    else if(baseMsg->getType() == DISCOVER_ASSOCIATE_PLATOON_REQUEST)
    {
        auto discAndAssMsg = packet->removeAtFront<PlatooningDiscoverAndAssociatePlatoonPacket>();
        discAndAssMsg->setType(DISCOVER_ASSOCIATE_PLATOON_RESPONSE);
        discAndAssMsg->setResult(result);
        if(result){
            discAndAssMsg->setProducerAppId(producerAppId_);
            discAndAssMsg->setControllerId(controllerId);
            discAndAssMsg->setControllerAddress(address);
            discAndAssMsg->setControllerPort(port);
        }

        discAndAssMsg->setChunkLength(B(20));
        responsePacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
        responsePacket->insertAtBack(discAndAssMsg);
    }

    else if(baseMsg->getType() == CONTROLLER_INSTANTIATION_REQUEST)
    {
        auto notification = packet->removeAtFront<PlatooningControllerInstantiationNotificationPacket>();
        notification->setType(CONTROLLER_INSTANTIATION_RESPONSE);
        notification->setResult(result);
        if(result){
            notification->setControllerId(controllerId);
            notification->setControllerAddress(address);
            notification->setControllerPort(port);
        }

        notification->setChunkLength(B(20));
        responsePacket->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
        responsePacket->insertAtBack(notification);

        // this must be sent to the producer app
        auto producerAppAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
        int producerAppPort = packet->getTag<L4PortInd>()->getSrcPort();

       EV << "MECPlatooningProducerApp::finalizeJoinRequest - sending CONTROLLER_INSTANTIATION_RESPONSE to producerApp with id [" << baseMsg->getProducerAppId() << "]"  << endl;

       platooningProducerAppsSocket_.sendTo(responsePacket, producerAppAddress, producerAppPort);
       delete packet;
       return;
    }

    auto consumerAppAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
    int consumerAppPort = packet->getTag<L4PortInd>()->getSrcPort();

    EV << "MECPlatooningProducerApp::finalizeJoinRequest - sending "<< result << " response to MecConsumerApp with id [" << baseMsg->getConsumerAppId() << "]"  << endl;

    platooningConsumerAppsSocket_.sendTo(responsePacket, consumerAppAddress, consumerAppPort);

    delete packet;
}

MecAppInstanceInfo* MECPlatooningProducerApp::instantiateLocalPlatoon(Coord& direction, int index)
{
    Coord dir = direction;
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
    string path = string("simu5g.apps.mec.Plaas_framework.");

    stringstream moduleType;
    moduleType << path << contName;

    createAppMsg->setUeAppID(mecPlatoonControllerAppIdCounter_++);
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
        confMsg->setLateralController(par("lateralController").stringValue());
        confMsg->setLateralControlPeriod(par("lateralControlPeriod").doubleValue());
        confMsg->setLateralUpdatePositionPeriod(par("lateralUpdatePositionPeriod").doubleValue());
        confMsg->setLongitudinalControlPeriod(par("longitudinalControlPeriod").doubleValue());
        confMsg->setLongitudinalUpdatePositionPeriod(par("longitudinalUpdatePositionPeriod").doubleValue());
        confMsg->setOffset(par("offset").doubleValue());
        confMsg->setAdjustPosition(par("adjustPosition").boolValue());
        confMsg->setSendBulk(par("sendBulk").boolValue());

        confMsg->setChunkLength(inet::B( 2*sizeof(double) + 2*sizeof(int) + 20 ));
        confMsg->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
        pkt->insertAtBack(confMsg);

        platooningControllerAppsSocket_.sendTo(pkt, appInfo->endPoint.addr, appInfo->endPoint.port);
    }
    return appInfo;
}

void MECPlatooningProducerApp::controlHeartbeats()
{
    /*
     *
     * TODO this method check how long is the last heartbeat from a controller.
     * If timestamp is too old, remove the controller and notify all the associate consumer apps
     */
}

void MECPlatooningProducerApp::established(int connId)
{
}






