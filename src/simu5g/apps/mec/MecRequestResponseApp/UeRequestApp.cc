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

#include "simu5g/apps/mec/MecRequestResponseApp/UeRequestApp.h"

#include <fstream>
#include <math.h>

#include <inet/common/TimeTag_m.h>
#include <inet/common/packet/chunk/BytesChunk.h>
#include <inet/networklayer/common/L3AddressTag_m.h>
#include <inet/transportlayer/common/L4PortTag_m.h>

#include "simu5g/apps/mec/DeviceApp/messages/DeviceAppPacket_m.h"
#include "simu5g/apps/mec/DeviceApp/messages/DeviceAppPacket_Types.h"
#include "simu5g/apps/mec/MecRequestResponseApp/packets/RequestResponsePacket_m.h"
#include "simu5g/mec/platform/MeAppPacket_Types.h"

namespace simu5g {

using namespace inet;
using namespace std;

Define_Module(UeRequestApp);

// register signals for stats
simsignal_t UeRequestApp::processingTimeSignal_ = registerSignal("processingTime");
simsignal_t UeRequestApp::serviceResponseTimeSignal_ = registerSignal("serviceResponseTime");
simsignal_t UeRequestApp::upLinkTimeSignal_ = registerSignal("upLinkTime");
simsignal_t UeRequestApp::downLinkTimeSignal_ = registerSignal("downLinkTime");
simsignal_t UeRequestApp::responseTimeSignal_ = registerSignal("responseTime");

UeRequestApp::~UeRequestApp()
{
    cancelAndDelete(selfStart_);
    cancelAndDelete(selfStop_);
    cancelAndDelete(unBlockingMsg_);
    cancelAndDelete(sendRequest_);
}

void UeRequestApp::initialize(int stage)
{
    EV << "UeRequestApp::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;

    sno_ = 0;

    //retrieve parameters
    requestPacketSize_ = B(par("requestPacketSize"));
    requestPeriod_ = par("period");

    localPort_ = par("localPort");
    deviceAppPort_ = par("deviceAppPort");

    const char *deviceAppAddressStr = par("deviceAppAddress").stringValue();
    deviceAppAddress_ = inet::L3AddressResolver().resolve(deviceAppAddressStr);

    //binding socket
    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort_);

    int tos = par("tos");
    if (tos != -1)
        socket.setTos(tos);

    mecAppName = par("mecAppName").stringValue();

    //initializing the auto-scheduling messages
    selfStart_ = new cMessage("selfStart", KIND_SELF_START);
    selfStop_ = new cMessage("selfStop", KIND_SELF_STOP);
    sendRequest_ = new cMessage("sendRequest", KIND_SEND_REQUEST);
    unBlockingMsg_ = new cMessage("unBlockingMsg", KIND_UN_BLOCKING_MSG);

    //starting UeRequestApp
    simtime_t startTime = par("startTime");
    EV << "UeRequestApp::initialize - starting sendStartMEWarningAlertApp() in " << startTime << " seconds " << endl;
    scheduleAt(simTime() + startTime, selfStart_);

    //testing
    EV << "UeRequestApp::initialize - binding to port: local:" << localPort_ << " , dest:" << deviceAppPort_ << endl;
}

