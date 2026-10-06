//
//                  Simu5G
//
// Authors: Esteban Egea Lopez (Universidad Politecnica de Cartagena), Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include <inet/networklayer/common/NetworkInterface.h>
#include <set>

#include <inet/common/ProtocolTag_m.h>

#include "simu5g/stack/rlc/am/NrRlcAmTxEntity.h"
#include "simu5g/stack/rlc/am/packet/NrRlcAmDataPdu.h"
#include "simu5g/stack/rlc/am/packet/NrRlcAmStatusPdu_m.h"
#include "simu5g/stack/mac/packet/LteMacSduRequest.h"
#include "simu5g/stack/pdcp/packet/LtePdcpPdu_m.h"
#include "simu5g/stack/rlc/packet/LteRlcNewDataTag_m.h"
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"

namespace simu5g {

Define_Module(NrRlcAmTxEntity);

NrRlcAmTxEntity::~NrRlcAmTxEntity()
{
    if (tPollRetransmitTimer_)
        cancelAndDelete(tPollRetransmitTimer_);
    delete txBuffer_;
    delete rtxBuffer_;
    while (!sduBuffer_.empty()) {
        delete sduBuffer_.front();
        sduBuffer_.pop_front();
    }
    while (!controlBuffer_.empty()) {
        delete controlBuffer_.front();
        controlBuffer_.pop_front();
    }
}

void NrRlcAmTxEntity::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        amWindowSize_ = par("AM_Window_Size");
        // The window is 2^(snBits-1), so it must be a power of two.
        if (amWindowSize_ < 1 || (amWindowSize_ & (amWindowSize_ - 1)) != 0)
            throw cRuntimeError("NrRlcAmTxEntity::initialize() AM_Window_Size=%u must be a power of two (e.g. 512, 2048, 131072)", amWindowSize_);
        // SN field length in bits: window = 2^(snBits-1) => snBits = log2(window) + 1.
        snFieldLength_ = 1;
        for (unsigned int w = amWindowSize_; w > 1; w >>= 1)
            ++snFieldLength_;

        pollPdu_ = par("pollPDU");
        pollByte_ = par("pollByte");
        maxRtxThreshold_ = par("maxRtxThreshold");
        tPollRetransmit_ = par("t_PollRetransmit");
        tPollRetransmitTimer_ = new cMessage("t_PollRetransmit timer");

        nameEntity_ = getFullPath();
        txBuffer_ = new RlcSduSlidingWindowTransmissionBuffer(amWindowSize_, nameEntity_ + "-tx-sliding window:");
        rtxBuffer_ = new RlcRetransmissionBuffer(maxRtxThreshold_);
        lastSduSample_ = NOW;
    }
}

