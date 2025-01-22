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
#include <fstream>

#include <inet/common/ModuleAccess.h>  // for multicast support
#include <inet/common/TimeTag_m.h>
#include <inet/common/packet/chunk/ByteCountChunk.h>

#include "apps/d2dMultihop/MultihopD2D.h"
#include "apps/d2dMultihop/TrickleTimerMsg_m.h"
#include "stack/mac/LteMacBase.h"

namespace simu5g {

#define round(x)    floor((x) + 0.5)

Define_Module(MultihopD2D);

using namespace inet;

uint16_t MultihopD2D::numMultihopD2DApps = 0;

// local statistics
simsignal_t MultihopD2D::d2dMultihopGeneratedMsgSignal_ = registerSignal("d2dMultihopGeneratedMsg");
simsignal_t MultihopD2D::d2dMultihopSentMsgSignal_ = registerSignal("d2dMultihopSentMsg");
simsignal_t MultihopD2D::d2dMultihopRcvdMsgSignal_ = registerSignal("d2dMultihopRcvdMsg");
simsignal_t MultihopD2D::d2dMultihopRcvdDupMsgSignal_ = registerSignal("d2dMultihopRcvdDupMsg");
simsignal_t MultihopD2D::d2dMultihopTrickleSuppressedMsgSignal_ = registerSignal("d2dMultihopTrickleSuppressedMsg");

MultihopD2D::MultihopD2D() : senderAppId_(numMultihopD2DApps++)
{
}

MultihopD2D::~MultihopD2D()
{
    cancelAndDelete(selfSender_);

    if (trickleEnabled_) {
        for (auto& pair : last_)
            if (pair.second != nullptr)
                delete pair.second;
    }
}

void MultihopD2D::initialize(int stage)
{
    // avoid multiple initializations
    if (stage == INITSTAGE_APPLICATION_LAYER) {
        EV << "MultihopD2D initialize: stage " << stage << endl;

        localPort_ = par("localPort");
        destPort_ = par("destPort");
        destAddress_ = L3AddressResolver().resolve(par("destAddress").stringValue());

        msgSize_ = B(par("msgSize"));

        if (msgSize_ < D2D_MULTIHOP_HEADER_LENGTH) {
            throw cRuntimeError("MultihopD2D::init - FATAL! Total message size cannot be less than D2D_MULTIHOP_HEADER_LENGTH");
        }

        maxBroadcastRadius_ = par("maxBroadcastRadius");
        ttl_ = par("ttl");
        maxTransmissionDelay_ = par("maxTransmissionDelay");
        selfishProbability_ = par("selfishProbability");

        trickleEnabled_ = par("trickle").boolValue();
        if (trickleEnabled_) {
            I_ = par("I");
            k_ = par("k");
            if (k_ <= 0)
                throw cRuntimeError("Bad value for k. It must be greater than zero");
        }

        EV << "MultihopD2D::initialize - binding to port: local:" << localPort_ << " , dest:" << destPort_ << endl;
        socket.setOutputGate(gate("socketOut"));
        socket.bind(localPort_);

        int tos = par("tos");
        if (tos != -1)
            socket.setTos(tos);

        // for multicast support
        inet::IInterfaceTable *ift = inet::getModuleFromPar<inet::IInterfaceTable>(par("interfaceTableModule"), this);
        NetworkInterface *ie = ift->findInterfaceByName(par("interfaceName").stringValue());
        if (!ie)
            throw cRuntimeError("Wrong multicastInterface setting: no interface found");
        inet::MulticastGroupList mgl = ift->collectMulticastGroups();
        socket.joinLocalMulticastGroups(mgl);
        socket.setMulticastOutputInterface(ie->getInterfaceId());
        // -------------------- //

        selfSender_ = new cMessage("selfSender", KIND_SELF_SENDER);

        // get references to LTE entities
        ltePhy_.reference(this, "ltePhyModule", true);
        LteMacBase *mac = getModuleFromPar<LteMacBase>(par("lteMacModule"), this);
        lteNodeId_ = mac->getMacNodeId();
        lteCellId_ = mac->getMacCellId();

        // register to the event generator
        eventGen_.reference(this, "eventGeneratorModule", true);
        eventGen_->registerNode(this, lteNodeId_);

        // global statistics recorder
        stat_.reference(this, "multihopD2DStatisticsModule", true);
    }
}

void MultihopD2D::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        switch (msg->getKind()) {
            case KIND_SELF_SENDER:
                sendPacket();
                break;
            case KIND_RELAY:
                msg->setKind(0);
                relayPacket(msg);
                break;
            case KIND_TRICKLE_TIMER:
                handleTrickleTimer(msg);
                break;
            default:
                throw cRuntimeError("Unrecognized self message");
        }
    }
    else {
        if (!strcmp(msg->getName(), "MultihopD2DPacket"))
            handleRcvdPacket(msg);
        else
            throw cRuntimeError("Unrecognized  message");
    }
}

