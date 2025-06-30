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

#include "apps/mec/RnisTestApp/UeRnisTestApp.h"

#include <fstream>

#include <inet/common/TimeTag_m.h>
#include <inet/common/packet/chunk/BytesChunk.h>
#include <inet/networklayer/common/L3AddressTag_m.h>
#include <inet/transportlayer/common/L4PortTag_m.h>

#include "apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket_m.h"
#include "apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket_Types.h"
#include "apps/mec/RnisTestApp/packets/RnisTestAppPacket_m.h"
#include "apps/mec/RnisTestApp/packets/RnisTestAppPacket_Types.h"

namespace simu5g {

using namespace std;
using namespace inet;

Define_Module(UeRnisTestApp);

UeRnisTestApp::~UeRnisTestApp() {
    cancelAndDelete(selfStart_);
    cancelAndDelete(selfStop_);
    cancelAndDelete(selfMecAppStart_);
}

void UeRnisTestApp::initialize(int stage)
{
    EV << "UeRnisTestApp::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;

    log = par("logger").boolValue();

    //retrieve parameters
    period_ = par("period");
    int localPort = par("localPort");
    deviceAppPort_ = par("deviceAppPort");
    const char *sourceSymbolicAddress = getParentModule()->getFullName();
    const char *deviceSymbolicAppAddress = par("deviceAppAddress").stringValue();
    deviceAppAddress_ = inet::L3AddressResolver().resolve(deviceSymbolicAppAddress);

    //binding socket
    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort);

    int tos = par("tos");
    if (tos != -1)
        socket.setTos(tos);

    mecAppName = par("mecAppName").stringValue();

    //initializing the auto-scheduling messages
    selfStart_ = new cMessage("selfStart", KIND_SELF_START);
    selfStop_ = new cMessage("selfStop", KIND_SELF_STOP);
    selfMecAppStart_ = new cMessage("selfMecAppStart", KIND_SELF_MEC_APP_START);

    //starting UeRnisTestApp
    simtime_t startTime = par("startTime");
    EV << "UeRnisTestApp::initialize - starting sendStartMecApp() in " << startTime << " seconds " << endl;
    scheduleAt(simTime() + startTime, selfStart_);

    //testing
    EV << "UeRnisTestApp::initialize - sourceAddress: " << sourceSymbolicAddress << " [" << inet::L3AddressResolver().resolve(sourceSymbolicAddress).str() << "]" << endl;
    EV << "UeRnisTestApp::initialize - destAddress: " << deviceSymbolicAppAddress << " [" << deviceAppAddress_.str() << "]" << endl;
    EV << "UeRnisTestApp::initialize - binding to port: local:" << localPort << " , dest:" << deviceAppPort_ << endl;
}

void UeRnisTestApp::handleMessage(cMessage *msg)
{
    EV << "UeRnisTestApp::handleMessage" << endl;
    // Sender Side
    if (msg->isSelfMessage()) {
        switch (msg->getKind()) {
            case KIND_SELF_START:
                sendStartMecApp();
                break;
            case KIND_SELF_STOP:
                sendStopMecApp();
                break;
            case KIND_SELF_MEC_APP_START:
                sendMessageToMecApp();
                scheduleAt(simTime() + period_, selfMecAppStart_);
                break;
            default:
                throw cRuntimeError("UeRnisTestApp::handleMessage - \tWARNING: Unrecognized self message");
        }
    }
    // Receiver Side
    else {
        inet::Packet *packet = check_and_cast<inet::Packet *>(msg);

        inet::L3Address ipAdd = packet->getTag<L3AddressInd>()->getSrcAddress();
        // int port = packet->getTag<L4PortInd>()->getSrcPort();

        /*
         * From Device app
         * The device app usually runs in the UE (loopback), but it could also run in other places
         */
        if (ipAdd == deviceAppAddress_ || ipAdd == inet::L3Address("127.0.0.1")) { // device app
            auto mePkt = packet->peekAtFront<DeviceAppPacket>();
            if (!strcmp(mePkt->getType(), ACK_START_MECAPP)) handleAckStartMecApp(msg);

            else if (!strcmp(mePkt->getType(), ACK_STOP_MECAPP)) handleAckStopMecApp(msg);

            else {
                throw cRuntimeError("UeRnisTestApp::handleMessage - \tFATAL! Error, DeviceAppPacket type %s not recognized", mePkt->getType());
            }
        }
        // From MEC application
        else {
            auto mePkt = packet->peekAtFront<RnisTestAppPacket>();
            if (!strcmp(mePkt->getType(), RNIS_INFO)) handleInfoMecApp(msg);
            else if (!strcmp(mePkt->getType(), START_QUERY_RNIS_NACK)) {
                EV << "UeRnisTestApp::handleMessage - MEC app did not start correctly, trying to start again" << endl;
            }
            else if (!strcmp(mePkt->getType(), START_QUERY_RNIS_ACK)) {
                EV << "UeRnisTestApp::handleMessage - MEC app started correctly" << endl;
                if (selfMecAppStart_->isScheduled()) {
                    cancelEvent(selfMecAppStart_);
                }
            }
            else {
                throw cRuntimeError("UeRnisTestApp::handleMessage - \tFATAL! Error, RnisTestAppPacket type %s not recognized", mePkt->getType());
            }
        }
        delete msg;
    }
}

void UeRnisTestApp::finish()
{
}

/*
 * -----------------------------------------------Sender Side------------------------------------------
 */