void NrRlcAmTxEntity::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (msg == tPollRetransmitTimer_) {
            pollPending_ = true;
            bool noPendingData = (txBuffer_->getTotalPendingBytes() == 0
                    && sduBuffer_.empty() && rtxBuffer_->getRetxPendingBytes() == 0);

            if (noPendingData || txBuffer_->windowFull()) {
                // TS 38.322 5.3.3: the poll went unanswered, so consider an outstanding
                // SDU for retransmission. That is a retransmission like any other, so it
                // opens a new round and advances RETX_COUNT -- which is what lets a link
                // that has gone silent altogether reach maxRtxThreshold and fail, rather
                // than stall here forever.
                rtxBuffer_->beginRetxRound();

                uint32_t hsn = txBuffer_->getHighestSnTransmitted();
                Packet *ptr = nullptr;
                uint32_t totalLength = 0;
                if (txBuffer_->getSduData(hsn, ptr, totalLength)) {
                    if (!rtxBuffer_->addNack(hsn, true, 0, totalLength - 1)) {
                        declareRadioLinkFailure();
                        return;
                    }
                }
                else {
                    // The highest SN transmitted is gone (already acknowledged); fall back
                    // to the oldest SDU still awaiting acknowledgement.
                    uint32_t next = txBuffer_->getTxNextAck();
                    while (txBuffer_->isInRtxRange(next)) {
                        if (!txBuffer_->isFullyAcknowledged(next)
                                && txBuffer_->getSduData(next, ptr, totalLength))
                        {
                            if (!rtxBuffer_->addNack(next, true, 0, totalLength - 1)) {
                                declareRadioLinkFailure();
                                return;
                            }
                            break;
                        }
                        ++next;
                    }
                }
            }

            // Tell MAC there is something to send, whether the poll queued a
            // retransmission just now or an earlier one is still waiting. Buffer status is
            // otherwise only reported from sendPdus(), that is, in response to a grant --
            // so once the upper layer goes idle, a pending retransmission would never be
            // scheduled, no acknowledgement would ever come back, and the transmitter
            // would sit on its unacknowledged SDUs for the rest of the run.
            reportBufferStatus();

            // Keep the poll cycle alive while anything is still unacknowledged. The timer
            // is otherwise only re-armed by checkPolling(), that is, only when a PDU is
            // actually handed to MAC; if the retransmission queued above is not granted --
            // or the poll expiry had nothing to queue -- the timer would never fire again
            // and the entity would sit on its unacknowledged SDUs forever.
            if (!radioLinkFailureDetected_ && txBuffer_->getTxNext() != txBuffer_->getTxNextAck())
                rescheduleAfter(tPollRetransmit_, tPollRetransmitTimer_);
        }
        return;
    }

    cGate *incoming = msg->getArrivalGate();
    if (incoming->isName("in")) {
        auto pkt = check_and_cast<Packet *>(msg);
        emit(receivedPacketFromUpperLayerSignal_, pkt);

        auto pdcpHeader = pkt->peekAtFront<LtePdcpHeader>();
        unsigned int sequenceNumber = pdcpHeader->getSequenceNumber();
        auto pdcpTag = pkt->addTagIfAbsent<PdcpTrackingTag>();
        pdcpTag->setPdcpSequenceNumber(sequenceNumber);
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
        throw cRuntimeError("NrRlcAmTxEntity: unexpected message from gate %s", incoming->getFullName());
    }
}

void NrRlcAmTxEntity::enque(Packet *sdu)
{
    Enter_Method("NrRlcAmTxEntity::enque()");
    EV << NOW << " NrRlcAmTxEntity::enque() - inserting new SDU " << sdu << endl;

    delete lteInfo_;
    lteInfo_ = sdu->getTag<FlowControlInfo>()->dup();

    take(sdu);
    auto *si = new SduInfo();
    si->sdu = sdu;
    sduBuffer_.push_back(si);
    ++receivedSdus_;

    sduSampleBytes_ += sdu->getByteLength();
    emit(enqueuedSduSizeSignal_, sdu->getByteLength());
    if ((NOW - lastSduSample_) >= 1) {
        emit(enqueuedSduRateSignal_, sduSampleBytes_ / (NOW - lastSduSample_));
        sduSampleBytes_ = 0;
        lastSduSample_ = NOW;
    }

    sendNewDataNotificationNr(sdu);
}

