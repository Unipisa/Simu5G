//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/apps/burst/BurstReceiver.h"

namespace simu5g {

Define_Module(BurstReceiver);

simsignal_t BurstReceiver::burstRcvdPktSignal_ = registerSignal("burstRcvdPkt");
simsignal_t BurstReceiver::burstPktDelaySignal_ = registerSignal("burstPktDelay");

void BurstReceiver::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        int port = par("localPort");
        EV << "BurstReceiver::initialize - binding to port: local:" << port << endl;
        if (port != -1) {
            socket.setOutputGate(gate("socketOut"));
            socket.bind(port);
        }
    }
}

void BurstReceiver::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
        return;

    Packet *pPacket = check_and_cast<Packet *>(msg);
    auto burstHeader = pPacket->popAtFront<BurstPacket>();

    numReceived_++;

    simtime_t delay = simTime() - burstHeader->getPayloadTimestamp();
    EV << "BurstReceiver::handleMessage - Packet received: FRAME[" << burstHeader->getMsgId() << "] with delay[" << delay << "]" << endl;

    emit(burstPktDelaySignal_, delay);
    emit(burstRcvdPktSignal_, (long)burstHeader->getMsgId());

    delete msg;
}

} //namespace

