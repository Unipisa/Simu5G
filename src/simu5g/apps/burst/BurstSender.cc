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
#include "simu5g/apps/burst/BurstSender.h"

namespace simu5g {

#define round(x)    floor((x) + 0.5)

Define_Module(BurstSender);

simsignal_t BurstSender::burstSentPktSignal_ = registerSignal("burstSentPkt");

BurstSender::~BurstSender()
{
    cancelAndDelete(burstTimer_);
    cancelAndDelete(packetTimer_);
}

void BurstSender::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        burstTimer_ = new cMessage("selfBurst");
        packetTimer_ = new cMessage("selfPacket");
        packetSize_ = par("packetSize");
        burstSize_ = par("burstSize");
        interBurstTime_ = par("interBurstTime");
        intraBurstTime_ = par("intraBurstTime");
        localPort_ = par("localPort");
        destPort_ = par("destPort");
    }
    else if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        initTraffic_ = new cMessage("initTraffic");
        initTraffic();
    }
}

void BurstSender::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (msg == burstTimer_)
            sendBurst();
        else if (msg == packetTimer_)
            sendPacket();
        else
            initTraffic();
    }
}

void BurstSender::initTraffic()
{
    std::string destAddress = par("destAddress").stringValue();
    cModule *destModule = findModuleByPath(par("destAddress").stringValue());
    if (destModule == nullptr) {
        // this might happen when users are created dynamically
        EV << simTime() << "BurstSender::initTraffic - destination " << destAddress << " not found" << endl;

        simtime_t offset = 0.01; // TODO check value
        scheduleAt(simTime() + offset, initTraffic_);
        EV << simTime() << "BurstSender::initTraffic - the node will retry to initialize traffic in " << offset << " seconds " << endl;
    }
    else {
        delete initTraffic_;

        destAddress_ = inet::L3AddressResolver().resolve(par("destAddress").stringValue());
        socket.setOutputGate(gate("socketOut"));
        socket.bind(localPort_);

        int tos = par("tos");
        if (tos != -1)
            socket.setTos(tos);

        EV << simTime() << "BurstSender::initialize - binding to port: local:" << localPort_ << " , dest: " << destAddress_.str() << ":" << destPort_ << endl;

        // calculating traffic starting time
        simtime_t startTime = par("startTime");

        scheduleAt(simTime() + startTime, burstTimer_);
        EV << "\t starting traffic in " << startTime << " seconds " << endl;
    }
}

void BurstSender::sendBurst()
{
    burstId_++;
    frameId_ = 0;

    EV << simTime() << " BurstSender::sendBurst - Start sending burst[" << burstId_ << "]" << endl;

    // send first packet
    sendPacket();

    scheduleAt(simTime() + interBurstTime_, burstTimer_);
}

void BurstSender::sendPacket()
{
    EV << "BurstSender::sendPacket - Sending frame[" << frameId_ << "] of burst [" << burstId_ << "], next packet at " << simTime() + intraBurstTime_ << endl;

    //unsigned int msgId = (idBurst_ << 16) | idFrame_;
    unsigned int msgId = (burstId_ * burstSize_) + frameId_;

    Packet *packet = new inet::Packet("Burst");
    auto burst = makeShared<BurstPacket>();

    burst->setMsgId(msgId);
    burst->setPayloadTimestamp(simTime());
    burst->setPayloadSize(packetSize_);
    burst->setChunkLength(B(packetSize_));
    burst->addTag<CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(burst);

    socket.sendTo(packet, destAddress_, destPort_);

    emit(burstSentPktSignal_, (long)msgId);

    frameId_++;
    if (frameId_ < burstSize_)
        scheduleAt(simTime() + intraBurstTime_, packetTimer_);
}

} //namespace

