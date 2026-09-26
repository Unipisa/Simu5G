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

#include "simu5g/stack/rlc/um/NrRlcUmRxEntity.h"
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"
#include "simu5g/stack/rlc/RlcMux.h"

namespace simu5g {

Define_Module(NrRlcUmRxEntity);

using namespace inet;

NrRlcUmRxEntity::~NrRlcUmRxEntity()
{
    Enter_Method("~NrRlcUmRxEntity");
    if (t_ReassemblyTimer)
        cancelAndDelete(t_ReassemblyTimer);
    if (sduBuffer) {
        sduBuffer->clearBuffer();
        delete sduBuffer;
    }
}

void NrRlcUmRxEntity::initMode(LteMacBase *mac)
{
    UM_Window_Size = par("UM_Window_Size");
    // Window = 2^(snBits-1). 3GPP UM SN lengths: LTE 5/10 bits (window 16/512),
    // NR 6/12 bits (window 32/2048).
    int snBits;
    switch (UM_Window_Size) {
        case 16:   snBits = 5;  break;
        case 32:   snBits = 6;  break;
        case 512:  snBits = 10; break;
        case 2048: snBits = 12; break;
        default:
            throw cRuntimeError("NrRlcUmRxEntity::initialize() UM_Window_Size=%d, but only 16, 32, 512 or 2048 are valid", UM_Window_Size);
    }
    sduBuffer = new RlcUmReceptionBuffer(snBits);
    t_ReassemblyTimer = new cMessage("UM Reassembly timer");
    t_Reassembly = par("t_Reassembly");
    // The mux feeding our "in" gate (for UL burst-throughput reporting).
    rlcMux_ = getModuleFromPar<RlcMux>(par("rlcMuxModule"), this);

    WATCH(totalRcvdBytesNr_);
    WATCH(recentCompleteSduSet_);
    WATCH(totalPduRcvdBytes_);
}

void NrRlcUmRxEntity::emitRxStatistics(bool perPdu, double throughput, simtime_t delay)
{
    Direction dir = static_cast<Direction>(flowControlInfo_->getDirection());
    emit(perPdu ? rlcPduThroughputSignal_[dir] : rlcThroughputSignal_[dir], throughput);
    emit(perPdu ? rlcPduDelaySignal_[dir] : rlcDelaySignal_[dir], delay.dbl());
}

void NrRlcUmRxEntity::handleMessage(cMessage *msg)
{
    if (msg == t_ReassemblyTimer) {
        sduBuffer->onTimerExpiry();
        if (sduBuffer->startTimer())
            scheduleAfter(t_Reassembly, t_ReassemblyTimer);
        return;
    }
    auto pkt = check_and_cast<Packet *>(msg);
    if (pkt->getArrivalGate() && pkt->getArrivalGate()->isName("in")) {
        emit(receivedPacketFromLowerLayerSignal_, pkt);
        enque(pkt);
    }
    else {
        throw cRuntimeError("NrRlcUmRxEntity: unexpected message %s", msg->getFullName());
    }
}

void NrRlcUmRxEntity::enque(cPacket *pktAux)
{
    auto pktPdu = check_and_cast<Packet *>(pktAux);

    EV << NOW << " NrRlcUmRxEntity::enque - buffering new PDU" << endl;

    // Per-PDU delay and throughput, as seen on the air interface (before reassembly).
    totalPduRcvdBytes_ += pktPdu->getByteLength();
    emitRxStatistics(true, (double)totalPduRcvdBytes_ / (NOW - getSimulation()->getWarmupPeriod()),
            NOW - pktPdu->getCreationTime());

    auto pdu = pktPdu->removeAtFront<NrRlcUmDataPdu>();

    // A full SDU (FI=00) can be delivered immediately; a segment goes to the
    // reassembly buffer.
    FramingInfo fi = pdu->getFramingInfo();
    if (fi.toValue() == 0) {
        // A complete SDU (FI=00) carries no RLC SN (TS 38.322) and bypasses the
        // reassembly window, so it has no duplicate protection there. Simu5G's MAC
        // HARQ can re-deliver an already-delivered TB (a retransmission whose ACK was
        // not yet processed, on a lossy channel), which would deliver the SDU twice.
        // Discard such duplicates here, mirroring the duplicate-discard that the
        // segmented path (SN window) and the LTE-FI path (per-PDU SN) already provide.
        // Keyed by the per-SDU identity (snoMainPacket = PDCP SN), within the
        // reassembly window; clean channels never produce duplicates, so this is inert.
        size_t sduLength;
        auto pktSdu = check_and_cast<Packet *>(pdu->popSdu(sduLength));
        unsigned int sno = pdu->getSnoMainPacket();
        while (!recentCompleteSduQueue_.empty() && NOW - recentCompleteSduQueue_.front().first > t_Reassembly) {
            recentCompleteSduSet_.erase(recentCompleteSduQueue_.front().second);
            recentCompleteSduQueue_.pop_front();
        }
        if (recentCompleteSduSet_.find(sno) != recentCompleteSduSet_.end()) {
            EV << NOW << " NrRlcUmRxEntity::enqueNr - discarding duplicate complete SDU (snoMainPacket " << sno << ")" << endl;
            delete pktSdu;
        }
        else {
            recentCompleteSduSet_.insert(sno);
            recentCompleteSduQueue_.emplace_back(NOW, sno);
            ttiBits_ += sduLength;   // UL burst accounting
            toPdcpNr(pktSdu);
        }
    }
    else {
        unsigned int tsn = pdu->getPduSequenceNumber();
        ttiBits_ += (pdu->getEndOffset() - pdu->getStartOffset() + 1);   // UL burst accounting
        handlePDUInReceivedBuffer(pdu, tsn);
    }

    pktPdu->insertAtFront(pdu);
    delete pktPdu;

    // UL data-burst throughput accounting is done only at the eNB on the UL leg
    if (flowControlInfo_ && flowControlInfo_->getDirection() == UL)
        handleBurst(ENQUE);
}

void NrRlcUmRxEntity::handlePDUInReceivedBuffer(inet::Ptr<NrRlcUmDataPdu> pdu, unsigned int tsn)
{
    size_t totalLength = pdu->getLengthMainPacket();
    auto sdu = check_and_cast<Packet *>(pdu->popSdu(totalLength));

    bool complete = sduBuffer->handleSegment(tsn, totalLength, pdu->getStartOffset(), pdu->getEndOffset(), sdu);
    if (complete) // otherwise the SDU is buffered or was discarded
        toPdcpNr(sdu);

    if (t_ReassemblyTimer->isScheduled()) {
        if (sduBuffer->stopTimer(true))
            cancelEvent(t_ReassemblyTimer);
    }

    // not else: the timer may have been cancelled just above
    if (!t_ReassemblyTimer->isScheduled()) {
        if (sduBuffer->startTimer())
            scheduleAfter(t_Reassembly, t_ReassemblyTimer);
    }
}

void NrRlcUmRxEntity::toPdcpNr(Packet *pktAux)
{
    // Set the receive-side flow info for PDCP routing and drop the TX-internal tag.
    *pktAux->addTagIfAbsent<FlowControlInfo>() = *flowControlInfo_;
    pktAux->removeTagIfPresent<PdcpTrackingTag>();

    // This bearer's delay and throughput of the reassembled SDU
    totalRcvdBytesNr_ += pktAux->getByteLength();
    double tput = (double)totalRcvdBytesNr_ / (NOW - getSimulation()->getWarmupPeriod());
    emitRxStatistics(false, tput, NOW - pktAux->getCreationTime());

    emit(sentPacketToUpperLayerSignal_, pktAux);

    EV << NOW << " NrRlcUmRxEntity::toPdcp - deliver SDU of " << pktAux->getByteLength() << " bytes to upper layer" << endl;
    send(pktAux, "out");
}


bool NrRlcUmRxEntity::isEmpty() const
{
    return sduBuffer == nullptr || sduBuffer->isEmpty();
}

} //namespace