void NrRlcAmTxEntity::sendPdus(int pduSize)
{
    Enter_Method("NrRlcAmTxEntity::sendPdusNr()");
    EV << NOW << " NrRlcAmTxEntity::sendPdus() - PDU with size " << pduSize << " requested from MAC" << endl;
    emit(requestedPduSizeSignal_, pduSize);

    if (radioLinkFailureDetected_) {
        EV << NOW << " " << nameEntity_ << " NrRlcAmTxEntity::sendPdus() RLF detected, stopping" << endl;
        return;
    }

    int size = pduSize - RLC_HEADER_AM;

    if (size < 0) {
        auto pkt = new inet::Packet("lteRlcFragment (empty)");
        auto rlcPdu = inet::makeShared<NrRlcAmDataPdu>();
        rlcPdu->setChunkLength(inet::b(1));
        pkt->insertAtFront(rlcPdu);
        if (lteInfo_)
            *(pkt->addTagIfAbsent<FlowControlInfo>()) = *lteInfo_;
        sendPduToMac(pkt);
        return;
    }

    // TS 38.322: prioritize control PDUs
    if (!controlBuffer_.empty()) {
        auto *pktControl = check_and_cast<inet::Packet *>(controlBuffer_.front());
        controlBuffer_.pop_front();

        EV << NOW << " NrRlcAmTxEntity::sendPdus() - sending Control PDU " << pktControl
           << " with size " << pktControl->getByteLength() << " bytes to lower layer" << endl;
        sendPduToMac(pktControl);

        unsigned int pendingData = getPendingDataVolume();
        if (pendingData > 0 && lteInfo_) {
            auto pkt = new inet::Packet("lteRlcFragment -Indicate new data");
            auto rlcPdu = inet::makeShared<NrRlcAmDataPdu>();
            rlcPdu->setChunkLength(B(pendingData));
            *(pkt->addTagIfAbsent<FlowControlInfo>()) = *lteInfo_;
            pkt->insertAtFront(rlcPdu);
            sendNewDataNotificationNr(pkt);
            delete pkt;
        }
        return;
    }

    if (sendRetransmission(pduSize)) {
        reportBufferStatus();
        return;
    }

    // Size the header from the front segment's continuation state before the carve, so it
    // matches what sendSegment will stamp (and what the scheduler reserved). An empty buffer
    // means a fresh SDU will be pulled below (start 0 -> first/complete, 2B).
    uint32_t st;
    unsigned int newHdr = txBuffer_->peekNextSegmentStart(st)
                        ? nrAmHeaderBytes((st > 0) ? NRUM_CONTINUATION : NRUM_FIRST, snFieldLength_)
                        : nrAmHeaderBytes(NRUM_FIRST, snFieldLength_);
    int newDataSize = pduSize - (int)newHdr;

    PendingSegment segment;
    segment.isValid = false;
    if (txBuffer_->getTotalPendingBytes() > 0)
        segment = txBuffer_->getSegmentForGrant(newDataSize);

    if (!segment.isValid) {
        if (sduBuffer_.empty()) {
            EV << NOW << " NrRlcAmTxEntity::sendPdus() buffer empty, wasting grant" << endl;
            emit(wastedGrantedBytesSignal_, size);
            return;
        }

        SduInfo *si = sduBuffer_.front();
        auto *bufferedSdu = check_and_cast<inet::Packet *>(si->sdu);
        auto pdcpTag = bufferedSdu->getTag<PdcpTrackingTag>();
        int sduLength = pdcpTag->getOriginalPacketLength();

        if (txBuffer_->windowFull()) {
            emit(txWindowFullSignal_, 1);
            return;
        }

        txBuffer_->addSdu(sduLength, bufferedSdu);
        sduBuffer_.pop_front();
        si->sdu = nullptr;  // owned by txBuffer_ from now on
        delete si;
        segment = txBuffer_->getSegmentForGrant(newDataSize);
    }

    if (!segment.isValid) {
        EV << NOW << " NrRlcAmTxEntity::sendPdus() no segment fits grant" << endl;
        emit(wastedGrantedBytesSignal_, size);
        return;
    }

    sendSegment(segment);
    emit(txWindowOccupationSignal_, txBuffer_->getTxNext() - txBuffer_->getTxNextAck());
    reportBufferStatus();
}

void NrRlcAmTxEntity::sendSegment(PendingSegment segment)
{
    auto rlcPdu = inet::makeShared<NrRlcAmDataPdu>();

    uint32_t segmentSize = segment.end - segment.start;
    FramingInfo fi;  // 00 = full SDU
    if (segmentSize != segment.totalLength) {
        if (segment.start == 0) {
            fi.lastIsFragment = true;  // 01 = first segment
        }
        else if (segment.end == segment.totalLength - 1) {
            fi.firstIsFragment = true;  // 10 = last segment
        }
        else {
            fi.firstIsFragment = true;
            fi.lastIsFragment = true;  // 11 = middle segment
        }
    }

    auto *bufferedSdu = check_and_cast<inet::Packet *>(segment.ptr);
    auto pdcpTag = bufferedSdu->getTag<PdcpTrackingTag>();
    int sduLength = pdcpTag->getOriginalPacketLength();
    unsigned int pduSequenceNumber = segment.sn;
    sn_ = std::max(sn_, pduSequenceNumber);
    int sduSequenceNumber = pdcpTag->getPdcpSequenceNumber();

    rlcPdu->pushSdu(bufferedSdu->dup(), segmentSize);
    // TS 38.322 6.2.1.4 AM header: a non-first segment (start>0) carries the 16-bit SO (4B);
    // a complete SDU or first segment carries SI+SN only (2B). The carve budget above
    // reserved the same size, so the PDU exactly fills its requested grant slot.
    unsigned int hdr = nrAmHeaderBytes((segment.start > 0) ? NRUM_CONTINUATION : NRUM_FIRST, snFieldLength_);
    rlcPdu->setFramingInfo(fi);
    rlcPdu->setPduSequenceNumber(pduSequenceNumber);
    rlcPdu->setChunkLength(inet::B(hdr + segmentSize));
    rlcPdu->setSnoMainPacket(sduSequenceNumber);
    rlcPdu->setLengthMainPacket(sduLength);
    rlcPdu->setStartOffset(segment.start);
    rlcPdu->setEndOffset(segment.end);

    ++pduWithoutPoll_;
    byteWithoutPoll_ += segmentSize;
    rlcPdu->setPollStatus(checkPolling());

    std::string name = "NR AM RLC Fragment -" + std::to_string(pduSequenceNumber);
    auto pkt = new inet::Packet(name.c_str());
    pkt->insertAtFront(rlcPdu);
    if (lteInfo_)
        *(pkt->addTagIfAbsent<FlowControlInfo>()) = *lteInfo_;

    sendPduToMac(pkt);
}

