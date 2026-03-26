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

#include <cmath>
#include <inet/common/TimeTag_m.h>
#include "CbrSender.h"

namespace simu5g {

#define round(x)    floor((x) + 0.5)

Define_Module(CbrSender);

using namespace inet;
using namespace std;

simsignal_t CbrSender::cbrGeneratedThroughputSignal_ = registerSignal("cbrGeneratedThroughput");
simsignal_t CbrSender::cbrGeneratedBytesSignal_ = registerSignal("cbrGeneratedBytes");
simsignal_t CbrSender::cbrSentPktSignal_ = registerSignal("cbrSentPkt");


CbrSender::~CbrSender()
{
    cancelAndDelete(sendTimer_);
}

void CbrSender::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        sendTimer_ = new cMessage("selfSource");
        packetSize_ = par("packetSize");
        samplingTime = par("samplingTime");
        localPort_ = par("localPort");
        destPort_ = par("destPort");
    }
    else if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        // calculating traffic starting time
        startTime_ = par("startTime");
        finishTime_ = par("finishTime");

        EV << " finish time " << finishTime_ << endl;
        numFrames_ = (finishTime_ - startTime_) / samplingTime;

        initTrafficTimer_ = new cMessage("initTraffic");
        initTraffic();
    }
}

void CbrSender::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (msg == sendTimer_) {
            EV << "CbrSender::handleMessage - now[" << simTime() << "] <= finish[" << finishTime_ << "]" << endl;
            if (simTime() <= finishTime_ || finishTime_ == 0)
                sendCbrPacket();
        }
        else
            initTraffic();
    }
}

void CbrSender::initTraffic()
{
    std::string destAddress = par("destAddress").stringValue();
    cModule *destModule = findModuleByPath(par("destAddress").stringValue());
    if (destModule == nullptr) {
        // this might happen when users are created dynamically
        EV << simTime() << "CbrSender::initTraffic - destination " << destAddress << " not found" << endl;

        simtime_t offset = 0.01; // TODO check value
        scheduleAt(simTime() + offset, initTrafficTimer_);
        EV << simTime() << "CbrSender::initTraffic - the node will retry to initialize traffic in " << offset << " seconds " << endl;
    }
    else {
        delete initTrafficTimer_;

        destAddress_ = inet::L3AddressResolver().resolve(par("destAddress").stringValue());
        socket.setOutputGate(gate("socketOut"));
        socket.bind(localPort_);

        int tos = par("tos");
        if (tos != -1)
            socket.setTos(tos);

        EV << simTime() << "CbrSender::initialize - binding to port: local:" << localPort_ << " , dest: " << destAddress_.str() << ":" << destPort_ << endl;

        // calculating traffic starting time
        simtime_t startTime = par("startTime");

        scheduleAt(simTime() + startTime, sendTimer_);
        EV << "\t starting traffic in " << startTime << " seconds " << endl;
    }
}

void CbrSender::sendCbrPacket()
{
    Packet *packet = new Packet("CBR");
    auto cbr = makeShared<CbrPacket>();
    cbr->setNumFrames(numFrames_);
    cbr->setFrameId(frameId_++);
    cbr->setPayloadTimestamp(simTime());
    cbr->setPayloadSize(packetSize_);
    cbr->setChunkLength(B(packetSize_));
    cbr->addTag<CreationTimeTag>()->setCreationTime(simTime());
    packet->insertAtBack(cbr);

    emit(cbrGeneratedBytesSignal_, packetSize_);

    if (simTime() > getSimulation()->getWarmupPeriod()) {
        txBytes_ += packetSize_;
    }
    socket.sendTo(packet, destAddress_, destPort_);

    scheduleAt(simTime() + samplingTime, sendTimer_);
}

void CbrSender::finish()
{
    simtime_t elapsedTime = simTime() - getSimulation()->getWarmupPeriod();
    emit(cbrGeneratedThroughputSignal_, txBytes_ / elapsedTime.dbl());
}

} //namespace

