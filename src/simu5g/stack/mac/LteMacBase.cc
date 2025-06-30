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

#include "common/LteControlInfo.h"
#include "common/binder/Binder.h"
#include "common/cellInfo/CellInfo.h"
#include "stack/mac/LteMacBase.h"
#include "stack/mac/buffer/harq/LteHarqBufferTx.h"
#include "stack/mac/buffer/harq_d2d/LteHarqBufferRxD2D.h"
#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/packet/LteMacPdu.h"
#include "stack/mac/buffer/LteMacQueue.h"
#include "stack/mac/packet/LteHarqFeedback_m.h"
#include "stack/mac/packet/LteMacPdu.h"
#include "stack/mac/buffer/LteMacBuffer.h"
#include "assert.h"
#include "stack/packetFlowManager/PacketFlowManagerBase.h"
#include "stack/phy/LtePhyBase.h"

namespace simu5g {

using namespace omnetpp;

// register signals
simsignal_t LteMacBase::macBufferOverflowDlSignal_ = registerSignal("macBufferOverFlowDl");
simsignal_t LteMacBase::macBufferOverflowUlSignal_ = registerSignal("macBufferOverFlowUl");
simsignal_t LteMacBase::macBufferOverflowD2DSignal_ = registerSignal("macBufferOverFlowD2D");
simsignal_t LteMacBase::receivedPacketFromUpperLayerSignal_ = registerSignal("receivedPacketFromUpperLayer");
simsignal_t LteMacBase::receivedPacketFromLowerLayerSignal_ = registerSignal("receivedPacketFromLowerLayer");
simsignal_t LteMacBase::sentPacketToUpperLayerSignal_ = registerSignal("sentPacketToUpperLayer");
simsignal_t LteMacBase::sentPacketToLowerLayerSignal_ = registerSignal("sentPacketToLowerLayer");

LteMacBase::~LteMacBase()
{
    for (auto& [key, buffer] : mbuf_)
        delete buffer;
    for (auto& [key, buffer] : macBuffers_)
        delete buffer;

    for (auto& [key, txBuffers] : harqTxBuffers_)
        for (auto& [key, buffer] : txBuffers)
            delete buffer;

    for (auto& [key, rxBuffers] : harqRxBuffers_)
        for (auto& [key, buffer] : rxBuffers)
            delete buffer;
}

void LteMacBase::sendUpperPackets(cPacket *pkt)
{
    EV << NOW << " LteMacBase::sendUpperPackets, Sending packet " << pkt->getName() << " on port MAC_to_RLC\n";
    // Send message
    send(pkt, upOutGate_);
    nrToUpper_++;
    emit(sentPacketToUpperLayerSignal_, pkt);
}

void LteMacBase::sendLowerPackets(cPacket *pkt)
{
    EV << NOW << " LteMacBase::sendLowerPackets, Sending packet " << pkt->getName() << " on port MAC_to_PHY\n";
    // Send message
    updateUserTxParam(pkt);
    send(pkt, downOutGate_);
    nrToLower_++;
    emit(sentPacketToLowerLayerSignal_, pkt);
}

/*
 * UE with nodeId left the simulation. Ensure that no
 * signals will be emitted via the deleted node.
 */
void LteMacBase::unregisterHarqBufferRx(MacNodeId nodeId) {

    for (auto& [key, harqRxBuffers] : harqRxBuffers_) {
        auto it = harqRxBuffers.find(nodeId);
        if (it != harqRxBuffers.end()) {
            it->second->unregister_macUe();
        }
    }
}

/*
 * Upper layer handler
 */
void LteMacBase::fromRlc(cPacket *pkt)
{
    handleUpperMessage(pkt);
}

/*
 * Lower layer handler
 */
void LteMacBase::fromPhy(cPacket *pktAux)
{
    // TODO: HARQ test (comment fromPhy: it has only to pass PDUs to the proper RX buffer and
    // to manage H-ARQ feedback)

    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto userInfo = pkt->getTag<UserControlInfo>();

    MacNodeId src = userInfo->getSourceId();
    double carrierFreq = userInfo->getCarrierFrequency();

    if (userInfo->getFrameType() == HARQPKT) {
        if (harqTxBuffers_.find(carrierFreq) == harqTxBuffers_.end()) {
            HarqTxBuffers newTxBuffs;
            harqTxBuffers_[carrierFreq] = newTxBuffs;
        }

        // H-ARQ feedback, send it to TX buffer of source
        HarqTxBuffers::iterator htit = harqTxBuffers_[carrierFreq].find(src);
        EV << NOW << " Mac::fromPhy: node " << nodeId_ << " Received HARQ Feedback pkt" << endl;
        if (htit == harqTxBuffers_[carrierFreq].end()) {
            // if a feedback arrives, a TX buffer must exist (unless it is a handover scenario
            // where the HARQ buffer was deleted but a feedback was in transit)
            // this case must be taken care of

            if (binder_->hasUeHandoverTriggered(nodeId_) || binder_->hasUeHandoverTriggered(src))
                return;

            throw cRuntimeError("Mac::fromPhy(): Received feedback for a non-existing H-ARQ TX buffer");
        }

        auto hfbpkt = pkt->peekAtFront<LteHarqFeedback>();
        htit->second->receiveHarqFeedback(pkt);
    }
    else if (userInfo->getFrameType() == FEEDBACKPKT) {
        // Feedback pkt
        EV << NOW << " Mac::fromPhy: node " << nodeId_ << " Received feedback pkt" << endl;
        macHandleFeedbackPkt(pkt);
    }
    else if (userInfo->getFrameType() == GRANTPKT) {
        // Scheduling Grant
        EV << NOW << " Mac::fromPhy: node " << nodeId_ << " Received Scheduling Grant pkt" << endl;
        macHandleGrant(pkt);
    }
    else if (userInfo->getFrameType() == DATAPKT) {
        // data packet: insert in proper RX buffer
        EV << NOW << " Mac::fromPhy: node " << nodeId_ << " Received DATA packet" << endl;

        auto pduAux = pkt->peekAtFront<LteMacPdu>();
        auto pdu = pkt;
        Codeword cw = userInfo->getCw();

        if (harqRxBuffers_.find(carrierFreq) == harqRxBuffers_.end()) {
            HarqRxBuffers newRxBuffs;
            harqRxBuffers_[carrierFreq] = newRxBuffs;
        }

        HarqRxBuffers::iterator hrit = harqRxBuffers_[carrierFreq].find(src);
        if (hrit != harqRxBuffers_[carrierFreq].end()) {
            hrit->second->insertPdu(cw, pdu);
        }
        else {
            // FIXME: possible memory leak
            LteHarqBufferRx *hrb;
            if (userInfo->getDirection() == DL || userInfo->getDirection() == UL)
                hrb = new LteHarqBufferRx(ENB_RX_HARQ_PROCESSES, this, binder_, src);
            else // D2D
                hrb = new LteHarqBufferRxD2D(ENB_RX_HARQ_PROCESSES, this, binder_, src, (userInfo->getDirection() == D2D_MULTI));

            harqRxBuffers_[carrierFreq][src] = hrb;
            hrb->insertPdu(cw, pdu);
        }
    }
    else if (userInfo->getFrameType() == RACPKT) {
        EV << NOW << " Mac::fromPhy: node " << nodeId_ << " Received RAC packet" << endl;
        macHandleRac(pkt);
    }
    else {
        throw cRuntimeError("Unknown packet type %d", (int)userInfo->getFrameType());
    }
}

bool LteMacBase::bufferizePacket(cPacket *pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);