void UeRequestApp::handleMessage(cMessage *msg)
{
    EV << "UeRequestApp::handleMessage" << endl;
    // Sender Side
    if (msg->isSelfMessage()) {
        switch (msg->getKind()) {
            case KIND_SELF_START:
                sendStartMecRequestApp();
                break;
            case KIND_SELF_STOP:
                sendStopApp();
                break;
            case KIND_SEND_REQUEST:
            case KIND_UN_BLOCKING_MSG:
                sendRequest();
                break;
            default:
                throw cRuntimeError("UeRequestApp::handleMessage - \tWARNING: Unrecognized self message");
        }
    }
    // Receiver Side
    else {
        inet::Packet *packet = check_and_cast<inet::Packet *>(msg);
        inet::L3Address ipAdd = packet->getTag<L3AddressInd>()->getSrcAddress();

        /*
         * From Device app
         * device app usually runs in the UE (loopback), but it could also run in other places
         */
        if (ipAdd == deviceAppAddress_ || ipAdd == inet::L3Address("127.0.0.1")) { // dev app
            auto mePkt = packet->peekAtFront<DeviceAppPacket>();
            if (!strcmp(mePkt->getType(), ACK_START_MECAPP))
                handleAckStartMecRequestApp(msg);
            else if (!strcmp(mePkt->getType(), ACK_STOP_MECAPP))
                handleAckStopMecRequestApp(msg);
            else
                throw cRuntimeError("UeRequestApp::handleMessage - \tFATAL! Error, DeviceAppPacket type %s not recognized", mePkt->getType());
        }
        // From MEC application
        else {
            auto mePkt = packet->peekAtFront<RequestResponseAppPacket>();
            if (mePkt->getType() == MECAPP_RESPONSE)
                recvResponse(msg);
            else if (mePkt->getType() == UEAPP_ACK_STOP)
                handleStopApp(msg);
            else
                throw cRuntimeError("UeRequestApp::handleMessage - \tFATAL! Error, RequestAppPacket type %d not recognized", mePkt->getType());
        }
    }
}

void UeRequestApp::finish()
{
}

void UeRequestApp::sendStartMecRequestApp()
{
    EV << "UeRequestApp::sendStartMecRequestApp - Sending " << START_MEAPP << " type RequestPacket\n";

    inet::Packet *packet = new inet::Packet("RequestAppStart");
    auto start = inet::makeShared<DeviceAppStartPacket>();

    //instantiation requirements and info
    start->setType(START_MECAPP);
    start->setMecAppName(mecAppName.c_str());
    start->setChunkLength(inet::B(2 + mecAppName.size() + 1));
    start->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    packet->insertAtBack(start);

    socket.sendTo(packet, deviceAppAddress_, deviceAppPort_);

    //rescheduling
    scheduleAt(simTime() + 0.5, selfStart_);
}

void UeRequestApp::sendStopMecRequestApp()
{
    EV << "UeRequestApp::sendStopMecRequestApp - Sending " << STOP_MEAPP << " type RequestPacket\n";

    inet::Packet *packet = new inet::Packet("DeviceAppStopPacket");
    auto stop = inet::makeShared<DeviceAppStopPacket>();

    //termination requirements and info
    stop->setType(STOP_MECAPP);
    stop->setChunkLength(inet::B(10));
    stop->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    packet->insertAtBack(stop);

    socket.sendTo(packet, deviceAppAddress_, deviceAppPort_);

    if (sendRequest_->isScheduled()) {
        cancelEvent(sendRequest_);
    }

    //rescheduling
    if (selfStop_->isScheduled())
        cancelEvent(selfStop_);
    scheduleAt(simTime() + 0.5, selfStop_);
}

void UeRequestApp::handleAckStartMecRequestApp(cMessage *msg)
{
    EV << "UeRequestApp::handleAckStartMecRequestApp - Received Start ACK packet" << endl;
    inet::Packet *packet = check_and_cast<inet::Packet *>(msg);
    auto pkt = packet->peekAtFront<DeviceAppStartAckPacket>();

    if (pkt->getResult() == true) {
        mecAppAddress_ = L3AddressResolver().resolve(pkt->getIpAddress());
        mecAppPort_ = pkt->getPort();
        EV << "UeRequestApp::handleAckStartMecRequestApp - Received " << pkt->getType() << " type RequestPacket. mecApp instance is at: " << mecAppAddress_ << ":" << mecAppPort_ << endl;
        cancelEvent(selfStart_);
        //scheduling sendStopMEWarningAlertApp()
        if (!selfStop_->isScheduled()) {
            simtime_t stopTime = par("stopTime");
            scheduleAt(simTime() + stopTime, selfStop_);
            EV << "UeRequestApp::handleAckStartMecRequestApp - Starting sendStopMecRequestApp() in " << stopTime << " seconds " << endl;
        }
        //send the first reuqest to the MEC app
        sendRequest();
    }
    else {
        EV << "UeRequestApp::handleAckStartMecRequestApp - MEC application cannot be instantiated! Reason: " << pkt->getReason() << endl;
        simtime_t startTime = par("startTime");
        EV << "UeRequestApp::initialize - starting sendStartMEWarningAlertApp() in " << startTime << " seconds " << endl;
        if (!selfStart_->isScheduled())
            scheduleAt(simTime() + startTime, selfStart_);
    }

    delete packet;
}

