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

#include "simu5g/apps/mec/WarningAlert/UeWarningAlertApp.h"

#include <fstream>

#include <inet/common/TimeTag_m.h>
#include <inet/common/packet/chunk/BytesChunk.h>
#include <inet/networklayer/common/L3AddressTag_m.h>
#include <inet/transportlayer/common/L4PortTag_m.h>

#include "simu5g/apps/mec/DeviceApp/messages/DeviceAppPacket_m.h"
#include "simu5g/apps/mec/DeviceApp/messages/DeviceAppPacket_Types.h"
#include "simu5g/apps/mec/WarningAlert/packets/WarningAlertPacket_m.h"
#include "simu5g/apps/mec/WarningAlert/packets/WarningAlertPacket_Types.h"

namespace simu5g {

using namespace inet;
using namespace std;

Define_Module(UeWarningAlertApp);


UeWarningAlertApp::~UeWarningAlertApp() {
    cancelAndDelete(selfStart_);
    cancelAndDelete(selfStop_);
    cancelAndDelete(selfMecAppStart_);
}

void UeWarningAlertApp::initialize(int stage)
{
    EV << "UeWarningAlertApp::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;

    log = par("logger").boolValue();

    //retrieving car cModule
    ue = getContainingNode(this);

    //retrieve parameters
    size_ = par("packetSize");
    period_ = par("period");
    localPort_ = par("localPort");
    deviceAppPort_ = par("deviceAppPort");
    const char *sourceSimbolicAddress = ue->getFullName();
    const char *deviceSimbolicAppAddress_ = par("deviceAppAddress").stringValue();
    deviceAppAddress_ = inet::L3AddressResolver().resolve(deviceSimbolicAppAddress_);

    //binding socket
    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort_);

    int tos = par("tos");
    if (tos != -1)
        socket.setTos(tos);

    //retrieving mobility module
    mobility.reference(this, "mobilityModule", true);

    mecAppName = par("mecAppName").stringValue();

    //initializing the auto-scheduling messages
    selfStart_ = new cMessage("selfStart", KIND_SELF_START);
    selfStop_ = new cMessage("selfStop", KIND_SELF_STOP);
    selfMecAppStart_ = new cMessage("selfMecAppStart", KIND_SELF_MEC_APP_START);

    //starting UeWarningAlertApp
    simtime_t startTime = par("startTime");
    EV << "UeWarningAlertApp::initialize - starting sendStartMEWarningAlertApp() in " << startTime << " seconds " << endl;
    scheduleAt(simTime() + startTime, selfStart_);

    //testing
    EV << "UeWarningAlertApp::initialize - sourceAddress: " << sourceSimbolicAddress << " [" << inet::L3AddressResolver().resolve(sourceSimbolicAddress).str() << "]" << endl;
    EV << "UeWarningAlertApp::initialize - destAddress: " << deviceSimbolicAppAddress_ << " [" << deviceAppAddress_.str() << "]" << endl;
    EV << "UeWarningAlertApp::initialize - binding to port: local:" << localPort_ << " , dest:" << deviceAppPort_ << endl;
}

