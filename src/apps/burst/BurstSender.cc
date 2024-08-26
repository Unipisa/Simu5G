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

#include <cmath>
#include <inet/common/TimeTag_m.h>
#include "apps/burst/BurstSender.h"

namespace simu5g {

#define round(x)    floor((x) + 0.5)

Define_Module(BurstSender);

simsignal_t BurstSender::burstSentPktSignal_ = registerSignal("burstSentPkt");

BurstSender::~BurstSender()
{
    cancelAndDelete(selfBurst_);
    cancelAndDelete(selfPacket_);
}

void BurstSender::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    EV << "BurstSender initialize: stage " << stage << " - initialize=" << initialized_ << endl;

    if (stage == INITSTAGE_LOCAL) {
        selfBurst_ = new cMessage("selfBurst");
        selfPacket_ = new cMessage("selfPacket");
        idBurst_ = 0;
        idFrame_ = 0;
        timestamp_ = 0;
        size_ = par("packetSize");
        burstSize_ = par("burstSize");
        interBurstTime_ = par("interBurstTime");
        intraBurstTime_ = par("intraBurstTime");
        localPort_ = par("localPort");
        destPort_ = par("destPort");
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        initTraffic_ = new cMessage("initTraffic");
        initTraffic();
    }
}

void BurstSender::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (msg == selfBurst_)
            sendBurst();
        else if (msg == selfPacket_)
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

        scheduleAt(simTime() + startTime, selfBurst_);
        EV << "\t starting traffic in " << startTime << " seconds " << endl;
    }
}

void BurstSender::sendBurst()
{
    idBurst_++;
    idFrame_ = 0;

    EV << simTime() << " BurstSender::sendBurst - Start sending burst[" << idBurst_ << "]" << endl;

    // send first packet
    sendPacket();

    scheduleAt(simTime() + interBurstTime_, selfBurst_);
}

void BurstSender::sendPacket()
{
    EV << "BurstSender::sendPacket - Sending frame[" << idFrame_ << "] of burst [" << idBurst_ << "], next packet at " << simTime() + intraBurstTime_ << endl;

    //unsigned int msgId = (idBurst_ << 16) | idFrame_;
    unsigned int msgId = (idBurst_ * burstSize_) + idFrame_;

    Packet *packet = new inet::Packet("Burst");
    auto burst = makeShared<BurstPacket>();

    burst->setMsgId(msgId);
    burst->setPayloadTimestamp(simTime());
    burst->setPayloadSize(size_);
    burst->setChunkLength(B(size_));
    burst->addTag<CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(burst);

    socket.sendTo(packet, destAddress_, destPort_);

    emit(burstSentPktSignal_, (long)msgId);

    idFrame_++;
    if (idFrame_ < burstSize_)
        scheduleAt(simTime() + intraBurstTime_, selfPacket_);
}

} //namespace

