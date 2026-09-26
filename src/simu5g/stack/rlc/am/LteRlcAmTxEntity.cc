//
//                  Simu5G
//
// Authors: Esteban Egea Lopez (Universidad PolitÃ©cnica de Cartagena), Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include <set>

#include <inet/common/ProtocolTag_m.h>

#include "simu5g/stack/rlc/am/LteRlcAmTxEntity.h"
#include "simu5g/stack/mac/packet/LteMacSduRequest.h"
#include "simu5g/stack/pdcp/packet/LtePdcpPdu_m.h"
#include "simu5g/stack/rlc/packet/LteRlcNewDataTag_m.h"
#include "simu5g/stack/rlc/packet/LteRlcPdu_m.h"
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"

namespace simu5g {

Define_Module(LteRlcAmTxEntity);

LteRlcAmTxEntity::~LteRlcAmTxEntity()
{
    if (tPollRetransmitTimer_)
        cancelAndDelete(tPollRetransmitTimer_);
    delete rtxBuffer_;
    for (auto *sdu : sduQueue_)
        delete sdu;
    for (auto& [sn, entry] : txWindow_)
        delete entry.pdu;
    for (auto *pkt : controlBuffer_)
        delete pkt;
}

void LteRlcAmTxEntity::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        amWindowSize_ = par("AM_Window_Size");
        pollPdu_ = par("pollPDU");
        pollByte_ = par("pollByte");
        maxRtxThreshold_ = par("maxRtxThreshold");
        tPollRetransmit_ = par("t_PollRetransmit");
        tPollRetransmitTimer_ = new cMessage("t_PollRetransmit timer");

        rtxBuffer_ = new RlcRetransmissionBuffer(maxRtxThreshold_);
        lastSduSample_ = NOW;

        WATCH(flowControlInfo_);
        WATCH(radioLinkFailureDetected_);
        WATCH(frontOffset_);
        WATCH(sduQueueBytes_);
        WATCH(txWindow_);
        WATCH(txNext_);
        WATCH(txNextAck_);
        WATCH(pduWithoutPoll_);
        WATCH(byteWithoutPoll_);
        WATCH(pollSn_);
        WATCH(pollPending_);
        WATCH(lastSduSample_);
        WATCH(sduSampleBytes_);
    }
}

void LteRlcAmTxEntity::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (msg == tPollRetransmitTimer_) {
            pollPending_ = true;
            bool noPendingData = (sduQueue_.empty() && rtxBuffer_->getRetxPendingBytes() == 0);

            if (noPendingData || windowFull()) {
                // TS 36.322 5.2.2.3: the poll went unanswered, so consider an outstanding
                // PDU for retransmission. That is a retransmission like any other, so it
                // opens a new round and advances RETX_COUNT -- which is what lets a link
                // that has gone silent altogether reach maxRtxThreshold and fail, rather
                // than stall here forever.
                rtxBuffer_->beginRetxRound();

                // Consider the highest-SN PDU still unacknowledged, or failing that the
                // oldest one (mirrors NrRlcAmTxEntity; map presence == unacknowledged, so
                // the last/first map entries are exactly those PDUs).
                if (!txWindow_.empty()) {
                    auto it = std::prev(txWindow_.end());
                    if (!rtxBuffer_->addNack(it->first, true, 0, it->second.payloadLength - 1)) {
                        declareRadioLinkFailure();
                        return;
                    }
                }
            }

            // Tell MAC there is something to send, whether the poll queued a
            // retransmission just now or an earlier one is still waiting -- without
            // this, a pending retransmission would never be granted once the upper
            // layer goes idle (see the same logic in NrRlcAmTxEntity).
            reportBufferStatus();

            // Keep the poll cycle alive while anything is pending -- not only
            // unacknowledged PDUs but also queued SDUs: a grant issued for an
            // announced SDU may have been spent on a retransmission instead, and
            // the periodic reportBufferStatus() above is what re-announces the
            // SDU to the MAC in that case.
            if (!radioLinkFailureDetected_
                    && (txNext_ != txNextAck_ || !sduQueue_.empty()))
                rescheduleAfter(tPollRetransmit_, tPollRetransmitTimer_);
        }
        return;
    }

    cGate *incoming = msg->getArrivalGate();
    if (incoming->isName("in")) {
        auto pkt = check_and_cast<Packet *>(msg);
        emit(receivedPacketFromUpperLayerSignal_, pkt);

        auto pdcpHeader = pkt->peekAtFront<LtePdcpHeader>();
        auto pdcpTag = pkt->addTagIfAbsent<PdcpTrackingTag>();
        pdcpTag->setPdcpSequenceNumber(pdcpHeader->getSequenceNumber());
        pdcpTag->setOriginalPacketLength(pkt->getByteLength());

        enque(pkt);
    }
    else if (incoming->isName("macIn")) {
        auto pkt = check_and_cast<Packet *>(msg);
        auto macSduRequest = pkt->peekAtFront<LteMacSduRequest>();
        sendPdus(macSduRequest->getSduSize());
        delete pkt;
    }
    else if (incoming->isName("feedbackIn")) {
        // STATUS PDU received from the peer, handed over by the co-located RX side
        processControlPacket(check_and_cast<Packet *>(msg));
    }
    else if (incoming->isName("statusIn")) {
        // locally generated STATUS report from the co-located RX side
        bufferControlPduInternal(check_and_cast<Packet *>(msg));
    }
    else {
        throw cRuntimeError("LteRlcAmTxEntity: unexpected message from gate %s", incoming->getFullName());
    }
}

