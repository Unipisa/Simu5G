//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "apps/mec/warningAlert_rest/UEWarningAlertApp_rest.h"
#include "inet/common/TimeTag_m.h"

Define_Module(UEWarningAlertApp_rest);

UEWarningAlertApp_rest::UEWarningAlertApp_rest(){
    selfStart_ = NULL;
    selfSender_ = NULL;
    selfStop_ = NULL;
}

UEWarningAlertApp_rest::~UEWarningAlertApp_rest(){
    cancelAndDelete(selfStart_);
    cancelAndDelete(selfSender_);
    cancelAndDelete(selfStop_);
}

void UEWarningAlertApp_rest::initialize(int stage)
{
    EV << "UEWarningAlertApp_rest::initialize - stage " << stage << endl;
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
        EV << "UEWarningAlertApp_rest::initialize - \tWARNING: Mobility module NOT FOUND!" << endl;
        throw cRuntimeError("UEWarningAlertApp_rest::initialize - \tWARNING: Mobility module NOT FOUND!");
    }

    //initializing the auto-scheduling messages
    selfSender_ = new cMessage("selfSender");
    selfStart_ = new cMessage("selfStart");
    selfStop_ = new cMessage("selfStop");

    //starting UEWarningAlertApp_rest
    simtime_t startTime = par("startTime");
    EV << "UEWarningAlertApp_rest::initialize - starting sendStartMEWarningAlertApp() in " << startTime << " seconds " << endl;
    scheduleAt(simTime() + startTime, selfStart_);

    //testing
    EV << "UEWarningAlertApp_rest::initialize - sourceAddress: " << sourceSimbolicAddress << " [" << inet::L3AddressResolver().resolve(sourceSimbolicAddress).str()  <<"]"<< endl;
    EV << "UEWarningAlertApp_rest::initialize - destAddress: " << destSimbolicAddress << " [" << destAddress_.str()  <<"]"<< endl;
    EV << "UEWarningAlertApp_rest::initialize - binding to port: local:" << localPort_ << " , dest:" << destPort_ << endl;
}

void UEWarningAlertApp_rest::handleMessage(cMessage *msg)
{
    EV << "UEWarningAlertApp_rest::handleMessage" << endl;
    // Sender Side
    if (msg->isSelfMessage())
    {
        if(!strcmp(msg->getName(), "selfStart"))   sendStartMEWarningAlertApp();

        else if(!strcmp(msg->getName(), "selfStop"))    sendStopMEWarningAlertApp();

        else    throw cRuntimeError("UEWarningAlertApp_rest::handleMessage - \tWARNING: Unrecognized self message");
    }
    // Receiver Side
    else{
        inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
        auto mePkt = packet->peekAtFront<MEAppPacket>();

        if (mePkt == 0)
            throw cRuntimeError("UEWarningAlertApp_rest::handleMessage - \tFATAL! Error when casting to MEAppPacket");

        if( !strcmp(mePkt->getType(), ACK_START_MEAPP) )    handleAckStartMEWarningAlertApp(msg);

        else if(!strcmp(mePkt->getType(), ACK_STOP_MEAPP))  handleAckStopMEWarningAlertApp(msg);

        else if(!strcmp(mePkt->getType(), INFO_MEAPP))      handleInfoMEWarningAlertApp(msg);

        delete msg;
    }
}

void UEWarningAlertApp_rest::finish()
{
    // ensuring there is no selfStop_ scheduled!
    if(selfStop_->isScheduled())
        cancelEvent(selfStop_);
}
/*
 * -----------------------------------------------Sender Side------------------------------------------
 */
void UEWarningAlertApp_rest::sendStartMEWarningAlertApp()
{
    EV << "UEWarningAlertApp_rest::sendStartMEWarningAlertApp - Sending " << START_MEAPP << " type WarningAlertPacket\n";

    inet::Packet* packet = new inet::Packet("WarningAlertPacketStart");
    auto alert = inet::makeShared<WarningAlertPacket>();

    //instantiation requirements and info
    alert->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    alert->setType(START_MEAPP);
    alert->setMEModuleType("lte.apps.mec.warningAlert_rest.MEWarningAlertApp_rest");
    alert->setMEModuleName("MEWarningAlertApp_rest");
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
void UEWarningAlertApp_rest::sendStopMEWarningAlertApp()
{
    EV << "UEWarningAlertApp_rest::sendStopMEWarningAlertApp - Sending " << STOP_MEAPP <<" type WarningAlertPacket\n";

    inet::Packet* packet = new inet::Packet("WarningAlertPacketStop");
    auto alert = inet::makeShared<WarningAlertPacket>();

    //termination requirements and info
    alert->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    alert->setType(STOP_MEAPP);
    alert->setMEModuleType("lte.apps.mec.warningAlert_rest.MEWarningrAlertApp_rest");
    alert->setMEModuleName("MEWarningAlertApp_rest");
    alert->setRequiredService("MEWarningAlertService");
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

/*
 * ---------------------------------------------Receiver Side------------------------------------------
 */
void UEWarningAlertApp_rest::handleAckStartMEWarningAlertApp(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto pkt = packet->peekAtFront<MEAppPacket>();

    EV << "UEWarningAlertApp_rest::handleAckStartMEWarningAlertApp - Received " << pkt->getType() << " type WarningAlertPacket from: "<< pkt->getSourceAddress() << endl;

    cancelEvent(selfStart_);

    //scheduling sendStopMEWarningAlertApp()
    if(!selfStop_->isScheduled()){
        simtime_t  stopTime = par("stopTime");
        scheduleAt(simTime() + stopTime, selfStop_);
        EV << "UEWarningAlertApp_rest::handleAckStartMEWarningAlertApp - Starting sendStopMEWarningAlertApp() in " << stopTime << " seconds " << endl;
    }

    //send start message alla mec app!

    //REMOVE THIS. just debug

    inet::Packet* pp = new inet::Packet("WarningAlertPacketStop");
    auto alert = inet::makeShared<WarningAlertPacket>();

    //termination requirements and info
    alert->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    pp->insertAtBack(alert);

    inet::L3Address destApp = inet::L3Address(pkt->getDestinationMecAppAddress());
    socket.sendTo(pp, destApp , pkt->getDestinationMecAppPort());


}
void UEWarningAlertApp_rest::handleInfoMEWarningAlertApp(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto pkt = packet->peekAtFront<WarningAlertPacket>();

    EV << "UEWarningAlertApp_rest::handleInfoMEWarningrAlertApp - Received " << pkt->getType() << " type WarningAlertPacket from: "<< pkt->getSourceAddress() << endl;

    //updating runtime color of the car icon background
    if(pkt->getDanger()){

        EV << "UEWarningAlertApp_rest::handleInfoMEWarningrAlertApp - Warning Alert Detected: DANGER!" << endl;
        ue->getDisplayString().setTagArg("i",1, "red");
    }
    else{

        EV << "UEWarningAlertApp_rest::handleInfoMEWarningrAlertApp - Warning Alert Detected: NO DANGER!" << endl;
        ue->getDisplayString().setTagArg("i",1, "green");
    }
}
void UEWarningAlertApp_rest::handleAckStopMEWarningAlertApp(cMessage* msg)
{

    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto pkt = packet->peekAtFront<MEAppPacket>();

    EV << "UEWarningAlertApp_rest::handleAckStopMEWarningAlertApp - Received " << pkt->getType() << " type WarningAlertPacket from: "<< pkt->getSourceAddress() << endl;

    //updating runtime color of the car icon background
    ue->getDisplayString().setTagArg("i",1, "white");

    cancelEvent(selfStop_);
}
