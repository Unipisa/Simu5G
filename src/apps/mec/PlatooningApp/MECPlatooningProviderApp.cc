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

#include "apps/mec/PlatooningApp/MECPlatooningProviderApp.h"
#include "apps/mec/PlatooningApp/platoonSelection/PlatoonSelectionSample.h"
#include "apps/mec/PlatooningApp/platoonController/PlatoonControllerSample.h"
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

Define_Module(MECPlatooningProviderApp);

using namespace inet;
using namespace omnetpp;

MECPlatooningProviderApp::MECPlatooningProviderApp(): MecAppBase()
{
    platoonSelection_ = nullptr;
    nextControllerIndex_ = 0;
}

MECPlatooningProviderApp::~MECPlatooningProviderApp()
{
    delete platoonSelection_;
}


void MECPlatooningProviderApp::initialize(int stage)
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

    EV << "MECPlatooningProviderApp::initialize - Mec application "<< getClassName() << " with mecAppId["<< mecAppId << "] has started!" << endl;
}

void MECPlatooningProviderApp::handleMessage(cMessage *msg)
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
                throw cRuntimeError("MECPlatooningProviderApp::handleUeMessage - packet not recognized");
            }
            return;
        }
    }
    MecAppBase::handleMessage(msg);

}

void MECPlatooningProviderApp::finish()
{
    MecAppBase::finish();
    EV << "MECPlatooningProviderApp::finish()" << endl;
    if(gate("socketOut")->isConnected())
    {

    }
}

void MECPlatooningProviderApp::handleSelfMessage(cMessage *msg)
{
    if (strcmp(msg->getName(), "ControllerTimer") == 0)
    {
        // a controller timer has been fired
        handleControllerTimer(msg);
    }
}

void MECPlatooningProviderApp::handleMp1Message()
{
}

void MECPlatooningProviderApp::handleServiceMessage()
{
}

void MECPlatooningProviderApp::handleRegistrationRequest(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    L3Address mecAppAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
    int mecAppPort = packet->getTag<L4PortInd>()->getSrcPort();

    auto registrationReq = packet->removeAtFront<PlatooningRegistrationPacket>();
    int mecAppId = registrationReq->getMecAppId();

    EV << "MECPlatooningProviderApp::handleRegistrationRequest - Received registration request from MEC app " << mecAppId << " [" << mecAppAddress.str() << ":" << mecAppPort << "]" << endl;

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

    EV << "MECPlatooningProviderApp::handleRegistrationRequest - Sending response to MEC app " << mecAppId << " [" << mecAppAddress.str() << ":" << mecAppPort << "]" << endl;

    delete packet;
}