void LteRlcAmTxEntity::enque(Packet *sdu)
{
    Enter_Method("LteRlcAmTxEntity::enque()");
    EV << NOW << " LteRlcAmTxEntity::enque() - inserting new SDU " << sdu << endl;

    delete lteInfo_;
    lteInfo_ = sdu->getTag<FlowControlInfo>()->dup();

    take(sdu);
    sduQueue_.push_back(sdu);
    sduQueueBytes_ += sdu->getByteLength();

    sduSampleBytes_ += sdu->getByteLength();
    emit(enqueuedSduSizeSignal_, sdu->getByteLength());
    if ((NOW - lastSduSample_) >= 1) {
        emit(enqueuedSduRateSignal_, sduSampleBytes_ / (NOW - lastSduSample_));
        sduSampleBytes_ = 0;
        lastSduSample_ = NOW;
    }

    sendNewDataNotification(sdu);
}

void LteRlcAmTxEntity::sendPdus(int pduSize)
{
    Enter_Method("LteRlcAmTxEntity::sendPdus()");
    EV << NOW << " LteRlcAmTxEntity::sendPdus() - PDU with size " << pduSize << " requested from MAC" << endl;
    emit(requestedPduSizeSignal_, pduSize);

    if (radioLinkFailureDetected_) {
        EV << NOW << " LteRlcAmTxEntity::sendPdus() RLF detected, stopping" << endl;
        sendEmptyPdu();
        return;
    }

    // TS 36.322: STATUS PDUs take precedence over data
    if (!controlBuffer_.empty()) {
        auto *pktControl = check_and_cast<inet::Packet *>(controlBuffer_.front());
        controlBuffer_.pop_front();

        EV << NOW << " LteRlcAmTxEntity::sendPdus() - sending STATUS PDU of "
           << pktControl->getByteLength() << " bytes to lower layer" << endl;
        sendPduToMac(pktControl);
        reportBufferStatus();
        return;
    }

    if (sendRetransmission(pduSize)) {
        reportBufferStatus();
        return;
    }

    if (sduQueue_.empty()) {
        EV << NOW << " LteRlcAmTxEntity::sendPdus() buffer empty, wasting grant" << endl;
        emit(wastedGrantedBytesSignal_, std::max(pduSize - (int)RLC_HEADER_AM, 0));
        sendEmptyPdu();
        return;
    }

    if (windowFull()) {
        // No new SN can be assigned until the window advances; the SDUs wait.
        // Buffer status is re-reported when a STATUS advances the window.
        EV << NOW << " LteRlcAmTxEntity::sendPdus() TX window stalled, cannot send new data" << endl;
        emit(txWindowFullSignal_, 1);
        sendEmptyPdu();
        return;
    }

    if (pduSize - (int)RLC_HEADER_AM <= 0) {
        sendEmptyPdu();
        return;
    }

    buildAndSendPdu(pduSize);
}