void UeRnisTestApp::sendStartMecApp()
{
    EV << "UeRnisTestApp::sendStartMecApp - Sending " << START_MECAPP << " type packet\n";

    inet::Packet *packet = new inet::Packet("DeviceAppStartPacket");
    auto start = inet::makeShared<DeviceAppStartPacket>();

    //instantiation requirements and info
    start->setType(START_MECAPP);
    start->setMecAppName(mecAppName.c_str());
    start->setChunkLength(inet::B(2 + mecAppName.size() + 1));
    start->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(start);
    socket.sendTo(packet, deviceAppAddress_, deviceAppPort_);

    if (log) {
        ofstream myfile;
        myfile.open("example.txt", ios::app);
        if (myfile.is_open()) {
            myfile << "[" << NOW << "] UeRnisTestApp - UE sent start message to the Device App \n";
            myfile.close();
        }
    }

    //rescheduling
    scheduleAt(simTime() + period_, selfStart_);
}

void UeRnisTestApp::sendStopMecApp()
{
    EV << "UeRnisTestApp::sendStopMecApp - Sending " << STOP_MECAPP << " type packet\n";

    inet::Packet *packet = new inet::Packet("DeviceAppStopPacket");
    auto stop = inet::makeShared<DeviceAppStopPacket>();

    //termination requirements and info
    stop->setType(STOP_MECAPP);
    stop->setChunkLength(inet::B(10));
    stop->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(stop);
    socket.sendTo(packet, deviceAppAddress_, deviceAppPort_);

    if (log) {
        ofstream myfile;
        myfile.open("example.txt", ios::app);
        if (myfile.is_open()) {
            myfile << "[" << NOW << "] UeRnisTestApp - UE sent stop message to the Device App \n";
            myfile.close();
        }
    }

    //rescheduling
    if (selfStop_->isScheduled())
        cancelEvent(selfStop_);
    scheduleAt(simTime() + period_, selfStop_);
}

/*
 * ---------------------------------------------Receiver Side------------------------------------------
 */
void UeRnisTestApp::handleAckStartMecApp(cMessage *msg)
{
    inet::Packet *packet = check_and_cast<inet::Packet *>(msg);
    auto pkt = packet->peekAtFront<DeviceAppStartAckPacket>();

    if (pkt->getResult() == true) {
        mecAppAddress_ = L3AddressResolver().resolve(pkt->getIpAddress());
        mecAppPort_ = pkt->getPort();
        EV << "UeRnisTestApp::handleAckStartMecApp - Received " << pkt->getType() << ". MecApp instance is at: " << mecAppAddress_ << ":" << mecAppPort_ << endl;
        cancelEvent(selfStart_);

        //scheduling sendStopMecApp()
        if (!selfStop_->isScheduled()) {
            simtime_t stopTime = par("stopTime");
            scheduleAt(simTime() + stopTime, selfStop_);
            EV << "UeRnisTestApp::handleAckStartMecApp - Starting sendStopMecApp() in " << stopTime << " seconds " << endl;
        }
    }
    else {
        EV << "UeRnisTestApp::handleAckStartMecApp - MEC application cannot be instantiated! Reason: " << pkt->getReason() << endl;
    }

    sendMessageToMecApp();
    scheduleAt(simTime() + period_, selfMecAppStart_);
}

void UeRnisTestApp::sendMessageToMecApp() {

    // send start monitoring message to the MEC application

    inet::Packet *pkt = new inet::Packet("RnisTestAppStartPacket");
    auto startMsg = inet::makeShared<RnisTestAppStartPacket>();
    startMsg->setType(START_QUERY_RNIS);
    startMsg->setPeriod(par("queryingPeriod"));
    startMsg->setChunkLength(inet::B(20));
    startMsg->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    pkt->insertAtBack(startMsg);

    if (log) {
        ofstream myfile;
        myfile.open("example.txt", ios::app);
        if (myfile.is_open()) {
            myfile << "[" << NOW << "] UeRnisTestApp - UE sent start subscription message to the MEC application \n";
            myfile.close();
        }
    }

    socket.sendTo(pkt, mecAppAddress_, mecAppPort_);
    EV << "UeRnisTestApp::sendMessageToMecApp() - start Message sent to the MEC app" << endl;
}

void UeRnisTestApp::handleInfoMecApp(cMessage *msg)
{
    inet::Packet *packet = check_and_cast<inet::Packet *>(msg);
    auto rnisInfo = packet->peekAtFront<RnisTestAppInfoPacket>();

    EV << "UeRnisTestApp::handleInfoMecApp - Received " << rnisInfo->getType() << " from MEC app:" << endl;

    // print received information
    nlohmann::json jsonBody = rnisInfo->getL2meas();
    EV << jsonBody.dump(4) << endl;

    if (log) {
        ofstream myfile;
        myfile.open("example.txt", ios::app);
        if (myfile.is_open()) {
            myfile << "[" << NOW << "] UeRnisTestApp - Received RNIS info from MEC app:\n";
            myfile << jsonBody.dump(4) << "\n";
            myfile.close();
        }
    }
}

void UeRnisTestApp::handleAckStopMecApp(cMessage *msg)
{
    inet::Packet *packet = check_and_cast<inet::Packet *>(msg);
    auto pkt = packet->peekAtFront<DeviceAppStopAckPacket>();

    EV << "UeRnisTestApp::handleAckStopMecApp - Received " << pkt->getType() << " with result: " << pkt->getResult() << endl;
    if (pkt->getResult() == false)
        EV << "Reason: " << pkt->getReason() << endl;

    cancelEvent(selfStop_);
}

} //namespace

