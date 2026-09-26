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

#include "simu5g/stack/rlc/um/LteRlcUmRxEntity.h"
#include "simu5g/stack/rlc/packet/LteRlcPdu_m.h"
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"
#include "simu5g/stack/mac/LteMacBase.h"
#include "simu5g/stack/mac/LteMacEnb.h"
#include "simu5g/stack/rlc/RlcMux.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/mec/utils/MecCommon.h"

namespace simu5g {

Define_Module(LteRlcUmRxEntity);

using namespace inet;


LteRlcUmRxEntity::LteRlcUmRxEntity() :
    t_reordering_(this)
{
    t_reordering_.setTimerId(REORDERING_T);
    buffered_.pkt = nullptr;
    buffered_.size = 0;
}

LteRlcUmRxEntity::~LteRlcUmRxEntity()
{
    Enter_Method("~LteRlcUmRxEntity");
    delete buffered_.pkt;
}

void LteRlcUmRxEntity::initMode(LteMacBase *mac)
{
    timeout_ = par("timeout").doubleValue();
    rxWindowDesc_.clear();
    rxWindowDesc_.windowSize_ = par("rxWindowSize");
    received_.resize(rxWindowDesc_.windowSize_);
    rlcMux_ = getModuleFromPar<RlcMux>(par("rlcMuxModule"), this);
    dir_ = mac->getNodeType() == NODEB ? UL : DL;

    WATCH(pduBuffer_);
    WATCH(rxWindowDesc_);
    WATCH(t_reordering_);
    WATCH(buffered_);
    WATCH(lastPduReassembled_);
    WATCH(init_);
    WATCH(totalPduRcvdBytes_);
    WATCH(totalRcvdBytes_);
    WATCH(dir_);
}

void LteRlcUmRxEntity::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        t_reordering_.handle();

        EV << NOW << " LteRlcUmRxEntity::handleMessage : t_reordering timer has expired " << endl;

        unsigned int old = rxWindowDesc_.firstSnoForReordering_;

        // move to the first missing SN
        while (received_.at(rxWindowDesc_.firstSnoForReordering_ - rxWindowDesc_.firstSno_) == true
               || rxWindowDesc_.firstSnoForReordering_ < rxWindowDesc_.reorderingSno_)
        {
            rxWindowDesc_.firstSnoForReordering_++;
            if (rxWindowDesc_.firstSnoForReordering_ == rxWindowDesc_.highestReceivedSno_) // end of the window
                break;
        }

        int index = old - rxWindowDesc_.firstSno_;
        for (unsigned int i = index; i < rxWindowDesc_.firstSnoForReordering_ - rxWindowDesc_.firstSno_; i++) {
            // try to reassemble
            reassemble(i);
        }

        if (rxWindowDesc_.highestReceivedSno_ > rxWindowDesc_.firstSnoForReordering_) {
            rxWindowDesc_.reorderingSno_ = rxWindowDesc_.highestReceivedSno_;
            t_reordering_.start(timeout_);
        }

        delete msg;
    }
    else if (msg->getArrivalGate() && msg->getArrivalGate()->isName("in")) {
        // RLC PDU from RlcMux — enqueue for reassembly
        auto pkt = check_and_cast<inet::Packet *>(msg);
        EV << "LteRlcUmRxEntity::handleMessage - Enqueue packet " << pkt->getName() << " from RlcMux\n";
        enque(pkt);
    }
    else {
        throw cRuntimeError("LteRlcUmRxEntity: unexpected message %s", msg->getFullName());
    }
}