void LteRlcAmTxEntity::buildAndSendPdu(int pduSize)
{
    // Concatenate queued SDUs / SDU fragments into one AMD PDU filling the grant
    // (TS 36.322 5.1.3.1.1; the walk mirrors LteRlcUmTxEntity::rlcPduMake).
    int budget = pduSize - (int)RLC_HEADER_AM;

    auto rlcPdu = inet::makeShared<LteRlcAmDataPdu>();
    FramingInfo fi;
    fi.firstIsFragment = (frontOffset_ > 0);
    fi.lastIsFragment = false;
    int len = 0;

    while (!sduQueue_.empty() && budget > 0) {
        auto *sdu = sduQueue_.front();
        auto pdcpTag = sdu->getTag<PdcpTrackingTag>();
        int remaining = pdcpTag->getOriginalPacketLength() - frontOffset_;

        if (budget >= remaining) {
            // the rest of this SDU fits: carry it whole and pop the SDU
            sduQueue_.pop_front();
            rlcPdu->pushSdu(sdu, remaining);
            len += remaining;
            budget -= remaining;
            sduQueueBytes_ -= remaining;
            frontOffset_ = 0;
        }
        else {
            // carry the front `budget` bytes; the SDU stays queued with an offset.
            // As everywhere in the model, the fragment chunk is a dup of the whole
            // SDU packet; only the per-chunk size (and chunkLength) reflect the
            // bytes on the air.
            rlcPdu->pushSdu(sdu->dup(), budget);
            len += budget;
            sduQueueBytes_ -= budget;
            frontOffset_ += budget;
            fi.lastIsFragment = true;
            budget = 0;
        }
    }
    ASSERT(len > 0);

    uint32_t sn = txNext_++;
    rlcPdu->setPduSequenceNumber(sn);
    rlcPdu->setFramingInfo(fi);
    rlcPdu->setChunkLength(inet::B(RLC_HEADER_AM + len));

    ++pduWithoutPoll_;
    byteWithoutPoll_ += len;
    rlcPdu->setPollStatus(checkPolling(sn));

    auto pkt = new inet::Packet("lteRlcAmPdu");
    pkt->insertAtFront(rlcPdu);
    if (lteInfo_)
        *(pkt->addTagIfAbsent<FlowControlInfo>()) = *lteInfo_;

    // retain the built PDU: it is the unit of ARQ retransmission
    txWindow_[sn] = TxPdu{pkt->dup(), (uint32_t)len};

    emit(txWindowOccupationSignal_, (long)(txNext_ - txNextAck_));
    sendPduToMac(pkt);
}

bool LteRlcAmTxEntity::checkPolling(uint32_t sn)
{
    // TS 36.322 5.2.2.1: poll when the counters reach pollPDU/pollByte, or when
    // this transmission empties the buffers, or when the window is stalled.
    bool noPendingData = (sduQueue_.empty() && rtxBuffer_->getRetxPendingBytes() == 0);

    if (pollPending_ || pduWithoutPoll_ > pollPdu_ || byteWithoutPoll_ >= pollByte_
            || noPendingData || windowFull()) {
        pduWithoutPoll_ = 0;
        byteWithoutPoll_ = 0;
        pollSn_ = sn;
        rescheduleAfter(tPollRetransmit_, tPollRetransmitTimer_);
        pollPending_ = false;
        return true;
    }
    return false;
}

bool LteRlcAmTxEntity::sendRetransmission(int pduSize)
{
    RetxTask next;
    if (!rtxBuffer_->getNextRetxTask(next))
        return false;

    auto it = txWindow_.find(next.sn);
    if (it == txWindow_.end()) {
        // Unreachable after a radio link failure: sendPdus() gates on the RLF flag
        // before calling here, and the flag only changes in other events. A miss
        // therefore means the retransmission buffer references an SN the TX window
        // no longer holds -- a bookkeeping bug worth failing fast on.
        throw cRuntimeError("LteRlcAmTxEntity::sendRetransmission PDU sn=%u not in the TX window", next.sn);
    }
    uint32_t payloadLength = it->second.payloadLength;

    uint32_t start = next.isWholeUnit ? 0 : next.soStart;
    uint32_t end = next.isWholeUnit ? payloadLength - 1 : next.soEnd;

    if (next.isWholeUnit && (int)payloadLength <= pduSize - (int)RLC_HEADER_AM) {
        // the whole AMD PDU fits the grant: resend it as built (with a fresh poll decision)
        auto pkt = it->second.pdu->dup();
        auto rlcPdu = pkt->removeAtFront<LteRlcAmDataPdu>();
        ++pduWithoutPoll_;
        byteWithoutPoll_ += payloadLength;
        rlcPdu->setPollStatus(checkPolling(next.sn));
        pkt->insertAtFront(rlcPdu);

        sendPduToMac(pkt);
        rtxBuffer_->markRetransmitted(next);
        emit(retransmissionPduSignal_, 1);
        return true;
    }

    // Re-segment (TS 36.322 5.2.1: the retransmission must fit the grant): carry
    // the byte range [start, sendEnd] of the original PDU's data field as an AMD
    // PDU segment. The segment packet carries the original PDU's whole SDU list;
    // chunkLength reflects the bytes on the air (header + SO fields + range).
    int segBudget = pduSize - (int)RLC_HEADER_AM - 2;   // 2 B: SO + LSF fields
    if (segBudget <= 0)
        return false;
    uint32_t sendEnd = std::min(end, start + (uint32_t)segBudget - 1);

    auto pkt = it->second.pdu->dup();
    pkt->setName("lteRlcAmPduSegment");
    auto rlcPdu = pkt->removeAtFront<LteRlcAmDataPdu>();
    rlcPdu->setRf(true);
    rlcPdu->setSoStart(start);
    rlcPdu->setSoEnd(sendEnd);
    rlcPdu->setLsf(sendEnd == payloadLength - 1);
    rlcPdu->setChunkLength(inet::B(RLC_HEADER_AM + 2 + (sendEnd - start + 1)));
    ++pduWithoutPoll_;
    byteWithoutPoll_ += sendEnd - start + 1;
    rlcPdu->setPollStatus(checkPolling(next.sn));
    pkt->insertAtFront(rlcPdu);

    sendPduToMac(pkt);
    rtxBuffer_->markRetransmitted(next);
    if (sendEnd < end) {
        // remainder stays pending; re-adding within the same round does not advance
        // RETX_COUNT (the per-round flag is already set for this SN)
        rtxBuffer_->addNack(next.sn, false, sendEnd + 1, end);
    }
    emit(retransmissionPduSignal_, 1);
    return true;
}

