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

#include "simu5g/stack/rlc/am/NrRlcAmRxEntity.h"
#include "simu5g/stack/rlc/am/RlcAmTxEntityBase.h"
#include "simu5g/stack/rlc/am/packet/NrRlcAmDataPdu.h"
#include "simu5g/stack/rlc/am/packet/NrRlcAmStatusPdu_m.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/stack/rrc/BearerManagement.h"
#include "simu5g/stack/rlc/RlcMux.h"
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"

namespace simu5g {

Define_Module(NrRlcAmRxEntity);

using namespace inet;

NrRlcAmRxEntity::~NrRlcAmRxEntity()
{
    delete rxBuffer_;
    if (tReassemblyTimer_)
        cancelAndDelete(tReassemblyTimer_);
    if (tStatusProhibitTimer_)
        cancelAndDelete(tStatusProhibitTimer_);
}

void NrRlcAmRxEntity::initMode()
{
    nameEntity_ = getFullPath();

    amWindowSize_ = par("AM_Window_Size");
    if (amWindowSize_ < 1 || (amWindowSize_ & (amWindowSize_ - 1)) != 0)
        throw cRuntimeError("NrRlcAmRxEntity::initialize() AM_Window_Size=%u must be a power of two (e.g. 512, 2048, 131072)", amWindowSize_);

    rxBuffer_ = new RlcSduSlidingWindowReceptionBuffer(amWindowSize_, nameEntity_ + "-rx-sliding window:");
    tReassemblyTimer_ = new cMessage("t_ReassemblyTimer");
    tReassembly_ = par("t_Reassembly");
    tStatusProhibitTimer_ = new cMessage("t_StatusProhibitTimer");
    tStatusProhibit_ = par("t_StatusProhibit");

    WATCH(passedUpSdus_);
    WATCH(rxNextStatusTrigger_);
    WATCH(statusReportPending_);
}

void NrRlcAmRxEntity::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (msg == tReassemblyTimer_) {
            rxBuffer_->handleReassemblyTimerExpiry(rxNextStatusTrigger_);
            bool hasHoles = rxBuffer_->hasMissingByteSegmentBeforeLast(rxBuffer_->getRxHighestStatus());
            bool restart = (rxBuffer_->getRxNextHighest() == rxBuffer_->getRxHighestStatus() && hasHoles);

            if (rxBuffer_->getRxNextHighest() > rxBuffer_->getRxHighestStatus() + 1 || restart) {
                scheduleAfter(tReassembly_, tReassemblyTimer_);
                rxNextStatusTrigger_ = rxBuffer_->getRxNextHighest();
            }
            sendStatusReportNr();
        }
        else if (msg == tStatusProhibitTimer_) {
            if (statusReportPending_)
                sendStatusReportNr();
        }
        return;
    }

    auto pkt = check_and_cast<Packet *>(msg);
    auto chunk = pkt->peekAtFront<inet::Chunk>();
    if (inet::dynamicPtrCast<const NrRlcAmStatusPdu>(chunk) != nullptr) {
        routeControlToTxEntityNr(pkt);
    }
    else {
        emit(receivedPacketFromLowerLayerSignal_, pkt);
        enque(pkt);
    }
}

void NrRlcAmRxEntity::enque(Packet *pkt)
{
    Enter_Method("enque()");
    take(pkt);

    auto pdu = pkt->peekAtFront<NrRlcAmDataPdu>();

    if (ackFlowControlInfo_ == nullptr) {
        auto orig = pkt->getTag<FlowControlInfo>();
        ackFlowControlInfo_ = orig->dup();
        ackFlowControlInfo_->setSourceId(orig->getDestId());
        ackFlowControlInfo_->setDestId(orig->getSourceId());
        ackFlowControlInfo_->setDirection((orig->getDirection() == DL) ? UL : DL);
    }

    // Per-PDU delay and throughput, as seen on the air interface (before reassembly).
    // Counted here, before the in-window test, so that duplicates and out-of-window
    // arrivals -- which the air interface carried all the same -- are included.
    totalPduRcvdBytes_ += pkt->getByteLength();
    emitRxStatistics(true, (double)totalPduRcvdBytes_ / (NOW - getSimulation()->getWarmupPeriod()),
            NOW - pkt->getCreationTime());

    unsigned int sn = pdu->getPduSequenceNumber();

    if (!rxBuffer_->inWindow(sn)) {
        if (pdu->getPollStatus())
            sendStatusReportNr();
        delete pkt;
        return;
    }

    if (rxBuffer_->isReady(sn)) {
        if (pdu->getPollStatus())
            sendStatusReportNr();
        delete pkt;
        return;
    }

    int totalSduLength = pdu->getLengthMainPacket();
    int start = pdu->getStartOffset();
    int end = pdu->getEndOffset();

    auto segmentResult = rxBuffer_->handleSegment(sn, totalSduLength, start, end, pkt);
    if (segmentResult.second) {
        if (pdu->getPollStatus())
            sendStatusReportNr();
        delete pkt;
        return;
    }

    if (segmentResult.first) {
        passUpNr(sn);
        if (sn == rxBuffer_->getRxHighestStatus())
            rxBuffer_->updateRxHighestStatus();
        if (sn == rxBuffer_->getRxNext())
            rxBuffer_->updateRxNext();
    }

    if (pdu->getPollStatus()) {
        if (sn < rxBuffer_->getRxHighestStatus() || rxBuffer_->aboveWindow(sn))
            sendStatusReportNr();
    }

    unsigned int currentRxNext = rxBuffer_->getRxNext();
    bool hasHoles = rxBuffer_->hasMissingByteSegmentBeforeLast(currentRxNext);

    if (tReassemblyTimer_->isScheduled()) {
        bool noHolesAndStatus = (currentRxNext + 1 == rxNextStatusTrigger_ && !hasHoles);
        bool statusOff = (!rxBuffer_->inWindow(rxNextStatusTrigger_)
                && rxNextStatusTrigger_ != rxBuffer_->getRxNext() + amWindowSize_);

        if (currentRxNext == rxNextStatusTrigger_ || noHolesAndStatus || statusOff)
            cancelEvent(tReassemblyTimer_);
    }

    if (!tReassemblyTimer_->isScheduled()) {
        bool missingAndHole = (rxBuffer_->getRxNextHighest() == currentRxNext + 1 && hasHoles);
        if (rxBuffer_->getRxNextHighest() > currentRxNext + 1 || missingAndHole) {
            EV << NOW << " NrRlcAmRxEntity::enque() t_ReassemblyTimer scheduled" << endl;
            scheduleAfter(tReassembly_, tReassemblyTimer_);
            rxNextStatusTrigger_ = rxBuffer_->getRxNextHighest();
        }
    }
}

