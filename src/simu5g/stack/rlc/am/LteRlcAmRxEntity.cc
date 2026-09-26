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

#include "simu5g/stack/rlc/am/LteRlcAmRxEntity.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/stack/rlc/packet/LteRlcPdu_m.h"
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"

namespace simu5g {

Define_Module(LteRlcAmRxEntity);

using namespace inet;

LteRlcAmRxEntity::~LteRlcAmRxEntity()
{
    delete rxBuffer_;
    if (tReorderingTimer_)
        cancelAndDelete(tReorderingTimer_);
    if (tStatusProhibitTimer_)
        cancelAndDelete(tStatusProhibitTimer_);
    delete pendingSdu_.pkt;
}

void LteRlcAmRxEntity::initMode()
{
    amWindowSize_ = par("AM_Window_Size");
    rxBuffer_ = new RlcSduSlidingWindowReceptionBuffer(amWindowSize_, getFullPath() + "-rx-sliding window:");
    tReorderingTimer_ = new cMessage("t_ReorderingTimer");
    tReordering_ = par("t_Reordering");
    tStatusProhibitTimer_ = new cMessage("t_StatusProhibitTimer");
    tStatusProhibit_ = par("t_StatusProhibit");

    WATCH(rxNextStatusTrigger_);
    WATCH(statusReportPending_);
    WATCH(pendingSdu_);
}

void LteRlcAmRxEntity::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        // The reordering/status logic mirrors NrRlcAmRxEntity (t-Reordering here
        // plays the role its t-Reassembly plays there).
        if (msg == tReorderingTimer_) {
            rxBuffer_->handleReassemblyTimerExpiry(rxNextStatusTrigger_);
            bool hasHoles = rxBuffer_->hasMissingByteSegmentBeforeLast(rxBuffer_->getRxHighestStatus());
            bool restart = (rxBuffer_->getRxNextHighest() == rxBuffer_->getRxHighestStatus() && hasHoles);

            if (rxBuffer_->getRxNextHighest() > rxBuffer_->getRxHighestStatus() + 1 || restart) {
                scheduleAfter(tReordering_, tReorderingTimer_);
                rxNextStatusTrigger_ = rxBuffer_->getRxNextHighest();
            }
            sendStatusReport();
        }
        else if (msg == tStatusProhibitTimer_) {
            if (statusReportPending_)
                sendStatusReport();
        }
        return;
    }

    auto pkt = check_and_cast<Packet *>(msg);
    auto chunk = pkt->peekAtFront<inet::Chunk>();
    if (inet::dynamicPtrCast<const LteRlcAmStatusPdu>(chunk) != nullptr) {
        // Received STATUS PDU: hand it to the co-located TX side of this AM entity
        // (feedbackOut is connected to tx.feedbackIn inside the RlcAmEntityBase compound).
        send(pkt, "feedbackOut");
    }
    else {
        emit(receivedPacketFromLowerLayerSignal_, pkt);
        enque(pkt);
    }
}