void UeRequestApp::handleAckStopMecRequestApp(cMessage *msg)
{
    EV << "UeRequestApp::handleAckStopMecRequestApp - Received Stop ACK packet" << endl;

    inet::Packet *packet = check_and_cast<inet::Packet *>(msg);
    auto pkt = packet->peekAtFront<DeviceAppStopAckPacket>();

    EV << "UeRequestApp::handleAckStopMecRequestApp - Received " << pkt->getType() << " type RequestPacket with result: " << pkt->getResult() << endl;
    if (pkt->getResult() == false)
        EV << "Reason: " << pkt->getReason() << endl;

    cancelEvent(selfStop_);
}

void UeRequestApp::sendRequest()
{
    EV << "UeRequestApp::sendRequest()" << endl;
    inet::Packet *pkt = new inet::Packet("RequestResponseAppPacket");
    auto req = inet::makeShared<RequestResponseAppPacket>();
    req->setType(UEAPP_REQUEST);
    req->setSno(sno_++);
    req->setRequestSentTimestamp(simTime());
    req->setChunkLength(requestPacketSize_);
    req->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    pkt->insertAtBack(req);

    start_ = simTime();

    socket.sendTo(pkt, mecAppAddress_, mecAppPort_);
}

void UeRequestApp::handleStopApp(cMessage *msg)
{
    EV << "UeRequestApp::handleStopApp" << endl;
    inet::Packet *packet = check_and_cast<inet::Packet *>(msg);
    auto res = packet->peekAtFront<RequestResponseAppPacket>();

    sendStopMecRequestApp();
}

void UeRequestApp::sendStopApp()
{
    inet::Packet *pkt = new inet::Packet("RequestResponseAppPacket");
    auto req = inet::makeShared<RequestResponseAppPacket>();
    req->setType(UEAPP_STOP);
    req->setRequestSentTimestamp(simTime());
    req->setChunkLength(requestPacketSize_);
    req->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    pkt->insertAtBack(req);

    socket.sendTo(pkt, mecAppAddress_, mecAppPort_);
}

void UeRequestApp::recvResponse(cMessage *msg)
{
    EV << "UeRequestApp::recvResponse" << endl;
    inet::Packet *packet = check_and_cast<inet::Packet *>(msg);
    auto res = packet->peekAtFront<RequestResponseAppPacket>();

    simtime_t upLinkDelay = res->getRequestArrivedTimestamp() - res->getRequestSentTimestamp();
    simtime_t downLinkDelay = simTime() - res->getResponseSentTimestamp();
    simtime_t respTime = simTime() - res->getRequestSentTimestamp();

    EV << "UeRequestApp::recvResponse - message with sno [" << res->getSno() << "] " <<
        "upLinkDelay [" << upLinkDelay << "ms]\t" <<
        "downLinkDelay [" << downLinkDelay << "ms]\t" <<
        "processingTime [" << res->getProcessingTime() << "ms]\t" <<
        "serviceResponseTime [" << res->getServiceResponseTime() << "ms]\t" <<
        "responseTime [" << respTime << "ms]" << endl;

    //emit stats
    emit(upLinkTimeSignal_, upLinkDelay);
    emit(downLinkTimeSignal_, downLinkDelay);
    emit(processingTimeSignal_, res->getProcessingTime());
    emit(serviceResponseTimeSignal_, res->getServiceResponseTime());
    emit(responseTimeSignal_, respTime);

    delete packet;

    if (unBlockingMsg_->isScheduled())
        cancelEvent(unBlockingMsg_);
    if (!sendRequest_->isScheduled())
        scheduleAt(simTime() + requestPeriod_, sendRequest_);
}

} //namespace

