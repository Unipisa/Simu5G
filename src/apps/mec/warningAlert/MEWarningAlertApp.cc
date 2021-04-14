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

#include "apps/mec/warningAlert/MEWarningAlertApp.h"
#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/Packet_m.h"

Define_Module(MEWarningAlertApp);

using namespace inet;

void MEWarningAlertApp::initialize(int stage)
{
    EV << "MEWarningAlertApp::initialize - stage " << stage << endl;

    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    //retrieving parameters
    size_ = par("packetSize");
    ueSimbolicAddress = (char*)par("ueSimbolicAddress").stringValue();
    meHostSimbolicAddress = (char*)par("meHostSimbolicAddress").stringValue();
    destAddress_ = inet::L3AddressResolver().resolve(ueSimbolicAddress);

    //testing
    EV << "MEWarningAlertApp::initialize - MEWarningAlertApp Symbolic Address: " << meHostSimbolicAddress << endl;
    EV << "MEWarningAlertApp::initialize - UEWarningAlertApp Symbolic Address: " << ueSimbolicAddress <<  " [" << destAddress_.str() << "]" << endl;

}

void MEWarningAlertApp::handleMessage(omnetpp::cMessage *msg)
{
    EV << "MEWarningAlertApp::handleMessage - \n";

    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto pkt = packet->peekAtFront<MEAppPacket>();

    if (pkt == 0)
        throw cRuntimeError("MEWarningAlertApp::handleMessage - \tFATAL! Error when casting to WarningAlertPacket");

    if(!strcmp(pkt->getType(), INFO_UEAPP))         handleInfoUEWarningAlertApp(msg);

    else if(!strcmp(pkt->getType(), INFO_MEAPP))    handleInfoMEWarningAlertApp(msg);
}

void MEWarningAlertApp::finish(){

    EV << "MEWarningAlertApp::finish - Sending " << STOP_MEAPP << " to the MEIceAlertService" << endl;

    if(gate("mePlatformOut")->isConnected()){

        WarningAlertPacket* packet = new WarningAlertPacket();
        packet->setType(STOP_MEAPP);

        send((cMessage*)packet, "mePlatformOut");
    }
}
void MEWarningAlertApp::handleInfoUEWarningAlertApp(cMessage* msg){

    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto pkt = packet->peekAtFront<WarningAlertPacket>();

    simtime_t delay = simTime() - pkt->getTag<inet::CreationTimeTag>()->getCreationTime();

    EV << "MEWarningAlertApp::handleInfoUEWarningAlertApp - Received: " << pkt->getType() << " type WarningAlertPacket from " << pkt->getSourceAddress() << ": delay: "<< delay << endl;

    EV << "MEWarningAlertApp::handleInfoUEWarningAlertApp - Upstream " << pkt->getType() << " type WarningAlertPacket to MEIceAlertService \n";
    send(packet, "mePlatformOut");

    //testing
    EV << "MEWarningAlertApp::handleInfoUEWarningAlertApp position: [" << pkt->getPositionX() << " ; " << pkt->getPositionY() << " ; " << pkt->getPositionZ() << "]" << endl;
}

void MEWarningAlertApp::handleInfoMEWarningAlertApp(cMessage* msg){

    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto pkt = packet->removeAtFront<WarningAlertPacket>();

    EV << "MEWarningAlertApp::handleInfoMEWarningAlertApp - Finalize creation of " << pkt->getType() << " type WarningAlertPacket" << endl;

    pkt->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    pkt->setChunkLength(B(size_));
    pkt->setSourceAddress(meHostSimbolicAddress);
    pkt->setDestinationAddress(ueSimbolicAddress);
    pkt->setMEModuleType("simu5g.apps.mec.warningAlert.MEWarningAlertApp");
    pkt->setMEModuleName("MEWarningAlertApp");
    packet->insertAtFront(pkt);

    EV << "MEWarningAlertApp::handleInfoMEWarningAlertApp - Downstream " << pkt->getType() << " type WarningAlertPacket to VirtualisationManager \n";
    send(packet, "virtualisationInfrastructureOut");
}