void NrRlcAmRxEntity::passUpNr(int seqNum)
{
    Enter_Method("passUp");

    Packet *bufferedPkt = rxBuffer_->consumeSdu(seqNum);
    if (!bufferedPkt)
        throw cRuntimeError("NrRlcAmRxEntity::passUp() null PDU for seqNum=%d", seqNum);

    auto pdu = bufferedPkt->removeAtFront<NrRlcAmDataPdu>();
    if (pdu->getNumSdu() < 1)
        throw cRuntimeError("NrRlcAmRxEntity::passUp() PDU has no SDU");

    size_t sduLengthPktLen;
    auto sdu = pdu->popSdu(sduLengthPktLen);
    auto sduPdcp = sdu->getTag<PdcpTrackingTag>();
    EV << NOW << " NrRlcAmRxEntity::passUp() SDU[" << sduPdcp->getPdcpSequenceNumber()
       << "] from PDU sn=" << seqNum << endl;

    Direction dir = (ackFlowControlInfo_->getDirection() == DL) ? UL : DL;
    MacNodeId srcId = ackFlowControlInfo_->getDestId();
    MacNodeId dstId = ackFlowControlInfo_->getSourceId();

    auto ci = sdu->addTagIfAbsent<FlowControlInfo>();
    ci->setSourceId(srcId);
    ci->setDestId(dstId);
    ci->setDirection(dir);
    ci->setDrbId(ackFlowControlInfo_->getDrbId());
    sdu->removeTagIfPresent<PdcpTrackingTag>();

    // This bearer's delay and throughput of the reassembled SDU
    totalRcvdBytes_ += sdu->getByteLength();
    double tput = (double)totalRcvdBytes_ / (NOW - getSimulation()->getWarmupPeriod());
    emitRxStatistics(false, tput, NOW - sdu->getCreationTime());

    // How far the reordering window is stretched: the span between the next SDU
    // awaited in sequence and the highest one received, i.e. how much is being held
    // back waiting for a gap to be filled. The TX side reports the mirror quantity.
    emit(rxWindowOccupationSignal_, (long)(rxBuffer_->getRxNextHighest() - rxBuffer_->getRxNext()));

    emit(sentPacketToUpperLayerSignal_, sdu);
    send(sdu, "out");
    passedUpSdus_.insert(sduPdcp->getPdcpSequenceNumber());
    delete bufferedPkt;
}

void NrRlcAmRxEntity::sendStatusReportNr()
{
    Enter_Method("sendStatusReport()");

    if (tStatusProhibitTimer_->isScheduled()) {
        EV << NOW << " NrRlcAmRxEntity::sendStatusReport, minimum interval not reached "
           << tStatusProhibit_ << endl;
        statusReportPending_ = true;
        return;
    }

    StatusPduData data = rxBuffer_->generateStatusPduData();

    auto pktPdu = new Packet("NR STATUS AM PDU");
    auto pdu = makeShared<NrRlcAmStatusPdu>();
    pdu->setAmType(ACK);
    pdu->setData(data);

    unsigned int size = 3;
    for (const auto &nack : data.nacks) {
        size++;
        if (nack.isSegment)
            size += 4;
        else if (nack.nackRange > 1)
            size++;
    }
    pdu->setChunkLength(B(size));

    *pktPdu->addTagIfAbsent<FlowControlInfo>() = *ackFlowControlInfo_;
    pktPdu->insertAtFront(pdu);

    EV << NOW << " NrRlcAmRxEntity::sendStatusReport. Last sent " << lastSentAck_ << endl;
    bufferControlViaTxEntityNr(pktPdu);
    lastSentAck_ = NOW;
    scheduleAfter(tStatusProhibit_, tStatusProhibitTimer_);
    statusReportPending_ = false;
}

void NrRlcAmRxEntity::routeControlToTxEntityNr(Packet *pkt)
{
    // Received STATUS PDU: hand it to the co-located TX side of this AM entity
    // (feedbackOut is connected to tx.feedbackIn inside the RlcAmEntityBase compound). The TX
    // side always exists: bearers are established duplex.
    send(pkt, "feedbackOut");
}

void NrRlcAmRxEntity::bufferControlViaTxEntityNr(Packet *pkt)
{
    // Locally generated STATUS report: hand it to the co-located TX side for
    // transmission on this bearer's logical channel (statusOut -> tx.statusIn).
    send(pkt, "statusOut");
}

} //namespace