    pkt->setTimestamp();        // Add timestamp with current time to the packet

    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();

    // obtain the CID from the packet information
    MacCid cid = ctrlInfoToMacCid(lteInfo);

    // build the virtual packet corresponding to this incoming packet
    PacketInfo vpkt(pkt->getByteLength(), pkt->getTimestamp());

    LteMacBuffers::iterator it = mbuf_.find(cid);
    if (it == mbuf_.end()) {
        // Queue not found for this CID: create
        LteMacQueue *queue = new LteMacQueue(queueSize_);
        take(queue);
        LteMacBuffer *vqueue = new LteMacBuffer();

        queue->pushBack(pkt);
        vqueue->pushBack(vpkt);
        mbuf_[cid] = queue;
        macBuffers_[cid] = vqueue;

        // make a copy of LTE control info and store it in the traffic descriptors map
        FlowControlInfo toStore(*lteInfo);
        connDesc_[cid] = toStore;
        // register connection to LCG map.
        LteTrafficClass tClass = (LteTrafficClass)lteInfo->getTraffic();

        lcgMap_.insert(LcgPair(tClass, CidBufferPair(cid, macBuffers_[cid])));

        EV << "LteMacBuffers : Using new buffer on node: " <<
            MacCidToNodeId(cid) << " for LCID: " << MacCidToLcid(cid) << ", Space left in the Queue: " <<
            queue->getQueueSize() - queue->getByteLength() << "\n";
    }
    else {
        // Found
        LteMacQueue *queue = it->second;

        LteMacBuffer *vqueue = nullptr;
        LteMacBufferMap::iterator it = macBuffers_.find(cid);
        if (it != macBuffers_.end())
            vqueue = it->second;

        if (!queue->pushBack(pkt)) {
            totalOverflowedBytes_ += pkt->getByteLength();
            double sample = (double)totalOverflowedBytes_ / (NOW - getSimulation()->getWarmupPeriod());
            if (lteInfo->getDirection() == DL) {
                emit(macBufferOverflowDlSignal_, sample);
            }
            else if (lteInfo->getDirection() == UL) {
                emit(macBufferOverflowUlSignal_, sample);
            }
            else { // D2D
                emit(macBufferOverflowD2DSignal_, sample);
            }

            EV << "LteMacBuffers : Dropped packet: queue" << cid << " is full\n";
            delete pkt;
            return false;
        }

        if (vqueue != nullptr) {
            vqueue->pushBack(vpkt);

            EV << "LteMacBuffers : Using old buffer on node: " <<
                MacCidToNodeId(cid) << " for LCID: " << MacCidToLcid(cid) << ", Space left in the Queue: " <<
                queue->getQueueSize() - queue->getByteLength() << "\n";
        }
        else
            throw cRuntimeError("LteMacBase::bufferizePacket - cannot find MAC buffer for CID %d", cid);
    }
    /// After bufferization buffers must be synchronized
    assert(mbuf_[cid]->getQueueLength() == macBuffers_[cid]->getQueueLength());
    return true;
}

