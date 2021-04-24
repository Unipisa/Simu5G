//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "apps/mec/EchoApp/UEechoApp.h"
#include "inet/common/TimeTag_m.h"

Define_Module(UEechoApp);

UEechoApp::UEechoApp(){
    selfStart_ = NULL;
    selfSender_ = NULL;
    selfStop_ = NULL;
}

UEechoApp::~UEechoApp(){
    cancelAndDelete(selfStart_);
    cancelAndDelete(selfSender_);
    cancelAndDelete(selfStop_);
}

void UEechoApp::initialize(int stage)
{
    EV << "UEechoApp::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    //retrieve parameters
    size_ = par("packetSize");
    period_ = par("period");
    localPort_ = par("localPort");
    destPort_ = par("destPort");
    sourceSimbolicAddress = (char*)getParentModule()->getFullName();
    destSimbolicAddress = (char*)par("destAddress").stringValue();
    destAddress_ = inet::L3AddressResolver().resolve(destSimbolicAddress);

    requiredRam = par("requiredRam");
    requiredDisk = par("requiredDisk");
    requiredCpu = par("requiredCpu").doubleValue();

    //binding socket
    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort_);

    int tos = par("tos");
    if (tos != -1)
        socket.setTos(tos);

    //retrieving car cModule
    ue = this->getParentModule();

    //retrieving mobility module
    cModule *temp = getParentModule()->getSubmodule("mobility");
    if(temp != NULL){
        mobility = check_and_cast<inet::IMobility*>(temp);
    }
    else {
        EV << "UEechoApp::initialize - \tWARNING: Mobility module NOT FOUND!" << endl;
        throw cRuntimeError("UEechoApp::initialize - \tWARNING: Mobility module NOT FOUND!");
    }

    //initializing the auto-scheduling messages
    selfSender_ = new cMessage("selfSender");
    selfStart_ = new cMessage("selfStart");
    selfStop_ = new cMessage("selfStop");

    //starting UEechoApp
    simtime_t startTime = par("startTime");
    EV << "UEechoApp::initialize - starting sendStartMEWarningAlertApp() in " << startTime << " seconds " << endl;
    scheduleAt(simTime() + startTime, selfStart_);

    //testing
    EV << "UEechoApp::initialize - sourceAddress: " << sourceSimbolicAddress << " [" << inet::L3AddressResolver().resolve(sourceSimbolicAddress).str()  <<"]"<< endl;
    EV << "UEechoApp::initialize - destAddress: " << destSimbolicAddress << " [" << destAddress_.str()  <<"]"<< endl;
    EV << "UEechoApp::initialize - binding to port: local:" << localPort_ << " , dest:" << destPort_ << endl;
}

void UEechoApp::handleMessage(cMessage *msg)
{
    EV << "UEechoApp::handleMessage" << endl;
    // Sender Side
    if (msg->isSelfMessage())
    {
        if (!strcmp(msg->getName(), "selfSender"))      sendInfoUEWarningAlertApp();

        else if(!strcmp(msg->getName(), "selfStart"))   sendStartMEWarningAlertApp();

        else if(!strcmp(msg->getName(), "selfStop"))    sendStopMEWarningAlertApp();

        else    throw cRuntimeError("UEechoApp::handleMessage - \tWARNING: Unrecognized self message");
    }
    // Receiver Side
    else{
        inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
        auto mePkt = packet->peekAtFront<MEAppPacket>();

        if (mePkt == 0)
            throw cRuntimeError("UEechoApp::handleMessage - \tFATAL! Error when casting to MEAppPacket");

        if( !strcmp(mePkt->getType(), ACK_START_MEAPP) )    handleAckStartMEWarningAlertApp(msg);

        else if(!strcmp(mePkt->getType(), ACK_STOP_MEAPP))  handleAckStopMEWarningAlertApp(msg);

        else{
            inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
            auto pkt = packet->peekAtFront<WarningAlertPacket>();
            EV << "receive position: " << pkt->getPositionY() << endl;
            //rescheduling
            scheduleAt(simTime() + period_, selfSender_);
        }

        delete msg;
    }
}

void UEechoApp::finish()
{
    // ensuring there is no selfStop_ scheduled!
    if(selfStop_->isScheduled())
        cancelEvent(selfStop_);
}
/*
 * -----------------------------------------------Sender Side------------------------------------------
 */