void LteRlcAmRxEntity::enque(Packet *pkt)
{
    Enter_Method("enque()");
    take(pkt);

    auto pdu = pkt->peekAtFront<LteRlcAmDataPdu>();

    if (ackFlowControlInfo_ == nullptr) {
        auto orig = pkt->getTag<FlowControlInfo>();
        ackFlowControlInfo_ = orig->dup();
        ackFlowControlInfo_->setSourceId(orig->getDestId());
        ackFlowControlInfo_->setDestId(orig->getSourceId());
        ackFlowControlInfo_->setDirection((orig->getDirection() == DL) ? UL : DL);
    }

    // Per-PDU delay and throughput, as seen on the air interface (before
    // reassembly); counted before the in-window test, so that duplicates and
    // out-of-window arrivals are included.
    totalPduRcvdBytes_ += pkt->getByteLength();
    emitRxStatistics(true, (double)totalPduRcvdBytes_ / (NOW - getSimulation()->getWarmupPeriod()),
            NOW - pkt->getCreationTime());

    uint32_t sn = pdu->getPduSequenceNumber();

    if (pdu->getNumSdu() < 1) {
        // header-only padding PDU: the TX side answered a grant it could not
        // fill (see LteRlcAmTxEntity::sendEmptyPdu()); it carries nothing
        EV << NOW << " LteRlcAmRxEntity::enque() - discarding padding PDU" << endl;
        delete pkt;
        return;
    }

    if (!rxBuffer_->inWindow(sn)) {
        if (pdu->getPollStatus())
            sendStatusReport();
        delete pkt;
        return;
    }

    if (rxBuffer_->isReady(sn)) {
        if (pdu->getPollStatus())
            sendStatusReport();
        delete pkt;
        return;
    }

    // data-field length of the original AMD PDU; a segment's packet carries the
    // whole SDU list, so this is computable from any (re)transmission of the PDU
    uint32_t totalLength = 0;
    for (size_t i = 0; i < pdu->getNumSdu(); ++i)
        totalLength += pdu->getSduSize(i);

    uint32_t start = pdu->getRf() ? pdu->getSoStart() : 0;
    uint32_t end = pdu->getRf() ? pdu->getSoEnd() : totalLength - 1;
    bool polled = pdu->getPollStatus();

    auto segmentResult = rxBuffer_->handleSegment(sn, totalLength, start, end, pkt);
    if (segmentResult.second) {
        // duplicate byte range -- nothing new
        if (polled)
            sendStatusReport();
        delete pkt;
        return;
    }

    if (segmentResult.first) {
        if (sn == rxBuffer_->getRxHighestStatus())
            rxBuffer_->updateRxHighestStatus();

        // In-order delivery: unlike NR (one SDU per PDU-SN, delivered as each
        // completes), the FI walk needs the PDU stream in sequence -- an SDU may
        // span adjacent PDUs. Deliver the contiguous complete prefix, then let
        // the window slide over it.
        uint32_t next = rxBuffer_->getRxNext();
        while (rxBuffer_->isReady(next)) {
            passUpPdu(next);
            ++next;
        }
        rxBuffer_->updateRxNext();
    }

    if (polled) {
        if (sn < rxBuffer_->getRxHighestStatus() || rxBuffer_->aboveWindow(sn))
            sendStatusReport();
    }

    unsigned int currentRxNext = rxBuffer_->getRxNext();
    bool hasHoles = rxBuffer_->hasMissingByteSegmentBeforeLast(currentRxNext);

    if (tReorderingTimer_->isScheduled()) {
        bool noHolesAndStatus = (currentRxNext + 1 == rxNextStatusTrigger_ && !hasHoles);
        bool statusOff = (!rxBuffer_->inWindow(rxNextStatusTrigger_)
                && rxNextStatusTrigger_ != rxBuffer_->getRxNext() + amWindowSize_);

        if (currentRxNext == rxNextStatusTrigger_ || noHolesAndStatus || statusOff)
            cancelEvent(tReorderingTimer_);
    }

    if (!tReorderingTimer_->isScheduled()) {
        bool missingAndHole = (rxBuffer_->getRxNextHighest() == currentRxNext + 1 && hasHoles);
        if (rxBuffer_->getRxNextHighest() > currentRxNext + 1 || missingAndHole) {
            EV << NOW << " LteRlcAmRxEntity::enque() t_Reordering scheduled" << endl;
            scheduleAfter(tReordering_, tReorderingTimer_);
            rxNextStatusTrigger_ = rxBuffer_->getRxNextHighest();
        }
    }
}