void LteRlcUmRxEntity::enque(cPacket *pktAux)
{
    Enter_Method("enque()");
    take(pktAux);

    EV << NOW << " LteRlcUmRxEntity::enque - buffering new PDU" << endl;

    auto pktPdu = check_and_cast<Packet *>(pktAux);
    auto pdu = pktPdu->peekAtFront<LteRlcUmDataPdu>();
    auto lteInfo = pktPdu->getTag<FlowControlInfo>();

    // Get the RLC PDU Transmission sequence number (x)
    unsigned int tsn = pdu->getPduSequenceNumber();

    onFirstPduEnqueued(tsn);

    // get the position in the buffer
    int index = tsn - rxWindowDesc_.firstSno_;

    EV << NOW << " LteRlcUmRxEntity::enque - tsn " << tsn << ", the corresponding index in the buffer is " << index << endl;

    // x was already received
    if (tsn >= rxWindowDesc_.firstSnoForReordering_ && tsn < rxWindowDesc_.highestReceivedSno_ && received_.at(index) == true) {
        EV << NOW << " LteRlcUmRxEntity::enque the received PDU has index " << index << " which points to an already busy location. Discard the PDU" << endl;
        delete pktPdu;
        return;
    }

    // x was already considered for reordering & reassembling
    if (tsn < rxWindowDesc_.firstSnoForReordering_) {
        EV << NOW << " LteRlcUmRxEntity::enque the received PDU with " << tsn << " SN was already considered for reordering. Discard the PDU" << endl;
        delete pktPdu;
        return;
    }

    // x falls outside the rxWindow
    if (tsn >= rxWindowDesc_.highestReceivedSno_) {
        // move forward the rxWindow and try to reassemble

        unsigned int old = rxWindowDesc_.highestReceivedSno_;
        rxWindowDesc_.highestReceivedSno_ = tsn + 1;
        if (rxWindowDesc_.firstSno_ + rxWindowDesc_.windowSize_ < rxWindowDesc_.highestReceivedSno_) {
            int shift = rxWindowDesc_.highestReceivedSno_ - old;
            while (shift > 0) {

                // if "shift" is greater than the window size, we advance the window in several steps

                int p = (shift < rxWindowDesc_.windowSize_) ? shift : rxWindowDesc_.windowSize_;
                shift -= p;
                if (rxWindowDesc_.firstSno_ + p > tsn) { // HACK to avoid that the window goes ahead of the received tsn
                    p = tsn - rxWindowDesc_.firstSno_;
                }

                for (int i = 0; i < p; i++) {
                    // try to reassemble the PDU
                    reassemble(i);
                }

                // move the window (update buffer and firstSno)
                moveRxWindow(p);
            }

            // check whether firstSnoForReordering_ falls out of the window
            if (rxWindowDesc_.firstSnoForReordering_ < rxWindowDesc_.firstSno_) {
                rxWindowDesc_.firstSnoForReordering_ = rxWindowDesc_.firstSno_;
            }
        }
    }

    // buffer the received PDU at the correct position in the buffer
    // get the position in the buffer (the buffer may have been shifted)
    index = tsn - rxWindowDesc_.firstSno_;
    pduBuffer_.addAt(index, pktPdu);
    received_.at(index) = true;

    // emit statistics
    double tputSample = (double)totalPduRcvdBytes_ / (NOW - getSimulation()->getWarmupPeriod());

    emitPduStats(static_cast<Direction>(lteInfo->getDirection()), tputSample, pktPdu->getCreationTime());

    EV << NOW << " LteRlcUmRxEntity::enque - tsn " << tsn << ", the corresponding index after shift in the buffer is " << index << endl;
    EV << NOW << " LteRlcUmRxEntity::enque - firstSnoReordering " << rxWindowDesc_.firstSnoForReordering_ << endl;

    index = rxWindowDesc_.firstSnoForReordering_ - rxWindowDesc_.firstSno_;

    if (received_.at(rxWindowDesc_.firstSnoForReordering_ - rxWindowDesc_.firstSno_) == true) {
        unsigned int old = rxWindowDesc_.firstSnoForReordering_;

        index = rxWindowDesc_.firstSnoForReordering_ - rxWindowDesc_.firstSno_;

        // move to the first missing SN
        while (received_.at(rxWindowDesc_.firstSnoForReordering_ - rxWindowDesc_.firstSno_) == true) {
            rxWindowDesc_.firstSnoForReordering_++;
            if (rxWindowDesc_.firstSnoForReordering_ == rxWindowDesc_.highestReceivedSno_) // end of the window
                break;
        }

        int index = old - rxWindowDesc_.firstSno_;
        for (unsigned int i = index; i < rxWindowDesc_.firstSnoForReordering_ - rxWindowDesc_.firstSno_; i++) {
            // try to reassemble
            reassemble(i);
        }
    }

    // handle t-reordering

    // if t_reordering is running
    if (t_reordering_.busy()) {
        if (rxWindowDesc_.reorderingSno_ <= rxWindowDesc_.firstSnoForReordering_ ||
            rxWindowDesc_.reorderingSno_ < rxWindowDesc_.firstSno_ || rxWindowDesc_.reorderingSno_ > rxWindowDesc_.highestReceivedSno_)
        {
            t_reordering_.stop();
        }
    }
    // if t_reordering is not running
    if (!t_reordering_.busy()) {
        if (rxWindowDesc_.highestReceivedSno_ > rxWindowDesc_.firstSnoForReordering_) {
            t_reordering_.start(timeout_);
            rxWindowDesc_.reorderingSno_ = rxWindowDesc_.highestReceivedSno_;
        }
    }

    if (flowControlInfo_->getDirection() == UL) { //only eNodeB checks the burst
        handleBurst(ENQUE);
    }
}