void UEechoApp::sendStartMEWarningAlertApp()
{
    EV << "UEechoApp::sendStartMEWarningAlertApp - Sending " << START_MEAPP << " type WarningAlertPacket\n";

    inet::Packet* packet = new inet::Packet("WarningAlertPacketStart");
    auto alert = inet::makeShared<WarningAlertPacket>();

    //instantiation requirements and info
    alert->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    alert->setType(START_MEAPP);
    alert->setMEModuleType("lte.apps.mec.EchoApp.MECUdpEchoApp");
    alert->setMEModuleName("MECUdpEchoApp");
    alert->setRequiredService("NULL");
    alert->setRequiredRam(requiredRam);
    alert->setRequiredDisk(requiredDisk);
    alert->setRequiredCpu(requiredCpu);

    //connection info
    alert->setSourceAddress(sourceSimbolicAddress);
    alert->setSourcePort(localPort_);
    alert->setDestinationAddress(destSimbolicAddress);

    //identification info
    alert->setUeAppID(getId());

    //other info
    alert->setUeOmnetID(ue->getId());
    alert->setChunkLength(inet::B(size_));

    packet->insertAtBack(alert);
    socket.sendTo(packet, destAddress_, destPort_);

    //rescheduling
    scheduleAt(simTime() + period_, selfStart_);
}
void UEechoApp::sendStopMEWarningAlertApp()
{
    EV << "UEechoApp::sendStopMEWarningAlertApp - Sending " << STOP_MEAPP <<" type WarningAlertPacket\n";

    inet::Packet* packet = new inet::Packet("WarningAlertPacketStop");
    auto alert = inet::makeShared<WarningAlertPacket>();

    //termination requirements and info
    alert->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    alert->setType(STOP_MEAPP);
    alert->setMEModuleType("lte.apps.mec.EchoApp.MECUdpEchoApp");
    alert->setMEModuleName("MECUdpEchoApp");
    alert->setRequiredService("NULL");
    alert->setRequiredRam(requiredRam);
    alert->setRequiredDisk(requiredDisk);
    alert->setRequiredCpu(requiredCpu);

    //connection info
    alert->setSourceAddress(sourceSimbolicAddress);
    alert->setSourcePort(localPort_);
    alert->setDestinationAddress(destSimbolicAddress);

    //identification info
    alert->setUeAppID(getId());

    //other info
    alert->setUeOmnetID(ue->getId());
    alert->setChunkLength(inet::B(size_));

    packet->insertAtBack(alert);
    socket.sendTo(packet, destAddress_, destPort_);

    // stopping the info updating
    if(selfSender_->isScheduled())
        cancelEvent(selfSender_);

    //rescheduling
    if(selfStop_->isScheduled())
        cancelEvent(selfStop_);
    scheduleAt(simTime() + period_, selfStop_);
}
void UEechoApp::sendInfoUEWarningAlertApp()
{
    EV << "UEWarningAlertApp::sendInfoUEInceAlertApp - Sending " << INFO_UEAPP <<" type EchoApp\n";

    position = mobility->getCurrentPosition();
    EV << "send position: " << position << endl;
    inet::Packet* packet = new inet::Packet("WarningAlertPacketInfo");
    auto alert = inet::makeShared<WarningAlertPacket>();

    //forwarding requirements and info
    alert->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    alert->setType(INFO_UEAPP);

    //INFO_UEAPP info
    alert->setPositionX(position.x);
    alert->setPositionY(position.y);
    alert->setPositionZ(position.z);

    alert->setChunkLength(inet::B(size_*2));

    packet->insertAtBack(alert);
    socket.sendTo(packet, meAppAddress, meAppPort);



}
/*
 * ---------------------------------------------Receiver Side------------------------------------------
 */
void UEechoApp::handleAckStartMEWarningAlertApp(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto pkt = packet->peekAtFront<MEAppPacket>();

    meAppPort = pkt->getDestinationPort();
    meAppAddress = inet::L3AddressResolver().resolve(pkt->getSourceAddress());


    EV << "UEechoApp::handleAckStartMEWarningAlertApp - Received " << pkt->getType() << " type WarningAlertPacket from: "<< pkt->getSourceAddress() << endl;

    cancelEvent(selfStart_);

    //scheduling sendInfoUEInceAlertApp()
    if(!selfSender_->isScheduled()){

       scheduleAt( simTime() + period_, selfSender_);
       EV << "UEWarningAlertApp::handleAckStartMEWarningAlertApp - Starting traffic in " << period_ << " seconds " << endl;
    }


    //scheduling sendStopMEWarningAlertApp()
    if(!selfStop_->isScheduled()){
        simtime_t  stopTime = par("stopTime");
        scheduleAt(simTime() + stopTime, selfStop_);
        EV << "UEechoApp::handleAckStartMEWarningAlertApp - Starting sendStopMEWarningAlertApp() in " << stopTime << " seconds " << endl;
    }
}
void UEechoApp::handleInfoMEWarningAlertApp(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto pkt = packet->peekAtFront<WarningAlertPacket>();

    EV << "UEechoApp::handleInfoMEWarningrAlertApp - Received " << pkt->getType() << " type WarningAlertPacket from: "<< pkt->getSourceAddress() << endl;

    //updating runtime color of the car icon background
    if(pkt->getDanger()){

        EV << "UEechoApp::handleInfoMEWarningrAlertApp - Warning Alert Detected: DANGER!" << endl;
        ue->getDisplayString().setTagArg("i",1, "red");
    }
    else{

        EV << "UEechoApp::handleInfoMEWarningrAlertApp - Warning Alert Detected: NO DANGER!" << endl;
        ue->getDisplayString().setTagArg("i",1, "green");
    }
}
void UEechoApp::handleAckStopMEWarningAlertApp(cMessage* msg)
{

    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto pkt = packet->peekAtFront<MEAppPacket>();

    EV << "UEechoApp::handleAckStopMEWarningAlertApp - Received " << pkt->getType() << " type WarningAlertPacket from: "<< pkt->getSourceAddress() << endl;

    //updating runtime color of the car icon background
    ue->getDisplayString().setTagArg("i",1, "white");

    cancelEvent(selfStop_);
}