void LteRlcAmTxEntity::handleControlPacket(cPacket *pkt)
{
    Enter_Method("handleControlPacket()");
    take(pkt);
    processControlPacket(check_and_cast<Packet *>(pkt));
}

void LteRlcAmTxEntity::processControlPacket(Packet *pktPdu)
{
    auto pdu = pktPdu->peekAtFront<LteRlcAmStatusPdu>();
    StatusPduData data = pdu->getData();

    bool wasStalled = windowFull();

    rtxBuffer_->beginRetxRound();
    std::set<uint32_t> nacks;
    bool pollSnNacked = false;

    for (const NackInfo& info : data.nacks) {
        unsigned int range = std::max(info.nackRange, 1u);
        for (unsigned int j = 0; j < range; ++j) {
            uint32_t nackedSn = info.sn + j;
            if (nackedSn >= txNextAck_ && nackedSn < txNext_ && txWindow_.count(nackedSn)) {
                if (!rtxBuffer_->addNack(nackedSn, !info.isSegment, info.soStart, info.soEnd)) {
                    declareRadioLinkFailure();
                    delete pktPdu;
                    return;
                }
            }
            nacks.insert(nackedSn);
            if (nackedSn == pollSn_)
                pollSnNacked = true;
        }
    }

    // Cumulative acknowledgement: everything below ACK_SN that is not NACKed is
    // acknowledged -- drop the retained PDU and its retransmission state. VT(A)
    // then advances to the oldest PDU still unacknowledged.
    for (uint32_t sn = txNextAck_; sn < data.ackSn; ++sn) {
        if (nacks.find(sn) == nacks.end()) {
            auto it = txWindow_.find(sn);
            if (it != txWindow_.end()) {
                delete it->second.pdu;
                txWindow_.erase(it);
            }
            rtxBuffer_->clearSn(sn);
        }
    }
    while (txNextAck_ < data.ackSn && txWindow_.find(txNextAck_) == txWindow_.end()
            && nacks.find(txNextAck_) == nacks.end())
        ++txNextAck_;

    // TS 36.322 5.2.2.3: stop t-PollRetransmit when the STATUS acknowledges or
    // NACKs POLL_SN; a NACK immediately re-arms it for the retransmission.
    if (tPollRetransmitTimer_->isScheduled() && (pollSn_ < data.ackSn || pollSnNacked)) {
        cancelEvent(tPollRetransmitTimer_);
        if (pollSnNacked)
            rescheduleAfter(tPollRetransmit_, tPollRetransmitTimer_);
    }

    // Keep the poll machinery alive while anything is still pending. Stopping on
    // the acknowledgement of POLL_SN must not silence the entity for good: a
    // grant issued for an announced SDU may have been spent on a retransmission,
    // and the poll expiry's reportBufferStatus() is then the only thing that
    // re-announces the SDU to the MAC.
    if (!tPollRetransmitTimer_->isScheduled() && !radioLinkFailureDetected_
            && (txNext_ != txNextAck_ || !sduQueue_.empty()
                || rtxBuffer_->getRetxPendingBytes() > 0 || !controlBuffer_.empty()))
        rescheduleAfter(tPollRetransmit_, tPollRetransmitTimer_);

    // A window that just un-stalled has queued SDUs the MAC no longer knows
    // about; re-announce them.
    if (wasStalled && !windowFull() && !sduQueue_.empty())
        reportBufferStatus();

    emit(txWindowOccupationSignal_, (long)(txNext_ - txNextAck_));
    delete pktPdu;
}