void LteRlcUmRxEntity::onFirstPduEnqueued(unsigned int pduSno)
{
    // nothing to do; subclasses may initialize the reordering window
    // (e.g. the D2D_MULTI single-PDU window)
}

void LteRlcUmRxEntity::emitPduStats(Direction dir, double tputSample, simtime_t creationTime)
{
    emit(rlcPduThroughputSignal_[dir_], tputSample);
    emit(rlcPduDelaySignal_[dir_], (NOW - creationTime).dbl());
}

void LteRlcUmRxEntity::moveRxWindow(int pos)
{
    EV << NOW << " LteRlcUmRxEntity::moveRxWindow moving forth by " << pos << " locations" << endl;

    if (pos <= 0)
        return;                   // ignore the shift, it is ineffective.

    if (pos > rxWindowDesc_.windowSize_)
        throw cRuntimeError("LteRlcUmRxEntity::moveRxWindow(): positions %d win size %d ", pos, rxWindowDesc_.windowSize_);

    for (unsigned int i = pos; i < rxWindowDesc_.windowSize_; ++i) {
        if (pduBuffer_.get(i) != nullptr) {
            pduBuffer_.addAt(i - pos, pduBuffer_.remove(i));
        }
        else {
            pduBuffer_.remove(i);
        }
        received_.at(i - pos) = received_.at(i);
        received_.at(i) = false;
    }

    rxWindowDesc_.firstSno_ += pos;

    EV << NOW << " LteRlcUmRxEntity::moveRxWindow first sequence number updated to " << rxWindowDesc_.firstSno_ << endl;
}

void LteRlcUmRxEntity::toPdcpLte(Packet *pktAux)
{
    // Remove leftover tag added on the TX side.
    pktAux->removeTagIfPresent<PdcpTrackingTag>();

    auto lteInfo = pktAux->getTag<FlowControlInfo>();
    unsigned int length = pktAux->getByteLength();
    simtime_t ts = pktAux->getCreationTime();

    // emit statistics (throughput and delay only - no packet loss)
    totalRcvdBytes_ += length;
    double tputSample = (double)totalRcvdBytes_ / (NOW - getSimulation()->getWarmupPeriod());

    emitSduStats(static_cast<Direction>(lteInfo->getDirection()), tputSample, ts);

    EV << NOW << " LteRlcUmRxEntity::toPdcp Created PDCP PDU with length " << pktAux->getByteLength() << " bytes" << endl;
    EV << NOW << " LteRlcUmRxEntity::toPdcp Send packet to upper layer" << endl;

    send(pktAux, "out");
}

void LteRlcUmRxEntity::emitSduStats(Direction dir, double tputSample, simtime_t creationTime)
{
    emit(rlcThroughputSignal_[dir_], tputSample);
    emit(rlcDelaySignal_[dir_], (NOW - creationTime).dbl());
}

bool LteRlcUmRxEntity::consumeReassemblyReset(unsigned int pduSno)
{
    // no reassembly reset is pending on an infrastructure bearer; the D2D profile
    // consumes the post-mode-switch resynchronisation here
    return false;
}

