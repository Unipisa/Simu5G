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

#include "apps/mec/FLaaS/UEFLApp/UEFLApp.h"
#include "apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket_m.h"
#include "apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket_Types.h"
#include "nodes/mec/MECPlatform/MEAppPacket_Types.h"

#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/chunk/BytesChunk.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"

#include "apps/mec/FLaaS/packets/FLaasMsgs_m.h"
#include "apps/mec/FLaaS/FLaaSUtils.h"


#include <fstream>

#include <math.h>

using namespace inet;
using namespace std;

Define_Module(UEFLApp);

UEFLApp::UEFLApp()
{
    sendDataMsg_ = nullptr;
    selfStart_ = nullptr;
    selfStop_ = nullptr;


}

UEFLApp::~UEFLApp()
{
    cancelAndDelete(selfStart_);
    cancelAndDelete(selfStop_);
    cancelAndDelete(sendDataMsg_);

}

void UEFLApp::initialize(int stage)
{
    EV << "UEFLApp::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    localPort_ = par("localPort");
    deviceAppPort_ = par("deviceAppPort");

    char* deviceAppAddressStr = (char*)par("deviceAppAddress").stringValue();
    deviceAppAddress_ = inet::L3AddressResolver().resolve(deviceAppAddressStr);

    //binding socket
    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort_);

    int tos = par("tos");
    if (tos != -1)
        socket.setTos(tos);

    mecAppName = par("mecAppName").stringValue();

    //initializing the auto-scheduling messages
    selfStart_ = new cMessage("selfStart");
    selfStop_ = new cMessage("selfStop");
    sendDataMsg_ = new cMessage("sendDataMsg");
    dataPeriod_ = par("dataPeriod");

    //starting UEFLApp
    simtime_t startTime = par("startTime");
    EV << "UEFLApp::initialize - starting sendStartMEWarningAlertApp() in " << startTime << " seconds " << endl;
    scheduleAt(simTime() + startTime, selfStart_);

    //testing
    EV << "UEFLApp::initialize - binding to port: local:" << localPort_ << " , dest:" << deviceAppPort_ << endl;

    int dep = par("learnerDeployment");
    deployment_ = (dep == 0)? ON_UE : ON_MEC_HOST;


}

void UEFLApp::handleMessage(cMessage *msg)
{
    EV << "UEFLApp::handleMessage" << endl;
    // Sender Side
    if (msg->isSelfMessage())
    {
        if(!strcmp(msg->getName(), "selfStart"))
            sendStartMECRequestApp();
        else if(!strcmp(msg->getName(), "selfStop"))
            sendStopApp();
        else if(!strcmp(msg->getName(), "sendDataMsg"))
            sendDataMsg();
        else
            throw cRuntimeError("UEFLApp::handleMessage - \tWARNING: Unrecognized self message");
    }
    // Receiver Side
    else
    {
        inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
        inet::L3Address ipAdd = packet->getTag<L3AddressInd>()->getSrcAddress();

        /*
         * From Device app
         * device app usually runs in the UE (loopback), but it could also run in other places
         */
        if(ipAdd == deviceAppAddress_ || ipAdd == inet::L3Address("127.0.0.1")) // dev app
        {
            auto mePkt = packet->peekAtFront<DeviceAppPacket>();

            if (mePkt == 0)
                throw cRuntimeError("UEFLApp::handleMessage - \tFATAL! Error when casting to DeviceAppPacket");

            if( !strcmp(mePkt->getType(), ACK_START_MECAPP) )
                handleAckStartMECRequestApp(msg);
            else if(!strcmp(mePkt->getType(), ACK_STOP_MECAPP))
                handleAckStopMECRequestApp(msg);
            else
                throw cRuntimeError("UEFLApp::handleMessage - \tFATAL! Error, DeviceAppPacket type %s not recognized", mePkt->getType());
        }
        // From MEC application
        else
        {

        }
    }
}

void UEFLApp::finish()
{
}

void UEFLApp::sendStartMECRequestApp()
{
    EV << "UEFLApp::sendStartMECRequestApp - Sending " << START_MEAPP <<" type RequestPacket\n";

    inet::Packet* packet = new inet::Packet("RequestAppStart");
    auto start = inet::makeShared<DeviceAppStartPacket>();

    //instantiation requirements and info
    start->setType(START_MECAPP);
    start->setMecAppName(mecAppName.c_str());
    start->setChunkLength(inet::B(2+mecAppName.size()+1));
    start->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    packet->insertAtBack(start);

    socket.sendTo(packet, deviceAppAddress_, deviceAppPort_);

    //rescheduling
    scheduleAt(simTime() + 0.5, selfStart_);
}