void UeWarningAlertApp::handleMessage(cMessage *msg)
{
    EV << "UeWarningAlertApp::handleMessage" << endl;
    // Sender Side
    if (msg->isSelfMessage()) {
        switch (msg->getKind()) {
            case KIND_SELF_START:
                sendStartMEWarningAlertApp();
                break;
            case KIND_SELF_STOP:
                sendStopMEWarningAlertApp();
                break;
            case KIND_SELF_MEC_APP_START:
                sendMessageToMecApp();
                scheduleAt(simTime() + period_, selfMecAppStart_);
                break;
            default:
                throw cRuntimeError("UeWarningAlertApp::handleMessage - \tWARNING: Unrecognized self message");
        }
    }
    // Receiver Side
    else {
        inet::Packet *packet = check_and_cast<inet::Packet *>(msg);

        inet::L3Address ipAdd = packet->getTag<L3AddressInd>()->getSrcAddress();
        // int port = packet->getTag<L4PortInd>()->getSrcPort();

        /*
         * From Device app
         * device app usually runs in the UE (loopback), but it could also run in other places
         */
        if (ipAdd == deviceAppAddress_ || ipAdd == inet::L3Address("127.0.0.1")) { // dev app
            auto mePkt = packet->peekAtFront<DeviceAppPacket>();
            if (!strcmp(mePkt->getType(), ACK_START_MECAPP)) handleAckStartMEWarningAlertApp(msg);
            else if (!strcmp(mePkt->getType(), ACK_STOP_MECAPP)) handleAckStopMEWarningAlertApp(msg);
            else {
                throw cRuntimeError("UeWarningAlertApp::handleMessage - \tFATAL! Error, DeviceAppPacket type %s not recognized", mePkt->getType());
            }
        }
        // From MEC application
        else {
            auto mePkt = packet->peekAtFront<WarningAppPacket>();
            if (!strcmp(mePkt->getType(), WARNING_ALERT)) handleInfoMEWarningAlertApp(msg);
            else if (!strcmp(mePkt->getType(), START_NACK)) {
                EV << "UeWarningAlertApp::handleMessage - MEC app did not start correctly, trying to start again" << endl;
            }
            else if (!strcmp(mePkt->getType(), START_ACK)) {
                EV << "UeWarningAlertApp::handleMessage - MEC app started correctly" << endl;
                if (selfMecAppStart_->isScheduled()) {
                    cancelEvent(selfMecAppStart_);
                }
            }
            else {
                throw cRuntimeError("UeWarningAlertApp::handleMessage - \tFATAL! Error, WarningAppPacket type %s not recognized", mePkt->getType());
            }
        }
        delete msg;
    }
}

void UeWarningAlertApp::finish()
{
}

/*
 * -----------------------------------------------Sender Side------------------------------------------
 */
void UeWarningAlertApp::sendStartMEWarningAlertApp()
{
    inet::Packet *packet = new inet::Packet("WarningAlertPacketStart");
    auto start = inet::makeShared<DeviceAppStartPacket>();

    //instantiation requirements and info
    start->setType(START_MECAPP);
    start->setMecAppName(mecAppName.c_str());
    //start->setMecAppProvider("lte.apps.mec.warningAlert_rest.MEWarningAlertApp_rest_External");

    start->setChunkLength(inet::B(2 + mecAppName.size() + 1));
    start->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(start);

    socket.sendTo(packet, deviceAppAddress_, deviceAppPort_);

    if (log) {
        ofstream myfile;
        myfile.open("example.txt", ios::app);
        if (myfile.is_open()) {
            myfile << "[" << NOW << "] UeWarningAlertApp - UE sent start message to the Device App \n";
            myfile.close();
        }
    }

    //rescheduling
    scheduleAt(simTime() + period_, selfStart_);
}