void LteRlcUmRxEntity::reassemble(unsigned int index)
{
    Enter_Method("reassemble()");

    if (received_.at(index) == false) {
        // consider the case when a PDU is missing or already delivered
        EV << NOW << " LteRlcUmRxEntity::reassemble PDU at index " << index << " has not been received or already delivered" << endl;
        return;
    }

    EV << NOW << " LteRlcUmRxEntity::reassemble Consider PDU at index " << index << " for reassembly" << endl;

    auto pktPdu = check_and_cast<Packet *>(pduBuffer_.get(index));
    auto pdu = pktPdu->removeAtFront<LteRlcUmDataPdu>();
    auto lteInfo = pktPdu->getTag<FlowControlInfo>();

    // get PDU seq number
    unsigned int pduSno = pdu->getPduSequenceNumber();

    // consume a pending reassembly reset (if any); when consumed, the first
    // extracted SDU of this PDU must be considered in order (ignoreFragment)
    bool ignoreFragment = consumeReassemblyReset(pduSno);

    // get framing info
    FramingInfo fi = pdu->getFramingInfo();

    // get the number of (portions of) SDUs in the PDU
    unsigned int numSdu = pdu->getNumSdu();

    // for each SDU
    for (unsigned int i = 0; i < numSdu; i++) {
        size_t sduLengthPktLeng;
        auto pktSdu = check_and_cast<Packet *>(pdu->popSdu(sduLengthPktLeng));

        *pktSdu->addTag<FlowControlInfo>() = *flowControlInfo_;

        auto pdcpTag = pktSdu->getTag<PdcpTrackingTag>();
        unsigned int sduSno = pdcpTag->getPdcpSequenceNumber();
        unsigned int sduWholeLength = pdcpTag->getOriginalPacketLength(); // the length of the whole sdu

        if (i == 0) { // first SDU
            if (i == numSdu - 1) { // [first SDU, i==0] there is only one SDU in this PDU
                // read the FI field
                switch (fi.toValue()) {
                    case 0: {  // FI=00
                        EV << NOW << " LteRlcUmRxEntity::reassemble The PDU includes one whole SDU [sno=" << sduSno << "]" << endl;
                        if (sduLengthPktLeng != sduWholeLength)
                            throw cRuntimeError("LteRlcUmRxEntity::reassemble(): failed reassembly, the reassembled SDU has size %zu B, while the original SDU had size %d B", sduLengthPktLeng, sduWholeLength);

                        toPdcpLte(pktSdu);
                        pktSdu = nullptr;

                        clearBufferedSdu();

                        break;
                    }
                    case 1: {  // FI=01
                        EV << NOW << " LteRlcUmRxEntity::reassemble The PDU includes the first part [" << sduLengthPktLeng << " B] of an SDU [sno=" << sduSno << "]" << endl;

                        clearBufferedSdu();

                        // buffer the SDU and wait for the missing portion
                        buffered_.pkt = pktSdu;
                        pktSdu = nullptr;
                        buffered_.size = sduLengthPktLeng;
                        buffered_.currentPduSno = pduSno;

                        // for burst
                        ttiBits_ += sduLengthPktLeng;
                        EV << NOW << " LteRlcUmRxEntity::reassemble Wait for the missing part..." << endl;

                        break;
                    }
                    case 2: {  // FI=10
                        // it is the last portion of an SDU, take the awaiting SDU
                        EV << NOW << " LteRlcUmRxEntity::reassemble The PDU includes the last part [" << sduLengthPktLeng << " B] of an SDU [sno=" << sduSno << "]" << endl;

                        // check SDU SN
                        if (buffered_.pkt == nullptr ||
                            (pdcpTag->getPdcpSequenceNumber() != buffered_.pkt->getTag<PdcpTrackingTag>()->getPdcpSequenceNumber()) ||
                            (pduSno != (buffered_.currentPduSno + 1)) ||
                            ignoreFragment)
                        {
                            if (pduSno != buffered_.currentPduSno) {
                                EV << NOW << " LteRlcUmRxEntity::reassemble The SDU cannot be reassembled, Pdu sequence numbers are not in sequence" << endl;
                            }
                            else {
                                EV << NOW << " LteRlcUmRxEntity::reassemble The SDU cannot be reassembled, first part missing" << endl;
                            }
                            clearBufferedSdu();

                            delete pktSdu;
                            pktSdu = nullptr;
                            continue;
                        }

                        EV << NOW << " LteRlcUmRxEntity::reassemble The waiting SDU has size " << buffered_.size << " bytes" << endl;

                        unsigned int reassembledLength = buffered_.size + sduLengthPktLeng;
                        if (reassembledLength < sduWholeLength) {
                            clearBufferedSdu();
                            EV << NOW << " LteRlcUmRxEntity::reassemble The SDU cannot be reassembled, mid part missing" << endl;

                            delete pktSdu;
                            pktSdu = nullptr;
                            continue;
                        }
                        else if (reassembledLength > sduWholeLength) {
                            throw cRuntimeError("LteRlcUmRxEntity::reassemble(): failed reassembly, the reassembled SDU has size %d B, while the original SDU had size %d B", reassembledLength, sduWholeLength);
                        }

                        // for burst
                        ttiBits_ += sduLengthPktLeng;
                        toPdcpLte(pktSdu);
                        pktSdu = nullptr;

                        clearBufferedSdu();

                        break;
                    }
                    case 3: {  // FI=11
                        // add the length of this SDU to the awaiting SDU and wait for the missing portion
                        EV << NOW << " LteRlcUmRxEntity::reassemble The PDU includes the mid part [" << sduLengthPktLeng << " B] of an SDU [sno=" << sduSno << "]" << endl;

                        // check SDU SN
                        if (buffered_.pkt == nullptr ||
                            (pdcpTag->getPdcpSequenceNumber() != buffered_.pkt->getTag<PdcpTrackingTag>()->getPdcpSequenceNumber()) ||
                            (pduSno != (buffered_.currentPduSno + 1)) ||
                            ignoreFragment)
                        {
                            if (pduSno != buffered_.currentPduSno) {
                                EV << NOW << " LteRlcUmRxEntity::reassemble The SDU cannot be reassembled, Pdu sequence numbers are not in sequence" << endl;
                            }
                            else {
                                EV << NOW << " LteRlcUmRxEntity::reassemble The SDU cannot be reassembled, first part missing" << endl;
                            }
                            clearBufferedSdu();

                            delete pktSdu;
                            pktSdu = nullptr;
                            continue;
                        }

                        // for burst
                        ttiBits_ += sduLengthPktLeng;
                        buffered_.size += sduLengthPktLeng;
                        buffered_.currentPduSno = pduSno;
                        delete pktSdu;
                        pktSdu = nullptr;

                        EV << NOW << " LteRlcUmRxEntity::reassemble The waiting SDU has size " << buffered_.size << " bytes, was " << buffered_.size - sduLengthPktLeng << " bytes" << endl;
                        EV << NOW << " LteRlcUmRxEntity::reassemble Wait for the missing part..." << endl;

                        break;
                    }
                    default: {
                        throw cRuntimeError("LteRlcUmRxEntity::reassemble(): FI field was not valid %d ", fi.toValue());
                    }
                }
            }
            else { // [first SDU, i==0] there is more than one SDU in this PDU
                EV << NOW << " LteRlcUmRxEntity::reassemble Read the first chunk of the PDU" << endl;

                // read the FI field
                if (!fi.firstIsFragment) {
                    {  // FI=00 or FI=01
                        // it is a whole SDU, send the SDU to the PDCP
                        EV << NOW << " LteRlcUmRxEntity::reassemble This is a whole SDU [sno=" << sduSno << "]" << endl;
                        if (sduLengthPktLeng != sduWholeLength)
                            throw cRuntimeError("LteRlcUmRxEntity::reassemble(): failed reassembly, the reassembled SDU has size %zu B, while the original SDU had size %d B", sduLengthPktLeng, sduWholeLength);

                        // for burst
                        ttiBits_ += sduLengthPktLeng;
                        toPdcpLte(pktSdu);
                        pktSdu = nullptr;

                        clearBufferedSdu();
                    }
                }
                else {
                    {  // FI=10 or FI=11
                        // it is the last portion of an SDU, take the awaiting SDU and send to the PDCP
                        EV << NOW << " LteRlcUmRxEntity::reassemble This is the last part [" << sduLengthPktLeng << " B] of an SDU [sno=" << sduSno << "]" << endl;

                        // check SDU SN
                        if (buffered_.pkt == nullptr ||
                            (pdcpTag->getPdcpSequenceNumber() != buffered_.pkt->getTag<PdcpTrackingTag>()->getPdcpSequenceNumber()) ||
                            (pduSno != (buffered_.currentPduSno + 1)) ||
                            ignoreFragment)
                        {
                            if (pduSno != (buffered_.currentPduSno + 1)) {
                                EV << NOW << " LteRlcUmRxEntity::reassemble The SDU cannot be reassembled, Pdu sequence numbers are not in sequence" << endl;
                            }
                            else {
                                EV << NOW << " LteRlcUmRxEntity::reassemble The SDU cannot be reassembled, first part missing" << endl;
                            }
                            clearBufferedSdu();
                            EV << NOW << " LteRlcUmRxEntity::reassemble The SDU cannot be reassembled, first part missing" << endl;

                            delete pktSdu;
                            pktSdu = nullptr;

                            continue;
                        }

                        EV << NOW << " LteRlcUmRxEntity::reassemble The waiting SDU has size " << buffered_.size << " bytes" << endl;

                        unsigned int reassembledLength = buffered_.size + sduLengthPktLeng;
                        if (reassembledLength < sduWholeLength) {
                            clearBufferedSdu();
                            EV << NOW << " LteRlcUmRxEntity::reassemble The SDU cannot be reassembled, mid part missing" << endl;

                            delete pktSdu;

                            continue;
                        }
                        else if (reassembledLength > sduWholeLength) {
                            throw cRuntimeError("LteRlcUmRxEntity::reassemble(): failed reassembly, the reassembled SDU has size %d B, while the original SDU had size %d B", reassembledLength, sduWholeLength);
                        }

                        // for burst
                        ttiBits_ += sduWholeLength; // remove the discarded SDU size from the throughput
                        toPdcpLte(pktSdu);
                        pktSdu = nullptr;

                        clearBufferedSdu();
                    }
                }
            }
        }
        else if (i == numSdu - 1) { // last SDU in PDU with at least 2 SDUs
            // read the FI field
            if (!fi.lastIsFragment) {
                {  // FI=00 or FI=10
                    // it is a whole SDU, send the SDU to the PDCP
                    EV << NOW << " LteRlcUmRxEntity::reassemble This is a whole SDU [sno=" << sduSno << "]" << endl;
                    if (sduLengthPktLeng != sduWholeLength)
                        throw cRuntimeError("LteRlcUmRxEntity::reassemble(): failed reassembly, the reassembled SDU has size %zu B, while the original SDU had size %d B", sduLengthPktLeng, sduWholeLength);

                    // for burst
                    ttiBits_ += sduLengthPktLeng;

                    toPdcpLte(pktSdu);
                    pktSdu = nullptr;

                    clearBufferedSdu();
                }
            }
            else {
                {  // FI=01 or FI=11
                    // it is the first portion of an SDU, buffer it
                    EV << NOW << " LteRlcUmRxEntity::reassemble The PDU includes the first part [" << sduLengthPktLeng << " B] of an SDU [sno=" << sduSno << "]" << endl;

                    clearBufferedSdu();

                    // for burst
                    ttiBits_ += sduLengthPktLeng;

                    buffered_.pkt = pktSdu;
                    buffered_.size = sduLengthPktLeng;
                    buffered_.currentPduSno = pduSno;
                    pktSdu = nullptr;

                    EV << NOW << " LteRlcUmRxEntity::reassemble Wait for the missing part..." << endl;
                }
            }
        }
        else {
            // it is a whole SDU, send to the PDCP
            EV << NOW << " LteRlcUmRxEntity::reassemble This is a whole SDU [sno=" << sduSno << "]" << endl;
            if (sduLengthPktLeng != sduWholeLength)
                throw cRuntimeError("LteRlcUmRxEntity::reassemble(): failed reassembly, the reassembled SDU has size %zu B, while the original SDU had size %d B", sduLengthPktLeng, sduWholeLength);

            // for burst
            ttiBits_ += sduLengthPktLeng;
            toPdcpLte(pktSdu);
            pktSdu = nullptr;

            clearBufferedSdu();
        }

        if (pktSdu != nullptr) {
            delete pktSdu;
            pktSdu = nullptr;
        }
    }
    // remove PDU from buffer
    pduBuffer_.remove(index);
    received_.at(index) = false;
    EV << NOW << " LteRlcUmRxEntity::reassemble Removed PDU from position " << index << endl;

    // update the last PDU reassembled to the current PDU sequence number
    lastPduReassembled_ = pduSno;

    pktPdu->insertAtFront(pdu);

    delete pktPdu;
}

void LteRlcUmRxEntity::clearBufferedSdu()
{
    if (buffered_.pkt != nullptr) {
        // for burst
        ttiBits_ -= buffered_.size; // remove the discarded SDU size from the throughput
        delete buffered_.pkt;
        buffered_.pkt = nullptr;
        buffered_.size = 0;
        buffered_.currentPduSno = 0;
    }
}

bool LteRlcUmRxEntity::isEmpty() const
{
    return buffered_.pkt == nullptr && pduBuffer_.size() == 0;
}

} //namespace