void LteMacBase::deleteQueues(MacNodeId nodeId)
{
    for (auto mit = mbuf_.begin(); mit != mbuf_.end(); ) {
        if (MacCidToNodeId(mit->first) == nodeId) {
            while (!mit->second->isEmpty()) {
                cPacket *pkt = mit->second->popFront();
                delete pkt;
            }
            delete mit->second;        // Delete Queue
            mit = mbuf_.erase(mit);    // Delete Element
        }
        else {
            ++mit;
        }
    }
    for (auto vit = macBuffers_.begin(); vit != macBuffers_.end(); ) {
        if (MacCidToNodeId(vit->first) == nodeId) {
            while (!vit->second->isEmpty())
                vit->second->popFront();
            delete vit->second;        // Delete Queue
            vit = macBuffers_.erase(vit);        // Delete Element
        }
        else {
            ++vit;
        }
    }

    // delete H-ARQ buffers
    for (auto& [key, harqBuffers] : harqTxBuffers_) {
        for (auto hit = harqBuffers.begin(); hit != harqBuffers.end(); ) {
            if (hit->first == nodeId) {
                delete hit->second; // Delete Queue
                hit = harqBuffers.erase(hit); // Delete Element
            }
            else {
                ++hit;
            }
        }
    }

    for (auto& [key, harqBuffers] : harqRxBuffers_) {
        for (auto hit2 = harqBuffers.begin(); hit2 != harqBuffers.end(); ) {
            if (hit2->first == nodeId) {
                delete hit2->second; // Delete Queue
                hit2 = harqBuffers.erase(hit2); // Delete Element
            }
            else {
                ++hit2;
            }
        }
    }

    // TODO remove traffic descriptor and LCG entry
}

void LteMacBase::decreaseNumerologyPeriodCounter()
{
    for (auto& [index, counter] : numerologyPeriodCounter_) {
        if (counter.current == 0) // reset
            counter.current = counter.max - 1;
        else
            counter.current--;
    }
}

/*
 * Main functions
 */
void LteMacBase::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        networkNode_ = getContainingNode(this);

        // Gates initialization
        upInGate_ = gate("RLC_to_MAC");
        upOutGate_ = gate("MAC_to_RLC");
        downInGate_ = gate("PHY_to_MAC");
        downOutGate_ = gate("MAC_to_PHY");

        // Create buffers
        queueSize_ = par("queueSize");

        // Get reference to binder
        binder_.reference(this, "binderModule", true);

        // get the reference to the PHY layer
        phy_ = check_and_cast<LtePhyBase *>(downOutGate_->getPathEndGate()->getOwnerModule());

        // Set the MAC MIB

        muMimo_ = par("muMimo");

        harqProcesses_ = par("harqProcesses");

        // statistics
        statDisplay_ = par("statDisplay");

        totalOverflowedBytes_ = 0;
        nrFromUpper_ = 0;
        nrFromLower_ = 0;
        nrToUpper_ = 0;
        nrToLower_ = 0;

        packetFlowManager_.reference(this, "packetFlowManagerModule", false);

        WATCH(queueSize_);
        WATCH(nodeId_);
        WATCH_MAP(mbuf_);
        WATCH_MAP(macBuffers_);
    }
}

