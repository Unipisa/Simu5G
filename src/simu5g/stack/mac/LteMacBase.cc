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

#include "simu5g/common/LteControlInfo.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/common/cellInfo/CellInfo.h"
#include "simu5g/stack/mac/LteMacBase.h"
#include "simu5g/stack/mac/buffer/harq/LteHarqBufferTx.h"
#include "simu5g/stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "simu5g/stack/mac/packet/LteMacPdu.h"
#include "simu5g/stack/mac/buffer/LteMacQueue.h"
#include "simu5g/stack/mac/packet/LteHarqFeedback_m.h"
#include "simu5g/stack/mac/packet/LteMacPdu.h"
#include "simu5g/stack/mac/buffer/LteMacBuffer.h"
#include <assert.h>
#include "simu5g/stack/phy/PhyBase.h"
#include "simu5g/stack/packetFlowObserver/PacketFlowSignals.h"

namespace simu5g {

using namespace omnetpp;

simsignal_t LteMacBase::macPduAckedSignal_ = registerSignal("macPduAcked");
simsignal_t LteMacBase::macPduDiscardedSignal_ = registerSignal("macPduDiscarded");
simsignal_t LteMacBase::rlcPduDiscardedSignal_ = registerSignal("rlcPduDiscarded");
simsignal_t LteMacBase::grantSentSignal_ = registerSignal("grantSent");
simsignal_t LteMacBase::ulMacPduArrivedSignal_ = registerSignal("ulMacPduArrived");

// register signals
simsignal_t LteMacBase::macBufferOverflowDlSignal_ = registerSignal("macBufferOverFlowDl");
simsignal_t LteMacBase::macBufferOverflowUlSignal_ = registerSignal("macBufferOverFlowUl");
simsignal_t LteMacBase::receivedPacketFromUpperLayerSignal_ = registerSignal("receivedPacketFromUpperLayer");
simsignal_t LteMacBase::receivedPacketFromLowerLayerSignal_ = registerSignal("receivedPacketFromLowerLayer");
simsignal_t LteMacBase::sentPacketToUpperLayerSignal_ = registerSignal("sentPacketToUpperLayer");
simsignal_t LteMacBase::sentPacketToLowerLayerSignal_ = registerSignal("sentPacketToLowerLayer");

LteMacBase::~LteMacBase()
{
    for (auto& [key, connInfo] : connDescOut_) {
        delete connInfo.queue;
        delete connInfo.buffer;
    }

    for (auto& [key, txBuffers] : harqTxBuffers_)
        for (auto& [key, buffer] : txBuffers)
            delete buffer;

    for (auto& [key, rxBuffers] : harqRxBuffers_)
        for (auto& [key, buffer] : rxBuffers)
            delete buffer;
}

void LteMacBase::sendUpperPackets(cPacket *pkt)
{
    EV << NOW << " LteMacBase::sendUpperPackets, Sending packet " << pkt->getName() << " on port upperLayerOut\n";
    // Send message
    send(pkt, upOutGate_);
    nrToUpper_++;
    emit(sentPacketToUpperLayerSignal_, pkt);
}

void LteMacBase::sendLowerPackets(cPacket *pkt)
{
    EV << NOW << " LteMacBase::sendLowerPackets, Sending packet " << pkt->getName() << " on port phyOut\n";
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
    GHz carrierFreq = userInfo->getCarrierFrequency();

    if (userInfo->getFrameType() == HARQPKT) {
        if (harqTxBuffers_.find(carrierFreq) == harqTxBuffers_.end()) {
            HarqTxBuffers newTxBuffs;
            harqTxBuffers_[carrierFreq] = newTxBuffs;
        }

        // H-ARQ feedback, send it to TX buffer of source
        HarqTxBuffers::iterator htit = harqTxBuffers_[carrierFreq].find(src);
        EV << NOW << " Mac::fromPhy: node " << nodeId_ << " Received HARQ Feedback pkt" << endl;
        if (htit == harqTxBuffers_[carrierFreq].end()) {
            // A feedback for a non-existing HARQ TX buffer is a stale in-flight feedback whose
            // buffer was torn down -- on handover, or after a radio-link-failure teardown /
            // re-establishment. It cannot be processed (the buffer is gone), so drop it
            // gracefully rather than aborting the simulation.
            EV << NOW << " Mac::fromPhy: node " << nodeId_ << " - stale HARQ feedback for src "
               << src << " with no TX buffer (torn down); dropping" << endl;
            return;
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
            LteHarqBufferRx *hrb = createRxHarqBuffer(src, userInfo.get());
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

void LteMacBase::recordBufferOverflow(Direction dir, double sample)
{
    // The core MAC knows only the infrastructure directions; D2D directions are
    // serviced by the D2D MAC subclasses, which override this.
    if (dir == DL)
        emit(macBufferOverflowDlSignal_, sample);
    else if (dir == UL)
        emit(macBufferOverflowUlSignal_, sample);
    else
        throw cRuntimeError("LteMacBase::recordBufferOverflow: direction %s not supported "
                "by the core MAC", dirToA(dir).c_str());
}

LteHarqBufferRx *LteMacBase::createRxHarqBuffer(MacNodeId src, const UserControlInfo *userInfo)
{
    Direction dir = (Direction)userInfo->getDirection();
    if (dir != DL && dir != UL)
        throw cRuntimeError("LteMacBase::createRxHarqBuffer: direction %s not supported", dirToA(dir).c_str());
    return new LteHarqBufferRx(harqProcesses_, this, binder_, src);
}

void LteMacBase::createOutgoingConnection(MacCid cid, const FlowDescriptor& connInfo)
{
    Enter_Method("createOutgoingConnection(%s)", cid.str().c_str());
    EV << "LteMacBase::createOutgoingConnection - CID: " << cid
       << " sourceId: " << connInfo.getSourceId()
       << " destId: " << connInfo.getDestId()
       << " direction: " << dirToA(connInfo.getDirection())
       << " d2dGroupId: " << connInfo.getD2dGroupId() << endl;

    ASSERT(connDescOut_.find(cid) == connDescOut_.end());

    LteMacQueue* realBuffer = new LteMacQueue(queueSize_);
    LteMacBuffer* virtualBuffer = new LteMacBuffer();
    take(realBuffer);

    connDescOut_[cid] = OutgoingConnectionInfo(connInfo, realBuffer, virtualBuffer);

    // register connection to LCG map. The logical-channel configuration RRC pushes via
    // configureLogicalChannel() must already be in place -- the stage-2 both-directions
    // establishment invariant that getLogicalChannelConfig()'s strict throw enforces.
    Lcg lcg = getLogicalChannelConfig(cid).lcg;
    lcgMap_.insert(LcgPair(lcg, CidBufferPair(cid, virtualBuffer)));
}

void LteMacBase::deleteOutgoingConnection(MacCid cid)
{
    auto it = connDescOut_.find(cid);
    if (it == connDescOut_.end())
        throw cRuntimeError("LteMacBase::deleteOutgoingConnection - Connection %s not found", cid.str().c_str());

    OutgoingConnectionInfo& connInfo = it->second;

    // Empty and delete the real buffer
    while (!connInfo.queue->isEmpty())
        delete connInfo.queue->popFront();
    drop(connInfo.queue);
    delete connInfo.queue;

    // Empty and delete the virtual buffer
    while (!connInfo.buffer->isEmpty())
        connInfo.buffer->popFront();
    delete connInfo.buffer;

    // Remove from LCG map
    for (auto lt = lcgMap_.begin(); lt != lcgMap_.end(); ) {
        if (lt->second.first == cid)
            lt = lcgMap_.erase(lt);
        else
            ++lt;
    }

    // Remove from connection descriptor map
    connDescOut_.erase(it);
}

void LteMacBase::clearOutgoingConnectionBuffers(MacCid cid)
{
    auto it = connDescOut_.find(cid);
    if (it == connDescOut_.end())
        throw cRuntimeError("LteMacBase::clearOutgoingConnectionBuffers - Connection %s not found", cid.str().c_str());

    OutgoingConnectionInfo& connInfo = it->second;

    // Empty the real buffer (drop all packets)
    while (!connInfo.queue->isEmpty())
        delete connInfo.queue->popFront();

    // Empty the virtual buffer
    while (!connInfo.buffer->isEmpty())
        connInfo.buffer->popFront();
}

void LteMacBase::createIncomingConnection(MacCid cid, const FlowDescriptor& connInfo)
{
    Enter_Method("createIncomingConnection(%s)", cid.str().c_str());
    EV << "LteMacBase::createIncomingConnection - CID: " << cid
       << " sourceId: " << connInfo.getSourceId()
       << " destId: " << connInfo.getDestId()
       << " direction: " << dirToA(connInfo.getDirection())
       << " d2dGroupId: " << connInfo.getD2dGroupId() << endl;

    ASSERT(connDescIn_.find(cid) == connDescIn_.end());
    connDescIn_[cid] = connInfo;
}

void LteMacBase::configureLogicalChannel(MacCid cid, const LogicalChannelConfig& cfg)
{
    Enter_Method("configureLogicalChannel(%s)", cid.str().c_str());
    EV << "LteMacBase::configureLogicalChannel - CID: " << cid
       << " rlcMode: " << rlcModeToA(cfg.rlcMode)
       << " soFraming: " << cfg.soFraming
       << " snFieldLength: " << cfg.snFieldLength << endl;

    lcConfig_[cid] = cfg;
}

const LogicalChannelConfig& LteMacBase::getLogicalChannelConfig(MacCid cid) const
{
    auto it = lcConfig_.find(cid);
    if (it == lcConfig_.end())
        throw cRuntimeError("LteMacBase: No logical channel configuration for %s", cid.str().c_str());
    return it->second;
}

// note: this method is never called, as it is overridden (in the same way!) in both LteMacEnb and LteMacUe
bool LteMacBase::bufferizePacket(cPacket *cpkt)
{
    auto pkt = check_and_cast<Packet *>(cpkt);

    pkt->setTimestamp();        // Add timestamp with current time to the packet

    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();

    // obtain the CID from the packet information
    MacCid cid = ctrlInfoToMacCid(lteInfo.get());

    // check if queues exist
    if (connDescOut_.find(cid) == connDescOut_.end())
        //TODO this is dead code -- this throw needs to be added in subclasses too!!!!!!!!!!!
        throw cRuntimeError("LteMacBase::bufferizePacket - Buffer for CID %s not found. Connection must be established via the BearerConfigurator before use.", cid.str().c_str());

    OutgoingConnectionInfo& connInfo = connDescOut_.at(cid);
    LteMacQueue *queue = connInfo.queue;
    LteMacBuffer *vqueue = connInfo.buffer;

    bool dropped = !queue->pushBack(pkt);

    if (dropped) {
        totalOverflowedBytes_ += pkt->getByteLength();
        double sample = (double)totalOverflowedBytes_ / (NOW - getSimulation()->getWarmupPeriod());
        recordBufferOverflow((Direction)lteInfo->getDirection(), sample);

        EV << "LteMacBuffers : Dropped packet: queue " << cid << " is full\n";
        delete pkt;
        return false;
    }

    // build the virtual packet corresponding to this incoming packet
    PacketInfo vpkt(pkt->getByteLength(), pkt->getTimestamp());
    vqueue->pushBack(vpkt);

    int64_t spaceLeft = queue->getQueueSize() - queue->getByteLength();
    EV << "LteMacBuffers : Using buffer for " << cid << ", Space left in the Queue: " << spaceLeft << "\n";

    // After bufferization buffers must be synchronized
    ASSERT(connInfo.queue->getQueueLength() == connInfo.buffer->getQueueLength());
    return true;
}

void LteMacBase::deleteQueues(MacNodeId nodeId)
{
    // Create a list of outgoing connections CIDs to delete
    std::vector<MacCid> cidsToDelete;
    for (const auto& [cid, connInfo] : connDescOut_)
        if (cid.getNodeId() == nodeId)
            cidsToDelete.push_back(cid);

    for (const auto& cid : cidsToDelete)
        deleteOutgoingConnection(cid);

    // delete incoming connection descriptors for the departing node
    for (auto it = connDescIn_.begin(); it != connDescIn_.end(); ) {
        if (it->first.getNodeId() == nodeId) {
            it = connDescIn_.erase(it);
        }
        else {
            ++it;
        }
    }

    // delete logical channel configuration for the departing node (shared by both
    // directions, so this alone covers connDescOut_ and connDescIn_ above)
    for (auto it = lcConfig_.begin(); it != lcConfig_.end(); ) {
        if (it->first.getNodeId() == nodeId) {
            it = lcConfig_.erase(it);
        }
        else {
            ++it;
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

void LteMacBase::deleteQueuesRadioLinkFailure(MacNodeId nodeId)
{
    Enter_Method_Silent();
    EV << NOW << " LteMacBase::deleteQueuesRadioLinkFailure - RLF teardown for node " << nodeId << endl;
    // Ignore any in-flight HARQ feedback for this node for the rest of this TTI
    // (isHarqReset() consults resetHarq_), then tear down all MAC/HARQ/connection
    // state. deleteQueues() is virtual -> dispatches to the LteMacEnb/LteMacUe override.
    resetHarq_[nodeId] = NOW;
    deleteQueues(nodeId);
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
        upInGate_ = gate("upperLayerIn");
        upOutGate_ = gate("upperLayerOut");
        downInGate_ = gate("phyIn");
        downOutGate_ = gate("phyOut");

        // Create buffers
        queueSize_ = par("queueSize");

        // Get reference to binder
        binder_.reference(this, "binderModule", true);

        // get the reference to the PHY layer
        phy_ = check_and_cast<PhyBase *>(downOutGate_->getPathEndGate()->getOwnerModule());

        // Set the MAC MIB

        harqProcesses_ = par("harqProcesses");

        // statistics
        statDisplay_ = par("statDisplay");

        WATCH(cellId_);
        WATCH(totalOverflowedBytes_);
        WATCH(lcConfig_);
        WATCH(numerologyPeriodCounter_);
        WATCH(resetHarq_);
        WATCH(harqTxBuffers_);
        WATCH(harqRxBuffers_);
        WATCH(nrFromUpper_);
        WATCH(nrFromLower_);
        WATCH(nrToUpper_);
        WATCH(nrToLower_);
        WATCH(totalHarqErrorRateDlSum_);
        WATCH(totalHarqErrorRateUlSum_);
        WATCH(totalHarqErrorRateDlCount_);
        WATCH(totalHarqErrorRateUlCount_);
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
        // message from phyIn gate (from lower layer)
        emit(receivedPacketFromLowerLayerSignal_, pkt);
        nrFromLower_++;
        fromPhy(pkt);
    }
    else {
        // message from upperLayerIn gate (from upper layer)
        emit(receivedPacketFromUpperLayerSignal_, pkt);
        nrFromUpper_++;
        fromRlc(pkt);
    }
}

void LteMacBase::harqAckToFlowObserver(const inet::Packet *macPdu)
{
    if (hasListeners(macPduAckedSignal_)) {
        auto lteInfo = macPdu->getTag<UserControlInfo>();
        Direction dir = lteInfo->getDirection();
        if (dir == DL || dir == UL) {
            auto pdu = macPdu->peekAtFront<LteMacPdu>();
            MacPduSignalInfo info(lteInfo->getDestId(), pdu.get());
            emit(macPduAckedSignal_, &info);
        }
    }
}

void LteMacBase::discardMacPdu(const inet::Packet *macPdu)
{
    if (hasListeners(macPduDiscardedSignal_)) {
        auto lteInfo = macPdu->getTag<UserControlInfo>();
        Direction dir = lteInfo->getDirection();
        if (dir == DL || dir == UL) {
            auto pdu = macPdu->peekAtFront<LteMacPdu>();
            MacPduSignalInfo info(lteInfo->getDestId(), pdu.get());
            emit(macPduDiscardedSignal_, &info);
        }
    }
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