void MECPlatooningProviderApp::handleJoinPlatoonRequest(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto joinReq = packet->removeAtFront<PlatooningJoinPacket>();
    int mecAppId = joinReq->getMecAppId();

    EV << "MECPlatooningProviderApp::sendJoinPlatoonRequest - Received join request from MEC App " << mecAppId << endl;

    // find the most suitable platoonController for this new request
    PlatoonControllerBase* platoonController;
    int selectedPlatoon = platoonSelection_->findBestPlatoon(platoonControllers_);
    if (selectedPlatoon < 0)
    {
        // if no active platoon managers can be used, create one
        platoonController = new PlatoonControllerSample(this, nextControllerIndex_);
        platoonControllers_[nextControllerIndex_] = platoonController;

        EV << "MECPlatooningProviderApp::sendJoinPlatoonRequest - MEC App " << mecAppId << " added to new platoon " << selectedPlatoon << endl;
        nextControllerIndex_++;
    }
    else if (platoonControllers_.find(selectedPlatoon) != platoonControllers_.end())
    {
        platoonController = platoonControllers_[selectedPlatoon];
        EV << "MECPlatooningProviderApp::sendJoinPlatoonRequest - MEC App " << mecAppId << " added to platoon " << selectedPlatoon << endl;
    }
    else
        throw cRuntimeError("MECPlatooningProviderApp::handleJoinPlatoonRequest - invalid platoon manager index[%d] - size[%d]\n", selectedPlatoon, (int)platoonControllers_.size());

    // add the member to the identified platoonController
    bool success = platoonController->addPlatoonMember(mecAppId);


    inet::Packet* responsePacket = new Packet (packet->getName());
    joinReq->setType(JOIN_RESPONSE);
    if (success)
    {
        // accept the request and send ACK
        joinReq->setResponse(true);
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
    EV << "MECPlatooningProviderApp::sendJoinPlatoonRequest - Join response sent to the UE" << endl;

    delete packet;
}

void MECPlatooningProviderApp::handleLeavePlatoonRequest(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto leaveReq = packet->removeAtFront<PlatooningLeavePacket>();
    int mecAppId = leaveReq->getMecAppId();

    EV << "MECPlatooningProviderApp::handleLeavePlatoonRequest - Received leave request from MEC app " << mecAppId << endl;

    // TODO retrieve the platoonController for this member
    PlatoonControllerBase* platoonController = platoonControllers_.at(0);
    if (platoonController == nullptr)
    {
        EV << "MECPlatooningProviderApp::handleLeavePlatoonRequest - the UE was not registered to any platoonController " << endl;
        return;
    }

    // remove the member from the identified platoonController
    bool success = platoonController->removePlatoonMember(mecAppId);

    EV << "MECPlatooningProviderApp::sendLeavePlatoonRequest - MEC App " << mecAppId << " left platoon 0" << endl;

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
    EV << "MECPlatooningProviderApp::handleLeavePlatoonRequest() - Leave response sent to the UE" << endl;

    delete packet;
}

void MECPlatooningProviderApp::startControllerTimer(int controllerIndex, double controlPeriod)
{
    if (controlPeriod < 0.0)
        throw cRuntimeError("MECPlatooningProviderApp::startControllerTimer - control period not valid[%f]", controlPeriod);
    if (platoonControllers_.find(controllerIndex) == platoonControllers_.end())
        throw cRuntimeError("MECPlatooningProviderApp::startControllerTimer - controller with index %d not found", controllerIndex);

    // if a timer was already set for this controller, cancel it
    if (activeControllerTimer_.find(controllerIndex) != activeControllerTimer_.end())
        cancelAndDelete(activeControllerTimer_.at(controllerIndex));

    //schedule timer
    ControllerTimer* newTimer = new ControllerTimer("ControllerTimer");
    newTimer->setControllerIndex(controllerIndex);
    newTimer->setControlPeriod(controlPeriod);
    scheduleAt(simTime() + controlPeriod, newTimer);

    // store timer into the timers' data structure. This is necessary to retrieve the timer in case it needs to be canceled
    activeControllerTimer_[controllerIndex] = newTimer;

    EV << "MECPlatooningProviderApp::startControllerTimer() - New timer set for controller " << controllerIndex << " - period[" << controlPeriod << "s]" << endl;

}

void MECPlatooningProviderApp::stopControllerTimer(int controllerIndex)
{
    if (activeControllerTimer_.find(controllerIndex) == activeControllerTimer_.end())
        throw cRuntimeError("MECPlatooningProviderApp::stopControllerTimer - no active controller timer with index %d", controllerIndex);

    cancelAndDelete(activeControllerTimer_.at(controllerIndex));
    EV << "MECPlatooningProviderApp::startControllerTimer() - Timer for controller " << controllerIndex << "has been stopped" << endl;

}

void MECPlatooningProviderApp::handleControllerTimer(cMessage* msg)
{
    ControllerTimer* timer = check_and_cast<ControllerTimer*>(msg);
    int controllerIndex = timer->getControllerIndex();

    if (platoonControllers_.find(controllerIndex) == platoonControllers_.end())
        throw cRuntimeError("MECPlatooningProviderApp::handleControllerTimer - controller with index %d not found", controllerIndex);

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
                throw cRuntimeError("MECPlatooningProviderApp::handleControllerTimer - endpoint for MEC app %d not found.", mecAppId);

            MECAppEndpoint endpoint = mecAppEndpoint_.at(mecAppId);

            inet::Packet* pkt = new inet::Packet("PlatooningInfoPacket");
            auto cmd = inet::makeShared<PlatooningInfoPacket>();
            cmd->setType(PLATOON_CMD);
            cmd->setNewAcceleration(newAcceleration);
            cmd->setChunkLength(inet::B( sizeof(double) ));
            cmd->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
            pkt->insertAtBack(cmd);

            platooningConsumerAppsSocket.sendTo(pkt, endpoint.address, endpoint.port);
            EV << "MECPlatooningProviderApp::handleControllerTimer() - Sent new acceleration value[" << newAcceleration << "] to MEC App " << it->first << " [" << endpoint.address.str() << ":" << endpoint.port << "]" << endl;
        }
        delete cmdList;
    }

    // re-schedule controller timer
    scheduleAt(simTime() + timer->getControlPeriod(), timer);
}

void MECPlatooningProviderApp::established(int connId)
{
    if(connId == mp1Socket_.getSocketId())
    {

    }
    else if (connId == serviceSocket_.getSocketId())
    {

    }
    else
    {
        throw cRuntimeError("MecAppBase::socketEstablished - Socket %d not recognized", connId);
    }
}






