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

#include "CbrReceiver.h"

namespace simu5g {

Define_Module(CbrReceiver);
using namespace inet;

simsignal_t CbrReceiver::cbrFrameLossSignal_ = registerSignal("cbrFrameLoss");
simsignal_t CbrReceiver::cbrFrameDelaySignal_ = registerSignal("cbrFrameDelay");
simsignal_t CbrReceiver::cbrJitterSignal_ = registerSignal("cbrJitter");
simsignal_t CbrReceiver::cbrReceivedThroughputSignal_ = registerSignal("cbrReceivedThroughput");
simsignal_t CbrReceiver::cbrReceivedBytesSignal_ = registerSignal("cbrReceivedBytes");
simsignal_t CbrReceiver::cbrRcvdPktSignal_ = registerSignal("cbrRcvdPkt");

void CbrReceiver::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        int port = par("localPort");
        EV << "CbrReceiver::initialize - binding to port: local:" << port << endl;
        if (port != -1) {
            socket.setOutputGate(gate("socketOut"));
            socket.bind(port);
        }
    }
}

void CbrReceiver::handleMessage(cMessage *msg)
{
    ASSERT(!msg->isSelfMessage());

    Packet *pPacket = check_and_cast<Packet *>(msg);
    auto cbrHeader = pPacket->popAtFront<CbrPacket>();

    numReceived_++;
    totFrames_ = cbrHeader->getNumFrames(); // XXX this value can be written just once
    int pktSize = (int)cbrHeader->getPayloadSize();

    // just to make sure we do not update recvBytes AND we avoid dividing by 0
    if (simTime() > getSimulation()->getWarmupPeriod()) {
        recvBytes_ += pktSize;
        emit(cbrReceivedBytesSignal_, pktSize);
    }

    simtime_t delay = simTime() - cbrHeader->getPayloadTimestamp();
    emit(cbrFrameDelaySignal_, delay);

    EV << "CbrReceiver::handleMessage - Packet received: FRAME[" << cbrHeader->getFrameId() << "/" << cbrHeader->getNumFrames() << "] with delay[" << delay << "]" << endl;

    emit(cbrRcvdPktSignal_, (long)cbrHeader->getFrameId());

    delete msg;
}

void CbrReceiver::finish()
{
    double lossRate = 0;
    if (totFrames_ > 0)
        lossRate = 1.0 - (numReceived_ / (totFrames_ * 1.0));

    emit(cbrFrameLossSignal_, lossRate);

    simtime_t elapsedTime = simTime() - getSimulation()->getWarmupPeriod();
    emit(cbrReceivedThroughputSignal_, recvBytes_ / elapsedTime.dbl());
}

} //namespace

