//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "NrAmRxQueue.h"
#include "simu5g/stack/rlc/am/NrRlcAm.h"
#include "simu5g/stack/rlc/am/packet/NrRlcAmDataPdu.h"
#include "simu5g/stack/rlc/am/packet/NrRlcAmStatusPdu_m.h"
#include "simu5g/stack/rlc/LteRlcDefs.h"
#include "simu5g/stack/mac/LteMacBase.h"

namespace simu5g {

using namespace omnetpp;
using namespace inet;

Define_Module(NrAmRxQueue);

unsigned int NrAmRxQueue::totalCellRcvdBytes_ = 0;

simsignal_t NrAmRxQueue::rlcCellPacketLossSignal_[2] = { registerSignal("rlcCellPacketLossDl"), registerSignal("rlcCellPacketLossUl") };
simsignal_t NrAmRxQueue::rlcPacketLossSignal_[2] = { registerSignal("rlcPacketLossDl"), registerSignal("rlcPacketLossUl") };
simsignal_t NrAmRxQueue::rlcPduPacketLossSignal_[2] = { registerSignal("rlcPduPacketLossDl"), registerSignal("rlcPduPacketLossUl") };
simsignal_t NrAmRxQueue::rlcDelaySignal_[2] = { registerSignal("rlcDelayDl"), registerSignal("rlcDelayUl") };
simsignal_t NrAmRxQueue::rlcThroughputSignal_[2] = { registerSignal("rlcThroughputDl"), registerSignal("rlcThroughputUl") };
simsignal_t NrAmRxQueue::rlcPduDelaySignal_[2] = { registerSignal("rlcPduDelayDl"), registerSignal("rlcPduDelayUl") };
simsignal_t NrAmRxQueue::rlcPduThroughputSignal_[2] = { registerSignal("rlcPduThroughputDl"), registerSignal("rlcPduThroughputUl") };
simsignal_t NrAmRxQueue::rlcCellThroughputSignal_[2] = { registerSignal("rlcCellThroughputDl"), registerSignal("rlcCellThroughputUl") };

simsignal_t NrAmRxQueue::rlcThroughputSampleSignal_[2] = { registerSignal("rlcThroughputSampleDl"), registerSignal("rlcThroughputSampleUl") };
simsignal_t NrAmRxQueue::rlcCellThroughputSampleSignal_[2] = { registerSignal("rlcCellThroughputSampleDl"), registerSignal("rlcCellThroughputSampleUl") };
simsignal_t NrAmRxQueue::rxWindowOccupationSignal_ = registerSignal("rxWindowOccupation");
simsignal_t NrAmRxQueue::rxWindowFullSignal_ = registerSignal("rxWindowFull");


NrAmRxQueue::~NrAmRxQueue()
{
    delete rxBuffer_;
    cancelAndDelete(tReassemblyTimer_);
    cancelAndDelete(tStatusProhibitTimer_);
    cancelAndDelete(throughputTimer_);
}

void NrAmRxQueue::initialize()
{
    binder_.reference(this, "binderModule", true);
    lteRlc_.reference(this, "amModule", true);

    LteMacBase *mac = inet::getConnectedModule<LteMacBase>(getParentModule()->gate("RLC_to_MAC"), 0);
    lastTputSample_ = NOW;
    lastCellTputSample_ = NOW;

    if (mac->getNodeType() == ENODEB || mac->getNodeType() == GNODEB) {
        dir_ = UL;
        nameEntity_ = "GNB-" + std::to_string(lteRlc_->getId());
    }
    else {
        dir_ = DL;
        nameEntity_ = "UE-" + std::to_string(lteRlc_->getId());
    }

    amWindowSize_ = par("AM_Window_Size");
    if (amWindowSize_ != 2048 && amWindowSize_ != 131072)
        throw cRuntimeError("NrAmRxQueue::initialize() AM_Window_Size=%u, only 2048 or 131072 are valid", amWindowSize_);

    rxBuffer_ = new RlcSduSlidingWindowReceptionBuffer(amWindowSize_, nameEntity_ + "-rx-sliding window:");
    tReassemblyTimer_ = new cMessage("t_ReassemblyTimer");
    tReassembly_ = par("t_Reassembly");
    tStatusProhibitTimer_ = new cMessage("t_StatusProhibitTimer");
    tStatusProhibit_ = par("t_StatusProhibit");
    EV << "t_StatusProhibit=" << tStatusProhibit_ << endl;

    throughputTimer_ = new cMessage("throughputTimer");
    throughputInterval_ = par("throughputInterval");
    scheduleAfter(throughputInterval_, throughputTimer_);
}



void NrAmRxQueue::enque(Packet *pkt)
{
    Enter_Method("enque()");
    take(pkt);

    auto pdu = pkt->peekAtFront<NrRlcAmDataPdu>();

    // Extract FlowControlInfo on first PDU (swap src/dst for status reports)
    if (flowControlInfo_ == nullptr) {
        auto orig = pkt->getTag<FlowControlInfo>();
        flowControlInfo_ = orig->dup();
        flowControlInfo_->setSourceId(orig->getDestId());
        flowControlInfo_->setSrcAddr(orig->getDstAddr());
        flowControlInfo_->setTypeOfService(orig->getTypeOfService());
        flowControlInfo_->setDestId(orig->getSourceId());
        flowControlInfo_->setDstAddr(orig->getSrcAddr());
        flowControlInfo_->setDirection((orig->getDirection() == DL) ? UL : DL);
    }

    unsigned int sn = pdu->getPduSequenceNumber();

    // TS 38.322 5.2.3.2: discard PDUs outside the RX window
    if (!rxBuffer_->inWindow(sn)) {
        if (pdu->getPollStatus())
            sendStatusReport();
        delete pkt;
        return;
    }

    // Discard duplicate (already completed) PDUs
    if (rxBuffer_->isReady(sn)) {
        if (pdu->getPollStatus())
            sendStatusReport();
        delete pkt;
        return;
    }

    int totalSduLength = pdu->getLengthMainPacket();
    int start = pdu->getStartOffset();
    int end = pdu->getEndOffset();

    auto segmentResult = rxBuffer_->handleSegment(sn, totalSduLength, start, end, pkt);
    if (segmentResult.second) {
        // Duplicate segment — discard
        if (pdu->getPollStatus())
            sendStatusReport();
        delete pkt;
        return;
    }

    if (segmentResult.first) {
        // SDU fully reassembled
        passUp(sn);
        if (sn == rxBuffer_->getRxHighestStatus())
            rxBuffer_->updateRxHighestStatus();
        if (sn == rxBuffer_->getRxNext())
            rxBuffer_->updateRxNext();
    }

    // TS 38.322 5.3.4: polling
    if (pdu->getPollStatus()) {
        if (sn < rxBuffer_->getRxHighestStatus() || rxBuffer_->aboveWindow(sn))
            sendStatusReport();
    }

    // TS 38.322 5.2.3.2.3: reassembly timer management
    unsigned int currentRxNext = rxBuffer_->getRxNext();
    bool hasHoles = rxBuffer_->hasMissingByteSegmentBeforeLast(currentRxNext);

    if (tReassemblyTimer_->isScheduled()) {
        bool noHolesAndStatus = (currentRxNext + 1 == rxNextStatusTrigger_ && !hasHoles);
        bool statusOff = (!rxBuffer_->inWindow(rxNextStatusTrigger_)
                && rxNextStatusTrigger_ != rxBuffer_->getRxNext() + amWindowSize_);

        if (currentRxNext == rxNextStatusTrigger_ || noHolesAndStatus || statusOff)
            cancelEvent(tReassemblyTimer_);
    }

    // Not else: timer may have just been cancelled above
    if (!tReassemblyTimer_->isScheduled()) {
        bool missingAndHole = (rxBuffer_->getRxNextHighest() == currentRxNext + 1 && hasHoles);
        if (rxBuffer_->getRxNextHighest() > currentRxNext + 1 || missingAndHole) {
            EV << NOW << " NrAmRxQueue::enque() t_ReassemblyTimer scheduled" << endl;
            scheduleAfter(tReassembly_, tReassemblyTimer_);
            rxNextStatusTrigger_ = rxBuffer_->getRxNextHighest();
        }
    }
}





void NrAmRxQueue::passUp(int seqNum)
{
    Enter_Method("passUp");

    Packet *bufferedPkt = rxBuffer_->consumeSdu(seqNum);
    if (!bufferedPkt)
        throw cRuntimeError("NrAmRxQueue::passUp() null PDU for seqNum=%d", seqNum);

    auto pdu = bufferedPkt->removeAtFront<NrRlcAmDataPdu>();
    if (pdu->getNumSdu() < 1)
        throw cRuntimeError("NrAmRxQueue::passUp() PDU has no SDU");

    size_t sduLengthPktLen;
    auto sdu = pdu->popSdu(sduLengthPktLen);
    auto sduRlc = sdu->peekAtFront<LteRlcSdu>();
    EV << NOW << " NrAmRxQueue::passUp() SDU[" << sduRlc->getSnoMainPacket()
       << "] from PDU sn=" << seqNum << endl;

    auto ci = sdu->getTag<FlowControlInfo>();
    sdu->removeAtFront<LteRlcSdu>();

    Direction dir = (Direction)ci->getDirection();
    MacNodeId dstId = ci->getDestId();
    MacNodeId srcId = ci->getSourceId();
    cModule *nodeb = nullptr;
    cModule *ue = nullptr;
    double delay = (NOW - sdu->getCreationTime()).dbl();

    if (dir == DL) {
        nodeb = getRlcByMacNodeId(binder_, srcId, AM);
        ue = getRlcByMacNodeId(binder_, dstId, AM);
    }
    else {
        nodeb = getRlcByMacNodeId(binder_, dstId, AM);
        ue = getRlcByMacNodeId(binder_, srcId, AM);
    }

    auto length = sdu->getByteLength();
    totalRcvdBytes_ += length;
    totalCellRcvdBytes_ += length;
    double tputSample = (double)totalRcvdBytes_ / (NOW - getSimulation()->getWarmupPeriod());
    double cellTputSample = (double)totalCellRcvdBytes_ / (NOW - getSimulation()->getWarmupPeriod());

    tpSample_ += length;
    cellTpSample_ += length;

    if (nodeb != nullptr) {
        nodeb->emit(rlcCellThroughputSignal_[dir_], cellTputSample);
        nodeb->emit(rlcCellPacketLossSignal_[dir_], 0.0);
        double cellInterval = (NOW - lastCellTputSample_).dbl();
        if (cellInterval >= 1) {
            nodeb->emit(rlcCellThroughputSampleSignal_[dir_], (double)cellTpSample_ / cellInterval);
            cellTpSample_ = 0;
            lastCellTputSample_ = NOW;
        }
    }
    if (ue != nullptr) {
        ue->emit(rlcThroughputSignal_[dir_], tputSample);
        ue->emit(rlcDelaySignal_[dir_], delay);
        ue->emit(rlcPacketLossSignal_[dir_], 0.0);
    }

    lteRlc_->sendDefragmented(sdu);
    passedUpSdus_.insert(sduRlc->getSnoMainPacket());
    delete bufferedPkt;
}
void NrAmRxQueue::setRemoteEntity(MacNodeId id)
{
    remoteEntity_ = getRlcByMacNodeId(binder_, id, AM);
}

void NrAmRxQueue::handleControlPdu(inet::Packet *pkt)
{
    auto pdu = pkt->peekAtFront<NrRlcAmStatusPdu>();
    if (pdu->getAmType() == ACK) {
        EV << NOW << " NrAmRxQueue::handleControlPdu Received ACK" << endl;
        lteRlc_->routeControlMessage(pkt);
    }
    else {
        throw cRuntimeError("NrAmRxQueue::handleControlPdu unknown status PDU type=%d", pdu->getAmType());
    }
}

void NrAmRxQueue::sendStatusReport()
{
    Enter_Method("sendStatusReport()");

    if (tStatusProhibitTimer_->isScheduled()) {
        EV << NOW << " NrAmRxQueue::sendStatusReport, minimum interval not reached "
           << tStatusProhibit_ << endl;
        statusReportPending_ = true;
        return;
    }

    StatusPduData data = rxBuffer_->generateStatusPduData();

    auto pktPdu = new Packet("NR STATUS AM PDU");
    auto pdu = makeShared<NrRlcAmStatusPdu>();
    pdu->setAmType(ACK);
    pdu->setData(data);

    // TODO: compute proper length per TS 38.322 6.2.2.5
    // Header(1) + ACK_SN(2) + E1 = 3 bytes base
    unsigned int size = 3;
    for (const auto &nack : data.nacks) {
        size++; // NACK_SN
        if (nack.isSegment)
            size += 4; // SOstart + SOend
        else if (nack.nackRange > 1)
            size++;
    }
    pdu->setChunkLength(B(size));

    *pktPdu->addTagIfAbsent<FlowControlInfo>() = *flowControlInfo_;
    pktPdu->insertAtFront(pdu);

    EV << NOW << " NrAmRxQueue::sendStatusReport. Last sent " << lastSentAck_ << endl;
    lteRlc_->bufferControlPdu(pktPdu);
    lastSentAck_ = NOW;
    scheduleAfter(tStatusProhibit_, tStatusProhibitTimer_);
    statusReportPending_ = false;
}

void NrAmRxQueue::handleMessage(cMessage *msg)
{
    if (msg == tReassemblyTimer_) {
        rxBuffer_->handleReassemblyTimerExpiry(rxNextStatusTrigger_);
        bool hasHoles = rxBuffer_->hasMissingByteSegmentBeforeLast(rxBuffer_->getRxHighestStatus());
        bool restart = (rxBuffer_->getRxNextHighest() == rxBuffer_->getRxHighestStatus() && hasHoles);

        if (rxBuffer_->getRxNextHighest() > rxBuffer_->getRxHighestStatus() + 1 || restart) {
            scheduleAfter(tReassembly_, tReassemblyTimer_);
            rxNextStatusTrigger_ = rxBuffer_->getRxNextHighest();
        }
        // TS 38.322 5.3.4
        sendStatusReport();
    }
    // TODO: When the TX entity exceeds maxRtx, the TS says it should notify upper layers.
    // The RX entity window can also get stalled but there's no discard mechanism here yet.
    else if (msg == tStatusProhibitTimer_) {
        if (statusReportPending_)
            sendStatusReport();
    }
    else if (msg == throughputTimer_) {
        if (remoteEntity_) {
            double tput = (double)tpSample_ / throughputInterval_;
            remoteEntity_->emit(rlcThroughputSampleSignal_[dir_], tput);
            tpSample_ = 0;
        }
        scheduleAfter(throughputInterval_, throughputTimer_);
    }
}

} //namespace