bool NrRlcAmTxEntity::checkPolling()
{
    bool noPendingData = (txBuffer_->getTotalPendingBytes() == 0
            && sduBuffer_.empty() && rtxBuffer_->getRetxPendingBytes() == 0);

    if (pollPending_ || pduWithoutPoll_ > pollPdu_ || byteWithoutPoll_ >= pollByte_
            || noPendingData || txBuffer_->windowFull()) {
        pduWithoutPoll_ = 0;
        byteWithoutPoll_ = 0;
        pollSn_ = sn_;
        rescheduleAfter(tPollRetransmit_, tPollRetransmitTimer_);
        pollPending_ = false;
        return true;
    }
    return false;
}

void NrRlcAmTxEntity::reportBufferStatus()
{
    unsigned int pendingData = getPendingDataVolume();
    if (pendingData > 0 && lteInfo_) {
        auto pkt = new inet::Packet("lteRlcFragment Inform MAC");
        auto rlcPdu = inet::makeShared<NrRlcAmDataPdu>();
        rlcPdu->setChunkLength(B(pendingData));
        *(pkt->addTagIfAbsent<FlowControlInfo>()) = *lteInfo_;
        pkt->insertAtFront(rlcPdu);
        sendNewDataNotificationNr(pkt);
        delete pkt;
    }
}

bool NrRlcAmTxEntity::sendRetransmission(int pduSize)
{
    RetxTask next;
    if (!rtxBuffer_->getNextRetxTask(next))
        return false;

    uint32_t start = next.soStart;
    uint32_t end = next.soEnd;

    if (next.isWholeUnit) {
        Packet *ptr = nullptr;
        uint32_t totalLength = 0;
        if (txBuffer_->getSduData(next.sn, ptr, totalLength)) {
            start = 0;
            end = totalLength - 1;
        }
        else {
            // Unreachable after a radio link failure: sendPdus() gates on the RLF flag
            // before calling here, and the flag only changes in other events. A miss
            // therefore means the retransmission buffer references an SN the TX window
            // no longer holds -- a bookkeeping bug worth failing fast on.
            throw cRuntimeError("NrRlcAmTxEntity::sendRetransmission whole SDU sn=%u not found", next.sn);
        }
    }

    // TS 38.322 AM header from the finalized segment start; reserve it before re-carving.
    int hdr = (int)nrAmHeaderBytes((start > 0) ? NRUM_CONTINUATION : NRUM_FIRST, snFieldLength_);
    int budget = pduSize - hdr;
    if (budget < 0)
        return false;
    PendingSegment segment = txBuffer_->getRetransmissionSegment(next.sn, start, end, budget);
    if (!segment.isValid)
        throw cRuntimeError("NrRlcAmTxEntity::sendRetransmission SDU sn=%u: invalid segment", next.sn);
    if (!segment.ptr)
        throw cRuntimeError("NrRlcAmTxEntity::sendRetransmission SDU sn=%u: null pointer", next.sn);

    sendSegment(segment);
    rtxBuffer_->markRetransmitted(next);
    emit(retransmissionPduSignal_, 1);
    return true;
}

void NrRlcAmTxEntity::handleControlPacket(cPacket *pkt)
{
    Enter_Method("handleControlPacket()");
    take(pkt);
    processControlPacket(check_and_cast<Packet *>(pkt));
}