void UeWarningAlertApp::sendStopMEWarningAlertApp()
{
    EV << "UeWarningAlertApp::sendStopMEWarningAlertApp - Sending " << STOP_MEAPP << " type WarningAlertPacket\n";

    inet::Packet *packet = new inet::Packet("DeviceAppStopPacket");
    auto stop = inet::makeShared<DeviceAppStopPacket>();

    //termination requirements and info
    stop->setType(STOP_MECAPP);

    stop->setChunkLength(inet::B(size_));
    stop->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(stop);
    socket.sendTo(packet, deviceAppAddress_, deviceAppPort_);

    if (log) {
        ofstream myfile;
        myfile.open("example.txt", ios::app);
        if (myfile.is_open()) {
            myfile << "[" << NOW << "] UeWarningAlertApp - UE sent stop message to the Device App \n";
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
void UeWarningAlertApp::handleAckStartMEWarningAlertApp(cMessage *msg)
{
    inet::Packet *packet = check_and_cast<inet::Packet *>(msg);
    auto pkt = packet->peekAtFront<DeviceAppStartAckPacket>();

    if (pkt->getResult() == true) {
        mecAppAddress_ = L3AddressResolver().resolve(pkt->getIpAddress());
        mecAppPort_ = pkt->getPort();
        EV << "UeWarningAlertApp::handleAckStartMEWarningAlertApp - Received " << pkt->getType() << " type WarningAlertPacket. mecApp instance is at: " << mecAppAddress_ << ":" << mecAppPort_ << endl;
        cancelEvent(selfStart_);
        //scheduling sendStopMEWarningAlertApp()
        if (!selfStop_->isScheduled()) {
            simtime_t stopTime = par("stopTime");
            scheduleAt(simTime() + stopTime, selfStop_);
            EV << "UeWarningAlertApp::handleAckStartMEWarningAlertApp - Starting sendStopMEWarningAlertApp() in " << stopTime << " seconds " << endl;
        }
        sendMessageToMecApp();
        scheduleAt(simTime() + period_, selfMecAppStart_);
    }
    else {
        EV << "UeWarningAlertApp::handleAckStartMEWarningAlertApp - MEC application cannot be instantiated! Reason: " << pkt->getReason() << endl;
    }
}

void UeWarningAlertApp::sendMessageToMecApp() {

    // send start monitoring message to the MEC application

    inet::Packet *pkt = new inet::Packet("WarningAlertPacketStart");
    auto alert = inet::makeShared<WarningStartPacket>();
    alert->setType(START_WARNING);
    alert->setCenterPositionX(par("positionX").doubleValue());
    alert->setCenterPositionY(par("positionY").doubleValue());
    alert->setCenterPositionZ(par("positionZ").doubleValue());
    alert->setRadius(par("radius").doubleValue());
    alert->setChunkLength(inet::B(20));
    alert->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    pkt->insertAtBack(alert);

    if (log) {
        ofstream myfile;
        myfile.open("example.txt", ios::app);
        if (myfile.is_open()) {
            myfile << "[" << NOW << "] UeWarningAlertApp - UE sent start subscription message to the MEC application \n";
            myfile.close();
        }
    }

    socket.sendTo(pkt, mecAppAddress_, mecAppPort_);
    EV << "UeWarningAlertApp::sendMessageToMecApp() - Start message sent to the MEC app" << endl;
}

void UeWarningAlertApp::handleInfoMEWarningAlertApp(cMessage *msg)
{
    inet::Packet *packet = check_and_cast<inet::Packet *>(msg);
    auto pkt = packet->peekAtFront<WarningAlertPacket>();

    EV << "UeWarningAlertApp::handleInfoMEWarningAlertApp - Received " << pkt->getType() << " type WarningAlertPacket" << endl;

    //updating runtime color of the car icon background
    if (pkt->getDanger()) {
        if (log) {
            ofstream myfile;
            myfile.open("example.txt", ios::app);
            if (myfile.is_open()) {
                myfile << "[" << NOW << "] UeWarningAlertApp - UE received danger alert TRUE from MEC application \n";
                myfile.close();
            }
        }

        EV << "UeWarningAlertApp::handleInfoMEWarningAlertApp - Warning Alert Detected: DANGER!" << endl;
        ue->getDisplayString().setTagArg("i", 1, "red");
    }
    else {
        if (log) {
            ofstream myfile;
            myfile.open("example.txt", ios::app);
            if (myfile.is_open()) {
                myfile << "[" << NOW << "] UeWarningAlertApp - UE received danger alert FALSE from MEC application \n";
                myfile.close();
            }
        }

        EV << "UeWarningAlertApp::handleInfoMEWarningAlertApp - Warning Alert Detected: NO DANGER!" << endl;
        ue->getDisplayString().setTagArg("i", 1, "green");
    }
}

void UeWarningAlertApp::handleAckStopMEWarningAlertApp(cMessage *msg)
{

    inet::Packet *packet = check_and_cast<inet::Packet *>(msg);
    auto pkt = packet->peekAtFront<DeviceAppStopAckPacket>();

    EV << "UeWarningAlertApp::handleAckStopMEWarningAlertApp - Received " << pkt->getType() << " type WarningAlertPacket with result: " << pkt->getResult() << endl;
    if (pkt->getResult() == false)
        EV << "Reason: " << pkt->getReason() << endl;
    //updating runtime color of the car icon background
    ue->getDisplayString().setTagArg("i", 1, "white");

    cancelEvent(selfStop_);
}

} //namespace

