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

#include "apps/mecRequestResponseApp/MecResponseApp.h"

namespace simu5g {

simsignal_t MecResponseApp::recvRequestSnoSignal_ = registerSignal("recvRequestSno");

Define_Module(MecResponseApp);

void MecResponseApp::initialize(int stage)
{
    EV << "MecResponseApp::initialize - stage " << stage << endl;

    cSimpleModule::initialize(stage);
    if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        coreNetworkDelay_ = par("coreNetworkDelay");
        int port = par("localPort");
        EV << "CbrReceiver::initialize - binding to port: local:" << port << endl;
        if (port != -1) {
            socket.setOutputGate(gate("socketOut"));
            socket.bind(port);
            int tos = par("tos");
            if (tos != -1)
                socket.setTos(tos);
        }
    }
}

void MecResponseApp::handleMessage(cMessage *msg)
{
    EV << "MecResponseApp::handleMessage - \n";
    if (msg->isSelfMessage())
        sendResponse(msg);
    else
        handleRequest(msg);
}

void MecResponseApp::handleRequest(cMessage *msg)
{
    inet::Packet *packet = check_and_cast<inet::Packet *>(msg);
    packet->removeControlInfo();

    auto reqPkt = packet->peekAtFront<MecRequestResponsePacket>();

    simtime_t delay = simTime() - reqPkt->getReqTimestamp();
    int bits = (reqPkt->getChunkLength().get()) * 8;
    EV << simTime() << "MecResponseApp::handleRequest - Received packet with number " << reqPkt->getSno() << ": delay: " << delay << endl;

    unsigned int ueAppId = reqPkt->getAppId();
    MacNodeId ueBsId = reqPkt->getBsId();

    double responseTime = 0.0;
    if (ueAppId != num(ueBsId)) {  //TODO type mismatch
        // add delay
        responseTime += uniform(0.015, 0.035);
    }

    // extract random response time according to some distribution (paper INTEL)
    int cyclesPerBit = intuniform(100, 300);
    long long int cpuFreq = 9000000000;  // 9 Gcycles/sec
    responseTime += double(bits * cyclesPerBit) / cpuFreq;

    EV << simTime() << "MecResponseApp::handleRequest - sending response in " << responseTime << " seconds " << endl;

    scheduleAt(simTime() + responseTime, packet);

    emit(recvRequestSnoSignal_, (long)reqPkt->getSno());
}

void MecResponseApp::sendResponse(cMessage *msg)
{
    inet::Packet *packet = check_and_cast<inet::Packet *>(msg);
    auto pkt = packet->popAtFront<MecRequestResponsePacket>();
    auto respPkt = inet::makeShared<MecRequestResponsePacket>();

    const char *reqSourceAddress = pkt->getSrcAddress();
    int reqSourcePort = pkt->getSrcPort();

    EV << simTime() << "MecResponseApp::sendResponse - Sending response for packet with number " << pkt->getSno() << " to " << reqSourceAddress << "(port " << reqSourcePort << ")" << endl;

    respPkt->setRespTimestamp(simTime().dbl());
    respPkt->setSrcAddress(pkt->getDestAddress());
    respPkt->setSrcPort(pkt->getDestPort());
    respPkt->setDestAddress(reqSourceAddress);
    respPkt->setDestPort(reqSourcePort);
    packet->insertAtBack(respPkt);
    socket.sendTo(packet, inet::L3AddressResolver().resolve(reqSourceAddress), reqSourcePort);
}

} //namespace