void MultihopD2D::handleEvent(unsigned int eventId)
{
    Enter_Method_Silent("MultihopD2D::handleEvent()");
    EV << simTime() << " MultihopD2D::handleEvent - Received event notification " << endl;

    // the event id will be part of the message id
    localMsgId_ = eventId;

    simtime_t delay = 0.0;
    if (maxTransmissionDelay_ > 0)
        delay = uniform(0, maxTransmissionDelay_);

    simtime_t t = simTime() + (round(SIMTIME_DBL(delay) * 1000) / 1000);
    scheduleAt(t, selfSender_);
}

void MultihopD2D::sendPacket()
{
    // build the global identifier
    uint32_t msgId = ((uint32_t)senderAppId_ << 16) | localMsgId_;

    // create data corresponding to the desired multi-hop message size
    // (msgSize_ is the size of the MultihopD2D message including the MultihopD2D header)
    auto data = makeShared<ByteCountChunk>(msgSize_ - D2D_MULTIHOP_HEADER_LENGTH);

    // create new packet containing the data
    Packet *packet = new inet::Packet("MultihopD2DPacket", data);

    // add header
    auto mhop = makeShared<MultihopD2DPacket>();
    mhop->setMsgid(msgId);
    mhop->setSrcId(lteNodeId_);
    mhop->setPayloadTimestamp(simTime());
    mhop->setPayloadSize(B(msgSize_).get());
    mhop->setTtl(ttl_ - 1);
    mhop->setHops(1);                // first hop
    mhop->setLastHopSenderId(lteNodeId_);
    if (maxBroadcastRadius_ > 0) {
        mhop->setSrcCoord(ltePhy_->getCoord());
        mhop->setMaxRadius(maxBroadcastRadius_);
    }
    mhop->addTag<CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtFront(mhop);

    EV << "MultihopD2D::sendPacket - Sending msg (ID: " << mhop->getMsgid() << " src: " << lteNodeId_ << " size: " << msgSize_ << ")" << endl;
    socket.sendTo(packet, destAddress_, destPort_);

    std::set<MacNodeId> targetSet;
    eventGen_->computeTargetNodeSet(targetSet, lteNodeId_, maxBroadcastRadius_);
    stat_->recordNewBroadcast(msgId, targetSet);
    stat_->recordReception(lteNodeId_, msgId, 0.0, 0);
    markAsRelayed(msgId);

    emit(d2dMultihopGeneratedMsgSignal_, (long)1);
    emit(d2dMultihopSentMsgSignal_, (long)1);
}

void MultihopD2D::handleRcvdPacket(cMessage *msg)
{
    EV << "MultihopD2D::handleRcvdPacket - Received packet from lower layer" << endl;

    Packet *pPacket = check_and_cast<Packet *>(msg);
    auto mhop = pPacket->peekAtFront<MultihopD2DPacket>();
    pPacket->removeControlInfo();

    uint32_t msgId = mhop->getMsgid();

    // check if this is a duplicate
    if (isAlreadyReceived(msgId)) {
        if (trickleEnabled_)
            counter_[msgId]++;

        // do not need to relay the message again
        EV << "MultihopD2D::handleRcvdPacket - The message has already been received, counter = " << counter_[msgId] << endl;

        emit(d2dMultihopRcvdDupMsgSignal_, (long)1);
        stat_->recordDuplicateReception(msgId);
        delete pPacket;
    }
    else {
        // this is a new packet

        // mark the message as received
        markAsReceived(msgId);

        if (trickleEnabled_) {
            counter_[msgId] = 1;

            // store a copy of the packet
            if (last_.find(msgId) != last_.end() && last_[msgId] != nullptr) {
                delete last_[msgId];
                last_[msgId] = nullptr;
            }
            last_[msgId] = pPacket->dup();
        }

        // emit statistics
        simtime_t delay = simTime() - mhop->getPayloadTimestamp();
        emit(d2dMultihopRcvdMsgSignal_, (long)1);
        stat_->recordReception(lteNodeId_, msgId, delay, mhop->getHops());

        // === decide whether to relay the message === //

        // check for selfish behavior of the user
        if (uniform(0.0, 1.0) < selfishProbability_) {
            // selfish user, do not relay
            EV << "MultihopD2D::handleRcvdPacket - The user is selfish, do not forward the message. " << endl;
            delete pPacket;
        }
        else if (mhop->getMaxRadius() > 0 && !isWithinBroadcastArea(mhop->getSrcCoord(), mhop->getMaxRadius())) {
            EV << "MultihopD2D::handleRcvdPacket - The node is outside the broadcast area. Do not forward it. " << endl;
            delete pPacket;
        }
        else if (mhop->getTtl() == 0) {
            // TTL expired
            EV << "MultihopD2D::handleRcvdPacket - The TTL for this message has expired. Do not forward it. " << endl;
            delete pPacket;
        }
        else {
            if (trickleEnabled_) {
                // start Trickle interval timer
                TrickleTimerMsg *timer = new TrickleTimerMsg("trickleTimer", KIND_TRICKLE_TIMER);
                timer->setMsgid(msgId);

                simtime_t t = uniform(I_ / 2, I_);
                t = round(SIMTIME_DBL(t) * 1000) / 1000;
                EV << "MultihopD2D::handleRcvdPacket - start Trickle interval, duration[" << t << "s]" << endl;

                scheduleAt(simTime() + t, timer);
                delete pPacket;
            }
            else {
                // relay the message after some random backoff time
                simtime_t offset = 0.0;
                if (maxTransmissionDelay_ > 0)
                    offset = uniform(0, maxTransmissionDelay_);

                offset = round(SIMTIME_DBL(offset) * 1000) / 1000;
                pPacket->setKind(KIND_RELAY);
                scheduleAt(simTime() + offset, pPacket);
                EV << "MultihopD2D::handleRcvdPacket - will relay the message in " << offset << "s" << endl;
            }
        }
    }
}

