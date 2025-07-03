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
#include "simu5g/apps/voip/VoipSender.h"

namespace simu5g {

#define round(x)    floor((x) + 0.5)

Define_Module(VoipSender);
using namespace inet;

simsignal_t VoipSender::voIPGeneratedThroughputSignal_ = registerSignal("voIPGeneratedThroughput");

VoipSender::~VoipSender()
{
    cancelAndDelete(selfSource_);
    cancelAndDelete(selfSender_);
}

void VoipSender::initialize(int stage)
{
    EV << "VoIP Sender initialize: stage " << stage << endl;

    cSimpleModule::initialize(stage);

    // avoid multiple initializations
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;

    durTalk_ = 0;
    durSil_ = 0;
    selfSource_ = new cMessage("selfSource");
    scaleTalk_ = par("scaleTalk");
    shapeTalk_ = par("shapeTalk");
    scaleSil_ = par("scaleSil");
    shapeSil_ = par("shapeSil");
    isTalk_ = par("isTalk");
    iDtalk_ = 0;
    nframes_ = 0;
    nframesTmp_ = 0;
    iDframe_ = 0;
    timestamp_ = 0;
    size_ = par("packetSize");
    sampling_time = par("samplingTime");
    selfSender_ = new cMessage("selfSender");
    localPort_ = par("localPort");
    destPort_ = par("destPort");
    silences_ = par("silences");

    totalSentBytes_ = 0;
    warmUpPer_ = getSimulation()->getWarmupPeriod();

    initTraffic_ = new cMessage("initTraffic");
    initTraffic();
}

void VoipSender::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (msg == selfSender_)
            sendVoIPPacket();
        else if (msg == selfSource_)
            selectPeriodTime();
        else
            initTraffic();
    }
}

void VoipSender::initTraffic()
{
    std::string destAddress = par("destAddress").stringValue();
    cModule *destModule = findModuleByPath(par("destAddress").stringValue());
    if (destModule == nullptr) {
        // this might happen when users are created dynamically
        EV << simTime() << "VoipSender::initTraffic - destination " << destAddress << " not found" << endl;

        simtime_t offset = 0.01; // TODO check value
        scheduleAt(simTime() + offset, initTraffic_);
        EV << simTime() << "VoipSender::initTraffic - the node will retry to initialize traffic in " << offset << " seconds " << endl;
    }
    else {
        delete initTraffic_;

        socket.setOutputGate(gate("socketOut"));
        socket.bind(localPort_);

        int tos = par("tos");
        if (tos != -1)
            socket.setTos(tos);

        EV << simTime() << "VoipSender::initialize - binding to port: local:" << localPort_ << " , dest: " << destAddress_.str() << ":" << destPort_ << endl;

        // calculating traffic starting time
        simtime_t startTime = par("startTime");

        scheduleAt(simTime() + startTime, selfSource_);
        EV << "\t starting traffic in " << startTime << " seconds " << endl;
    }
}

void VoipSender::talkspurt(simtime_t dur)
{
    iDtalk_++;
    nframes_ = (ceil(dur / sampling_time));

    // a talkspurt must be at least 1 frame long
    if (nframes_ == 0)
        nframes_ = 1;

    EV << "VoipSender::talkspurt - TALKSPURT[" << iDtalk_ - 1 << "] - Will be created[" << nframes_ << "] frames\n\n";

    iDframe_ = 0;
    nframesTmp_ = nframes_;
    scheduleAt(simTime(), selfSender_);
}

void VoipSender::selectPeriodTime()
{
    if (!isTalk_) {
        double durSil2;
        if (silences_) {
            durSil_ = weibull(scaleSil_, shapeSil_);
            durSil2 = round(SIMTIME_DBL(durSil_) * 1000) / 1000;
        }
        else {
            durSil_ = durSil2 = 0;
        }

        EV << "VoipSender::selectPeriodTime - Silence Period: " << "Duration[" << durSil_ << "/" << durSil2 << "] seconds\n";
        scheduleAt(simTime() + durSil_, selfSource_);
        isTalk_ = true;
    }
    else {
        durTalk_ = weibull(scaleTalk_, shapeTalk_);
        double durTalk2 = round(SIMTIME_DBL(durTalk_) * 1000) / 1000;
        EV << "VoipSender::selectPeriodTime - Talkspurt[" << iDtalk_ << "] - Duration[" << durTalk_ << "/" << durTalk2 << "] seconds\n";
        talkspurt(durTalk_);
        scheduleAt(simTime() + durTalk_, selfSource_);
        isTalk_ = false;
    }
}

void VoipSender::sendVoIPPacket()
{
    if (destAddress_.isUnspecified())
        destAddress_ = L3AddressResolver().resolve(par("destAddress"));

    Packet *packet = new inet::Packet("VoIP");
    auto voip = makeShared<VoipPacket>();
    voip->setIDtalk(iDtalk_ - 1);
    voip->setNframes(nframes_);
    voip->setIDframe(iDframe_);
    voip->setPayloadTimestamp(simTime());
    voip->setChunkLength(B(size_));
    voip->addTag<CreationTimeTag>()->setCreationTime(simTime());
    packet->insertAtBack(voip);
    EV << "VoipSender::sendVoIPPacket - Talkspurt[" << iDtalk_ - 1 << "] - Sending frame[" << iDframe_ << "]\n";

    socket.sendTo(packet, destAddress_, destPort_);
    --nframesTmp_;
    ++iDframe_;

    // emit throughput sample
    totalSentBytes_ += size_;
    double interval = SIMTIME_DBL(simTime() - warmUpPer_);
    if (interval > 0.0) {
        double tputSample = (double)totalSentBytes_ / interval;
        emit(voIPGeneratedThroughputSignal_, tputSample);
    }

    if (nframesTmp_ > 0)
        scheduleAt(simTime() + sampling_time, selfSender_);
}

} //namespace