void LteRlcAmTxEntity::bufferControlPdu(cPacket *pkt)
{
    Enter_Method("LteRlcAmTxEntity::bufferControlPdu()");
    take(pkt);
    bufferControlPduInternal(check_and_cast<inet::Packet *>(pkt));
}

void LteRlcAmTxEntity::bufferControlPduInternal(inet::Packet *pkt)
{
    controlBuffer_.push_back(pkt);
    sendNewDataNotification(pkt);
}

void LteRlcAmTxEntity::sendEmptyPdu()
{
    // The LTE MAC requested a PDU this entity cannot fill (grant too small,
    // empty buffer, stalled window, an over-announcement consumed by an earlier
    // grant): respond with a header-only padding PDU. It must be a real,
    // bufferizable packet -- the UE-side MAC accounts for every granted SDU and
    // aborts on a missing one -- so unlike the UM entity's 1-bit marker it
    // carries the AM header size and travels to the peer, whose RX discards a
    // PDU with no SDUs. Stamped from flowControlInfo_ -- set at entity
    // installation, so present even on an entity that has only ever carried
    // STATUS traffic (lteInfo_ is per-SDU and may be null).
    auto pkt = new inet::Packet("lteRlcAmPdu (padding)");
    auto rlcPdu = inet::makeShared<LteRlcAmDataPdu>();
    rlcPdu->setChunkLength(inet::B(RLC_HEADER_AM));
    pkt->insertAtFront(rlcPdu);
    FlowControlInfo *info = lteInfo_ ? lteInfo_ : flowControlInfo_;
    if (info == nullptr)
        throw cRuntimeError("LteRlcAmTxEntity::sendEmptyPdu(): no flow control info available");
    *(pkt->addTagIfAbsent<FlowControlInfo>()) = *info;
    sendPduToMac(pkt);
}

void LteRlcAmTxEntity::sendPduToMac(inet::Packet *pkt)
{
    pkt->addTagIfAbsent<inet::PacketProtocolTag>()->setProtocol(&LteProtocol::rlc);
    emit(sentPacketToLowerLayerSignal_, pkt);
    send(pkt, "out");
}

void LteRlcAmTxEntity::sendNewDataNotification(inet::Packet *pkt)
{
    auto newData = new inet::Packet("AM-NewData");
    newData->copyTags(*pkt);
    if (!newData->findTag<PdcpTrackingTag>()) {
        auto t = newData->addTag<PdcpTrackingTag>();
        t->setOriginalPacketLength(pkt->getByteLength());
        t->setPdcpSequenceNumber(0);
    }
    newData->addTag<LteRlcNewDataTag>();
    send(newData, "out");
}

void LteRlcAmTxEntity::reportBufferStatus()
{
    unsigned int pendingData = getPendingDataVolume();
    FlowControlInfo *info = lteInfo_ ? lteInfo_ : flowControlInfo_;
    if (pendingData > 0 && info) {
        auto pkt = new inet::Packet("lteRlcAmPdu Inform MAC");
        auto rlcPdu = inet::makeShared<LteRlcAmDataPdu>();
        rlcPdu->setChunkLength(B(pendingData));
        *(pkt->addTagIfAbsent<FlowControlInfo>()) = *info;
        pkt->insertAtFront(rlcPdu);
        sendNewDataNotification(pkt);
        delete pkt;
    }
}

unsigned int LteRlcAmTxEntity::getPendingDataVolume() const
{
    unsigned int size = sduQueueBytes_;
    for (const auto *cpkt : controlBuffer_)
        size += check_and_cast<const Packet *>(cpkt)->getByteLength();

    // A PDU awaiting retransmission is already formed, so it counts towards the
    // data volume with its header (TS 36.322 5.4) -- and it has to, for the same
    // reason as in NrRlcAmTxEntity: the grant must be able to carry the segment
    // header the retransmission needs, or it can never be sent.
    for (const auto& task : rtxBuffer_->getPendingRetx()) {
        if (task.isWholeUnit) {
            auto it = txWindow_.find(task.sn);
            size += (it != txWindow_.end() ? it->second.payloadLength : 0) + RLC_HEADER_AM;
        }
        else
            size += (task.soEnd - task.soStart + 1) + RLC_HEADER_AM + 2;
    }
    return size;
}

} //namespace