void MultihopD2D::handleTrickleTimer(cMessage *msg)
{
    TrickleTimerMsg *timer = check_and_cast<TrickleTimerMsg *>(msg);
    unsigned int msgId = timer->getMsgid();
    if (counter_[msgId] < k_) {
        EV << "MultihopD2D::handleTrickleTimer - relay the message, counter[" << counter_[msgId] << "] k[" << k_ << "]" << endl;
        relayPacket(last_[msgId]->dup());
    }
    else {
        EV << "MultihopD2D::handleTrickleTimer - suppressed message, counter[" << counter_[msgId] << "] k[" << k_ << "]" << endl;
        stat_->recordSuppressedMessage(msgId);
        emit(d2dMultihopTrickleSuppressedMsgSignal_, (long)1);
    }
    delete msg;
}

void MultihopD2D::relayPacket(cMessage *msg)
{
    Packet *pPacket = check_and_cast<Packet *>(msg);
    auto src = pPacket->popAtFront<MultihopD2DPacket>();

    // create a relay packet using the data of the source packet
    auto data = pPacket->peekData();
    Packet *relayPacket = new inet::Packet("MultihopD2DPacket", data);

    // create a new header
    auto dst = makeShared<MultihopD2DPacket>();

    // increase the number of hops
    unsigned int hops = src->getHops();
    dst->setHops(++hops);

    // decrease TTL
    if (src->getTtl() > 0) {
        int ttl = src->getTtl();
        dst->setTtl(--ttl);
    }

    // set this node as last sender
    dst->setLastHopSenderId(lteNodeId_);

    // copy remaining header fields from original packet header
    dst->setSrcId(src->getSrcId());
    dst->setMsgid(src->getMsgid());
    dst->setPayloadSize(src->getPayloadSize());
    dst->setSrcCoord(src->getSrcCoord());
    dst->setMaxRadius(src->getMaxRadius());
    dst->setPayloadTimestamp(src->getPayloadTimestamp());

    relayPacket->insertAtFront(dst);

    EV << "MultihopD2D::relayPacket - Relay msg " << dst->getMsgid() << " to address " << destAddress_ << endl;
    socket.sendTo(relayPacket, destAddress_, destPort_);

    markAsRelayed(dst->getMsgid());    // mark the message as relayed

    emit(d2dMultihopSentMsgSignal_, (long)1);
    stat_->recordSentMessage(dst->getMsgid());

    delete pPacket;
}

void MultihopD2D::markAsReceived(uint32_t msgId)
{
    relayedMsgMap_.insert({msgId, false});
}

bool MultihopD2D::isAlreadyReceived(uint32_t msgId)
{
    return relayedMsgMap_.find(msgId) != relayedMsgMap_.end();
}

void MultihopD2D::markAsRelayed(uint32_t msgId)
{
    relayedMsgMap_[msgId] = true;
}

bool MultihopD2D::isAlreadyRelayed(uint32_t msgId)
{
    if (relayedMsgMap_.find(msgId) == relayedMsgMap_.end()) // the message has not been received
        return false;
    return relayedMsgMap_[msgId];                                    // the message has been received but not relayed yet
}

bool MultihopD2D::isWithinBroadcastArea(Coord srcCoord, double maxRadius)
{
    Coord myCoord = ltePhy_->getCoord();
    double dist = myCoord.distance(srcCoord);
    return dist < maxRadius;
}

void MultihopD2D::finish()
{
    // unregister from the event generator
    eventGen_->unregisterNode(this, lteNodeId_);
}

} //namespace

