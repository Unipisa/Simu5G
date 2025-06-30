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
#include <inet/common/ModuleAccess.h>
#include <inet/common/TimeTag_m.h>

#include "apps/alert/AlertSender.h"

namespace simu5g {

#define round(x)    floor((x) + 0.5)

Define_Module(AlertSender);
using namespace inet;

simsignal_t AlertSender::alertSentMsgSignal_ = registerSignal("alertSentMsg");

AlertSender::~AlertSender()
{
    cancelAndDelete(selfSender_);
}

void AlertSender::initialize(int stage)
{
    EV << "AlertSender::initialize - stage " << stage << endl;

    cSimpleModule::initialize(stage);

    // avoid multiple initializations
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;

    selfSender_ = new cMessage("selfSender");

    size_ = B(par("packetSize"));
    localPort_ = par("localPort");
    destPort_ = par("destPort");
    destAddress_ = inet::L3AddressResolver().resolve(par("destAddress").stringValue());

    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort_);

    int tos = par("tos");
    if (tos != -1)
        socket.setTos(tos);

    // for multicast support
    inet::IInterfaceTable *ift = inet::getModuleFromPar<inet::IInterfaceTable>(par("interfaceTableModule"), this);
    inet::MulticastGroupList mgl = ift->collectMulticastGroups();
    socket.joinLocalMulticastGroups(mgl);

    // if the multicastInterface parameter is not empty, set the interface explicitly
    const char *multicastInterface = par("multicastInterface");
    if (multicastInterface[0]) {
        NetworkInterface *ie = ift->findInterfaceByName(multicastInterface);
        if (ie == nullptr)
            throw cRuntimeError("Wrong multicastInterface setting: no interface named \"%s\"", multicastInterface);
        socket.setMulticastOutputInterface(ie->getInterfaceId());
    }

    // -------------------- //

    EV << "AlertSender::initialize - binding to port: local:" << localPort_ << " , dest:" << destPort_ << endl;

    // calculating traffic starting time
    simtime_t startTime = par("startTime");
    stopTime_ = par("stopTime");

    // TODO maybe un-necesessary
    // this conversion is made in order to obtain ms-aligned start time, even in case of random generated ones
    simtime_t offset = (round(SIMTIME_DBL(startTime) * 1000) / 1000) + simTime();

    scheduleAt(offset, selfSender_);
    EV << "\t starting traffic in " << offset << " seconds " << endl;
}

void AlertSender::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (msg == selfSender_)
            sendAlertPacket();
        else
            throw cRuntimeError("AlertSender::handleMessage - Unrecognized self message");
    }
}

void AlertSender::sendAlertPacket()
{
    Packet *packet = new inet::Packet("Alert");

    auto alert = makeShared<AlertPacket>();
    alert->setSno(nextSno_);
    alert->setPayloadTimestamp(simTime());
    alert->setChunkLength(size_);
    alert->addTag<CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(alert);

    EV << "AlertSender::sendAlertPacket - Sending message [" << nextSno_ << "]\n";

    socket.sendTo(packet, destAddress_, destPort_);
    nextSno_++;

    emit(alertSentMsgSignal_, (long)1);

    simtime_t d = simTime() + par("period");
    if (stopTime_ <= SIMTIME_ZERO || d < stopTime_) {
        scheduleAt(d, selfSender_);
    }
    else
        EV << "AlertSender::sendAlertPacket - Stop time reached, stopping transmissions" << endl;
}

void AlertSender::refreshDisplay() const
{
    char buf[80];
    sprintf(buf, "sent: %d pks", nextSno_);
    getDisplayString().setTagArg("t", 0, buf);
}

} //namespace