void NrRlcAmTxEntity::processControlPacket(Packet *pktPdu)
{
    auto pdu = pktPdu->peekAtFront<NrRlcAmStatusPdu>();
    StatusPduData data = pdu->getData();

    rtxBuffer_->beginRetxRound();
    std::set<uint32_t> nacks;
    bool restartPoll = false;

    for (size_t i = 0; i < data.nacks.size(); ++i) {
        const NackInfo &info = data.nacks[i];
        for (unsigned int j = 0; j < info.nackRange; ++j) {
            uint32_t nackedSn = info.sn + j;
            if (txBuffer_->isInRtxRange(nackedSn)) {
                bool isWhole = !info.isSegment;
                if (!rtxBuffer_->addNack(nackedSn, isWhole, info.soStart, info.soEnd)) {
                    declareRadioLinkFailure();
                    delete pktPdu;
                    return;
                }
            }
            nacks.insert(nackedSn);
            if (nackedSn == pollSn_)
                restartPoll = true;
        }
    }

    uint32_t next = txBuffer_->getTxNextAck();
    while (next < data.ackSn) {
        if (nacks.find(next) == nacks.end()) {
            uint32_t totalLength;
            Packet *sdu = nullptr;
            if (txBuffer_->getSduData(next, sdu, totalLength)) {
                std::set<uint32_t> acked = txBuffer_->handleAck(next, 0, totalLength - 1, pollSn_, restartPoll);
                for (uint32_t ackedSn : acked)
                    rtxBuffer_->clearSn(ackedSn);
            }
        }
        ++next;
    }

    if (tPollRetransmitTimer_->isScheduled() && restartPoll)
        rescheduleAfter(tPollRetransmit_, tPollRetransmitTimer_);

    emit(txWindowOccupationSignal_, txBuffer_->getCurrentWindowSize());
    delete pktPdu;
}

void NrRlcAmTxEntity::bufferControlPdu(cPacket *pkt)
{
    Enter_Method("NrRlcAmTxEntity::bufferControlPdu()");
    take(pkt);
    bufferControlPduInternal(check_and_cast<inet::Packet *>(pkt));
}

void NrRlcAmTxEntity::bufferControlPduInternal(inet::Packet *pkt)
{
    controlBuffer_.push_back(pkt);
    sendNewDataNotificationNr(pkt);
}

void NrRlcAmTxEntity::sendPduToMac(inet::Packet *pkt)
{
    pkt->addTagIfAbsent<inet::PacketProtocolTag>()->setProtocol(&LteProtocol::rlc);
    emit(sentPacketToLowerLayerSignal_, pkt);
    send(pkt, "out");
}

void NrRlcAmTxEntity::sendNewDataNotificationNr(inet::Packet *pkt)
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

unsigned int NrRlcAmTxEntity::getPendingDataVolume() const
{
    unsigned int size = 0;
    for (const auto *si : sduBuffer_) {
        auto *pkt = check_and_cast<inet::Packet *>(si->sdu);
        auto pdcpTag = pkt->getTag<PdcpTrackingTag>();
        size += pdcpTag->getOriginalPacketLength();
    }
    size += txBuffer_->getTotalPendingBytes();

    // A PDU awaiting retransmission is already formed, so it counts towards the data
    // volume with its header, unlike an SDU that has not been put into a PDU yet
    // (TS 38.322 5.5). It also has to: the scheduler sizes the RLC header from its own
    // view of how the flow is being segmented, which knows nothing about ARQ, so it
    // assumes a first segment. Reporting only the payload of an interior segment then
    // yields a grant too small to hold that segment's larger continuation header, and
    // the retransmission can never be sent at all.
    for (const auto& task : rtxBuffer_->getPendingRetx()) {
        size += task.soEnd - task.soStart + 1;
        size += nrAmHeaderBytes(task.soStart > 0 ? NRUM_CONTINUATION : NRUM_FIRST, snFieldLength_);
    }
    for (const auto *cpkt : controlBuffer_) {
        auto *p = check_and_cast<const Packet *>(cpkt);
        auto statusPdu = p->peekAtFront<NrRlcAmStatusPdu>();
        size += statusPdu->getChunkLength().get();
    }
    return size;
}

} //namespace