void UEFLApp::sendStopMECRequestApp()
{
    EV << "UEFLApp::sendStopMECRequestApp - Sending " << STOP_MEAPP <<" type RequestPacket\n";

    inet::Packet* packet = new inet::Packet("DeviceAppStopPacket");
    auto stop = inet::makeShared<DeviceAppStopPacket>();

    //termination requirements and info
    stop->setType(STOP_MECAPP);
    stop->setChunkLength(inet::B(10));
    stop->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    packet->insertAtBack(stop);

    socket.sendTo(packet, deviceAppAddress_, deviceAppPort_);

    if(sendRequest_->isScheduled())
    {
        cancelEvent(sendRequest_);
    }

    //rescheduling
    if(selfStop_->isScheduled())
        cancelEvent(selfStop_);
    scheduleAt(simTime() + 0.5, selfStop_);
}

void UEFLApp::handleAckStartMECRequestApp(cMessage* msg)
{
    EV << "UEFLApp::handleAckStartMECRequestApp - Received Start ACK packet" << endl;
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto pkt = packet->peekAtFront<DeviceAppStartAckPacket>();

    if(pkt->getResult() == true)
    {
        mecAppAddress_ = L3AddressResolver().resolve(pkt->getIpAddress());
        mecAppPort_ = pkt->getPort();
        EV << "UEFLApp::handleAckStartMECRequestApp - Received " << pkt->getType() << " type RequestPacket. mecApp instance is at: "<< mecAppAddress_<< ":" << mecAppPort_ << endl;
        cancelEvent(selfStart_);
        //scheduling sendStopMEWarningAlertApp()
        if(!selfStop_->isScheduled()){
            simtime_t  stopTime = par("stopTime");
            scheduleAt(simTime() + stopTime, selfStop_);
            EV << "UEFLApp::handleAckStartMECRequestApp - Starting sendStopMECRequestApp() in " << stopTime << " seconds " << endl;
        }
        // send where to deploy the learner
        inet::Packet* pkt = new inet::Packet("LearnerDeploymentPacket");
        auto learnerMsg = inet::makeShared<LearnerDeploymentPacket>();
        if(deployment_ == ON_UE)
        {
            learnerMsg->setDeploymentPosition(ON_UE);
            learnerMsg->setUeModuleName(getParentModule()->getFullPath().c_str());
            learnerMsg->setIpAddress(getParentModule()->getFullName());
            learnerMsg->setLearnerModuleName(par("learnerModuleName"));
            learnerMsg->setPort(par("learnerPort"));
            learnerMsg->setChunkLength(inet::B(20));
        }
        else
        {
            learnerMsg->setDeploymentPosition(ON_MEC_HOST);
            learnerMsg->setChunkLength(inet::B(4));

            //start sending data for the training
            scheduleAfter(dataPeriod_, sendDataMsg_);
        }
        learnerMsg->setType(LEARNER_DEPLOYMENT);
        learnerMsg->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

        pkt->insertAtBack(learnerMsg);
        socket.sendTo(pkt, mecAppAddress_, mecAppPort_);
        EV << "UEFLApp::handleAckStartMECRequestApp - Sent deployment message to LM local manger" << endl;
    }
    else
    {
        EV << "UEFLApp::handleAckStartMECRequestApp - MEC application cannot be instantiated! Reason: " << pkt->getReason() << endl;
        simtime_t startTime = par("startTime");
        EV << "UEFLApp::initialize - starting sendStartMEWarningAlertApp() in " << startTime << " seconds " << endl;
        if(!selfStart_->isScheduled())
            scheduleAt(simTime() + startTime, selfStart_);
    }



    delete packet;
}

void UEFLApp::handleAckStopMECRequestApp(cMessage* msg)
{
    EV << "UEFLApp::handleAckStopMECRequestApp - Received Stop ACK packet" << endl;

    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto pkt = packet->peekAtFront<DeviceAppStopAckPacket>();

    EV << "UEFLApp::handleAckStopMECRequestApp - Received " << pkt->getType() << " type RequestPacket with result: "<< pkt->getResult() << endl;
    if(pkt->getResult() == false)
        EV << "Reason: "<< pkt->getReason() << endl;

    cancelEvent(selfStop_);
}


void UEFLApp::sendDataMsg()
{
    EV << "UEFLApp::sendDataMsg()" << endl;
    inet::Packet* pkt = new inet::Packet("DataForTrainingPacket");
    auto req = inet::makeShared<DataForTrainingPacket>();
    req->setType(DATA_MSG);
    double dimension = par("dataDimension");
    req->setDimension(dimension);
    req->setChunkLength(inet::B(dimension));
    req->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    pkt->insertAtBack(req);
    socket.sendTo(pkt, mecAppAddress_ , mecAppPort_);
    scheduleAfter(dataPeriod_, sendDataMsg_);

}


void UEFLApp::handleStopApp(cMessage* msg)
{

}

void UEFLApp::sendStopApp()
{


}

void UEFLApp::recvResponse(cMessage* msg)
{
    EV << "UEFLApp::recvResponse" << endl;

}