void LteMacBase::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        handleSelfMessage();
        scheduleAt(NOW + ttiPeriod_, ttiTick_);
        return;
    }

    cPacket *pkt = check_and_cast<cPacket *>(msg);
    EV << "LteMacBase : Received packet " << pkt->getName() <<
        " from port " << pkt->getArrivalGate()->getName() << endl;

    cGate *incoming = pkt->getArrivalGate();

    if (incoming == downInGate_) {
        // message from PHY_to_MAC gate (from lower layer)
        emit(receivedPacketFromLowerLayerSignal_, pkt);
        nrFromLower_++;
        fromPhy(pkt);
    }
    else {
        // message from RLC_to_MAC gate (from upper layer)
        emit(receivedPacketFromUpperLayerSignal_, pkt);
        nrFromUpper_++;
        fromRlc(pkt);
    }
}

void LteMacBase::insertMacPdu(const inet::Packet *macPdu)
{
    auto lteInfo = macPdu->getTag<UserControlInfo>();
    Direction dir = (Direction)lteInfo->getDirection();
    if (packetFlowManager_ != nullptr && (dir == DL || dir == UL)) {
        EV << "LteMacBase::insertMacPdu" << endl;
        auto pdu = macPdu->peekAtFront<LteMacPdu>();
        packetFlowManager_->insertMacPdu(pdu);
    }
}

void LteMacBase::harqAckToFlowManager(inet::Ptr<const UserControlInfo> lteInfo, inet::Ptr<const LteMacPdu> macPdu)
{
    Direction dir = (Direction)lteInfo->getDirection();
    if (packetFlowManager_ != nullptr && (dir == DL || dir == UL))
        packetFlowManager_->macPduArrived(macPdu);
}

void LteMacBase::discardMacPdu(const inet::Packet *macPdu)
{
    auto lteInfo = macPdu->getTag<UserControlInfo>();
    Direction dir = (Direction)lteInfo->getDirection();
    if (packetFlowManager_ != nullptr && (dir == DL || dir == UL)) {
        auto pdu = macPdu->peekAtFront<LteMacPdu>();
        packetFlowManager_->discardMacPdu(pdu);
    }
}

void LteMacBase::discardRlcPdu(inet::Ptr<const UserControlInfo> lteInfo, unsigned int rlcSno)
{
    Direction dir = (Direction)lteInfo->getDirection();
    LogicalCid lcid = lteInfo->getLcid();
    if (packetFlowManager_ != nullptr && (dir == DL || dir == UL))
        packetFlowManager_->discardRlcPdu(lcid, rlcSno);
}

void LteMacBase::finish()
{
}

void LteMacBase::deleteModule() {
    cancelAndDelete(ttiTick_);
    cSimpleModule::deleteModule();
}

void LteMacBase::refreshDisplay() const
{
    if (statDisplay_) {
        char buf[80];

        sprintf(buf, "hl: %ld in, %ld out\nll: %ld in, %ld out", nrFromUpper_, nrToUpper_, nrFromLower_, nrToLower_);

        getDisplayString().setTagArg("t", 0, buf);
        getDisplayString().setTagArg("bgtt", 0, "Number of packets in and out of the higher layer (hl) and the lower layer (ll).");
    }
}

void LteMacBase::recordHarqErrorRate(unsigned int sample, Direction dir)
{
    if (dir == DL) {
        totalHarqErrorRateDlSum_ += sample;
        totalHarqErrorRateDlCount_++;
    }
    if (dir == UL) {
        totalHarqErrorRateUlSum_ += sample;
        totalHarqErrorRateUlCount_++;
    }
}

double LteMacBase::getHarqErrorRate(Direction dir)
{
    if (dir == DL)
        return (double)totalHarqErrorRateDlSum_ / totalHarqErrorRateDlCount_;
    if (dir == UL)
        return (double)totalHarqErrorRateUlSum_ / totalHarqErrorRateUlCount_;
    throw cRuntimeError("LteMacBase::getHarqErrorRate - unhandled direction %d", dir);
}

} //namespace