void LteRlcAmRxEntity::passUpPdu(uint32_t sn)
{
    Packet *bufferedPkt = rxBuffer_->consumeSdu(sn);
    if (!bufferedPkt)
        throw cRuntimeError("LteRlcAmRxEntity::passUpPdu() null packet for sn=%u", sn);

    auto pdu = bufferedPkt->removeAtFront<LteRlcAmDataPdu>();
    FramingInfo fi = pdu->getFramingInfo();
    size_t numSdu = pdu->getNumSdu();

    EV << NOW << " LteRlcAmRxEntity::passUpPdu() consuming AMD PDU sn=" << sn
       << " with " << numSdu << " SDU chunks, FI=" << fi.toValue() << endl;

    for (size_t i = 0; i < numSdu; ++i) {
        size_t chunkSize;
        auto *chunkPkt = check_and_cast<Packet *>(pdu->popSdu(chunkSize));
        auto chunkTag = chunkPkt->getTag<PdcpTrackingTag>();
        size_t sduLength = chunkTag->getOriginalPacketLength();
        bool startsMidSdu = (i == 0) && fi.firstIsFragment;
        bool endsMidSdu = (i == numSdu - 1) && fi.lastIsFragment;

        if (startsMidSdu) {
            // continuation of the SDU cut at the previous PDU boundary
            if (pendingSdu_.pkt == nullptr)
                throw cRuntimeError("LteRlcAmRxEntity::passUpPdu() sn=%u continues an SDU, but none is pending", sn);
            if (pendingSdu_.pkt->getTag<PdcpTrackingTag>()->getPdcpSequenceNumber()
                    != chunkTag->getPdcpSequenceNumber())
                throw cRuntimeError("LteRlcAmRxEntity::passUpPdu() sn=%u continuation does not match the pending SDU", sn);

            pendingSdu_.accumulated += chunkSize;
            delete pendingSdu_.pkt;    // keep the newest whole-SDU dup
            pendingSdu_.pkt = chunkPkt;

            if (pendingSdu_.accumulated == sduLength) {
                deliverSdu(pendingSdu_.pkt);
                pendingSdu_ = PendingSdu{};
            }
            else if (!endsMidSdu)
                throw cRuntimeError("LteRlcAmRxEntity::passUpPdu() sn=%u SDU ended with %u of %u bytes",
                        sn, (unsigned)pendingSdu_.accumulated, (unsigned)sduLength);
        }
        else if (endsMidSdu) {
            // this PDU ends mid-SDU: the chunk starts a new pending SDU
            if (pendingSdu_.pkt != nullptr)
                throw cRuntimeError("LteRlcAmRxEntity::passUpPdu() sn=%u starts an SDU while another is pending", sn);
            pendingSdu_.pkt = chunkPkt;
            pendingSdu_.accumulated = chunkSize;
        }
        else {
            // a whole SDU carried within this PDU
            if (chunkSize != sduLength)
                throw cRuntimeError("LteRlcAmRxEntity::passUpPdu() sn=%u whole-SDU chunk of %u bytes, SDU is %u",
                        sn, (unsigned)chunkSize, (unsigned)sduLength);
            deliverSdu(chunkPkt);
        }
    }

    delete bufferedPkt;
}

void LteRlcAmRxEntity::deliverSdu(Packet *sdu)
{
    Direction dir = (ackFlowControlInfo_->getDirection() == DL) ? UL : DL;

    auto ci = sdu->addTagIfAbsent<FlowControlInfo>();
    ci->setSourceId(ackFlowControlInfo_->getDestId());
    ci->setDestId(ackFlowControlInfo_->getSourceId());
    ci->setDirection(dir);
    ci->setDrbId(ackFlowControlInfo_->getDrbId());
    sdu->removeTagIfPresent<PdcpTrackingTag>();

    totalRcvdBytes_ += sdu->getByteLength();
    double tput = (double)totalRcvdBytes_ / (NOW - getSimulation()->getWarmupPeriod());
    emitRxStatistics(false, tput, NOW - sdu->getCreationTime());

    // How far the receiving window is stretched: the span between the next PDU
    // awaited in sequence and the highest one received.
    emit(rxWindowOccupationSignal_, (long)(rxBuffer_->getRxNextHighest() - rxBuffer_->getRxNext()));

    emit(sentPacketToUpperLayerSignal_, sdu);
    send(sdu, "out");
}

void LteRlcAmRxEntity::sendStatusReport()
{
    Enter_Method("sendStatusReport()");

    if (tStatusProhibitTimer_->isScheduled()) {
        statusReportPending_ = true;
        return;
    }

    StatusPduData data = rxBuffer_->generateStatusPduData();

    auto pktPdu = new Packet("lteRlcAmStatusPdu");
    auto pdu = makeShared<LteRlcAmStatusPdu>();
    pdu->setData(data);

    // approximate TS 36.322 6.2.1.6 encoding: 2 B fixed part, ~2 B per NACK_SN,
    // 4 B more for the SOstart/SOend pair of a segment NACK
    unsigned int size = 2;
    for (const auto& nack : data.nacks)
        size += 2 * std::max(nack.nackRange, 1u) + (nack.isSegment ? 4 : 0);
    pdu->setChunkLength(B(size));

    *pktPdu->addTagIfAbsent<FlowControlInfo>() = *ackFlowControlInfo_;
    pktPdu->insertAtFront(pdu);

    // hand the report to the co-located TX side for transmission on this
    // bearer's logical channel (statusOut -> tx.statusIn)
    send(pktPdu, "statusOut");
    lastSentAck_ = NOW;
    scheduleAfter(tStatusProhibit_, tStatusProhibitTimer_);
    statusReportPending_ = false;
}

} //namespace
