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

#include "stack/rlc/um/LteRlcUm.h"
#include "stack/rlc/um//UmRxEntity.h"
#include "stack/mac/LteMacBase.h"
#include "stack/mac/LteMacEnb.h"

namespace simu5g {

Define_Module(UmRxEntity);

using namespace inet;

unsigned int UmRxEntity::totalCellPduRcvdBytes_ = 0;
unsigned int UmRxEntity::totalCellRcvdBytes_ = 0;

simsignal_t UmRxEntity::rlcPacketLossTotalSignal_ = registerSignal("rlcPacketLossTotal");

simsignal_t UmRxEntity::rlcCellPacketLossSignal_[2] = { cComponent::registerSignal("rlcCellPacketLossDl"), cComponent::registerSignal("rlcCellPacketLossUl") };
simsignal_t UmRxEntity::rlcPacketLossSignal_[2] = { cComponent::registerSignal("rlcPacketLossDl"), cComponent::registerSignal("rlcPacketLossUl") };
simsignal_t UmRxEntity::rlcPduPacketLossSignal_[2] = { cComponent::registerSignal("rlcPduPacketLossDl"), cComponent::registerSignal("rlcPduPacketLossUl") };
simsignal_t UmRxEntity::rlcDelaySignal_[2] = { cComponent::registerSignal("rlcDelayDl"), cComponent::registerSignal("rlcDelayUl") };
simsignal_t UmRxEntity::rlcThroughputSignal_[2] = { cComponent::registerSignal("rlcThroughputDl"), cComponent::registerSignal("rlcThroughputUl") };
simsignal_t UmRxEntity::rlcPduDelaySignal_[2] = { cComponent::registerSignal("rlcPduDelayDl"), cComponent::registerSignal("rlcPduDelayUl") };
simsignal_t UmRxEntity::rlcPduThroughputSignal_[2] = { cComponent::registerSignal("rlcPduThroughputDl"), cComponent::registerSignal("rlcPduThroughputUl") };
simsignal_t UmRxEntity::rlcCellThroughputSignal_[2] = { cComponent::registerSignal("rlcCellThroughputDl"), cComponent::registerSignal("rlcCellThroughputUl") };

simsignal_t UmRxEntity::rlcPacketLossD2DSignal_ = registerSignal("rlcPacketLossD2D");
simsignal_t UmRxEntity::rlcPduPacketLossD2DSignal_ = registerSignal("rlcPduPacketLossD2D");
simsignal_t UmRxEntity::rlcDelayD2DSignal_ = registerSignal("rlcDelayD2D");
simsignal_t UmRxEntity::rlcThroughputD2DSignal_ = registerSignal("rlcThroughputD2D");
simsignal_t UmRxEntity::rlcPduDelayD2DSignal_ = registerSignal("rlcPduDelayD2D");
simsignal_t UmRxEntity::rlcPduThroughputD2DSignal_ = registerSignal("rlcPduThroughputD2D");

UmRxEntity::UmRxEntity() :
    t_reordering_(this)
{
    t_reordering_.setTimerId(REORDERING_T);
    buffered_.pkt = nullptr;
    buffered_.size = 0;

    // @author Alessandro Noferi
//    ttiT2_ = 0;
//    ttiT1_ = 0;
//    lastTTI_ = 0;
}

UmRxEntity::~UmRxEntity()
{
    Enter_Method("~UmRxEntity");
    delete buffered_.pkt;
    delete flowControlInfo_;
}

void UmRxEntity::enque(cPacket *pktAux)
{
    Enter_Method("enque()");
    take(pktAux);

    EV << NOW << " UmRxEntity::enque - buffering new PDU" << endl;

    auto pktPdu = check_and_cast<Packet *>(pktAux);
    auto pdu = pktPdu->peekAtFront<LteRlcUmDataPdu>();
    auto lteInfo = pktPdu->getTag<FlowControlInfo>();

    // Get the RLC PDU Transmission sequence number (x)
    unsigned int tsn = pdu->getPduSequenceNumber();

    if (!init_ && isD2DMultiConnection()) {
        // for D2D multicast connections, the first received PDU must be considered as the first valid PDU
        rxWindowDesc_.clear(tsn);
        // setting the window size to 1 allows the entity to deliver immediately out-of-sequence SDU,
        // since reordering is not applicable for D2D multicast communications
        rxWindowDesc_.windowSize_ = 1;
        init_ = true;
    }

    // get the position in the buffer
    int index = tsn - rxWindowDesc_.firstSno_;

    EV << NOW << " UmRxEntity::enque - tsn " << tsn << ", the corresponding index in the buffer is " << index << endl;

    // x was already received
    if (tsn >= rxWindowDesc_.firstSnoForReordering_ && tsn < rxWindowDesc_.highestReceivedSno_ && received_.at(index) == true) {
        EV << NOW << " UmRxEntity::enque the received PDU has index " << index << " which points to an already busy location. Discard the PDU" << endl;

        // TODO
        // Check if the received PDU points
        // to the same data structure as the PDU
        // stored in the buffer

        delete pktPdu;

        return;
    }

    // x was already considered for reordering & reassembling
    if (tsn < rxWindowDesc_.firstSnoForReordering_) {
        EV << NOW << " UmRxEntity::enque the received PDU with " << tsn << " SN was already considered for reordering. Discard the PDU" << endl;
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
    MacNodeId ueId;
    if (lteInfo->getDirection() == DL || lteInfo->getDirection() == D2D || lteInfo->getDirection() == D2D_MULTI)                                                                                                                    // This module is at a UE
        ueId = ownerNodeId_;
    else           // UL. This module is at the eNB: get the node id of the sender
        ueId = lteInfo->getSourceId();

    double tputSample = (double)totalPduRcvdBytes_ / (NOW - getSimulation()->getWarmupPeriod());

    // emit statistics
    cModule *ue = getRlcByMacNodeId(binder_, ueId, UM);
    if (ue != nullptr) {
        if (lteInfo->getDirection() != D2D && lteInfo->getDirection() != D2D_MULTI) { // UE in IM
            ue->emit(rlcPduThroughputSignal_[dir_], tputSample);
            ue->emit(rlcPduDelaySignal_[dir_], (NOW - pktPdu->getCreationTime()).dbl());
        }
        else { // UE in DM
            ue->emit(rlcPduThroughputD2DSignal_, tputSample);
            ue->emit(rlcPduDelayD2DSignal_, (NOW - pktPdu->getCreationTime()).dbl());
        }
    }

    EV << NOW << " UmRxEntity::enque - tsn " << tsn << ", the corresponding index after shift in the buffer is " << index << endl;
    EV << NOW << " UmRxEntity::enque - firstSnoReordering " << rxWindowDesc_.firstSnoForReordering_ << endl;

    index = rxWindowDesc_.firstSnoForReordering_ - rxWindowDesc_.firstSno_; //

    // D
    if (received_.at(rxWindowDesc_.firstSnoForReordering_ - rxWindowDesc_.firstSno_) == true) {
        unsigned int old = rxWindowDesc_.firstSnoForReordering_;

        index = rxWindowDesc_.firstSnoForReordering_ - rxWindowDesc_.firstSno_; //

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

    /* @author Alessandro Noferi
     *
     * At the end of each enque, the state of the buffer is checked.
     * A burst end does not mean that actually a burst has occurred,
     * it has occurred if t1 - t2 > TTI
     */

    if (flowControlInfo_->getDirection() == UL) { //only eNodeB checks the burst
        handleBurst(ENQUE);
    }
}

void UmRxEntity::moveRxWindow(int pos)
{
    EV << NOW << " UmRxEntity::moveRxWindow moving forth by " << pos << " locations" << endl;

    if (pos <= 0)
        return;                   // ignore the shift, it is ineffective.

    if (pos > rxWindowDesc_.windowSize_)
        throw cRuntimeError("AmRxQueue::moveRxWindow(): positions %d win size %d ", pos, rxWindowDesc_.windowSize_);

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

    EV << NOW << " UmRxEntity::moveRxWindow first sequence number updated to " << rxWindowDesc_.firstSno_ << endl;
}

//void UmRxEntity::toPdcp(LteRlcSdu* rlcSdu)
void UmRxEntity::toPdcp(Packet *pktAux)
{

    auto rlcSdu = pktAux->popAtFront<LteRlcSdu>();
    LteRlcUm *lteRlc = this->rlc_;

    auto lteInfo = pktAux->getTag<FlowControlInfo>();
    unsigned int sno = rlcSdu->getSnoMainPacket();
    unsigned int length = pktAux->getByteLength();
    simtime_t ts = pktAux->getCreationTime();

    // create a PDCP PDU and send it to the upper layer
    MacNodeId ueId;
    if (lteInfo->getDirection() == DL || lteInfo->getDirection() == D2D || lteInfo->getDirection() == D2D_MULTI)                                                                                                                    // This module is at a UE
        ueId = ownerNodeId_;
    else           // UL. This module is at the eNB: get the node id of the sender
        ueId = lteInfo->getSourceId();

    cModule *ue = getRlcByMacNodeId(binder_, ueId, UM);
    // check whether some PDCP PDUs have not been delivered
    while (sno > lastSnoDelivered_ + 1) {
        lastSnoDelivered_++;

        if (nodeB_ != nullptr)
            nodeB_->emit(rlcCellPacketLossSignal_[dir_], 1.0);

        // emit statistic: packet loss
        if (ue != nullptr) {
            if (lteInfo->getDirection() != D2D && lteInfo->getDirection() != D2D_MULTI)
                ue->emit(rlcPacketLossSignal_[dir_], 1.0);
            else
                ue->emit(rlcPacketLossD2DSignal_, 1.0);
            ue->emit(rlcPacketLossTotalSignal_, 1.0);
        }
    }
    // update the last sno delivered to the current sno
    lastSnoDelivered_ = sno;

    // emit statistics

    totalCellRcvdBytes_ += length;
    totalRcvdBytes_ += length;
    double cellTputSample = (double)totalCellRcvdBytes_ / (NOW - getSimulation()->getWarmupPeriod());
    double tputSample = (double)totalRcvdBytes_ / (NOW - getSimulation()->getWarmupPeriod());
    if (ue != nullptr) {
        if (lteInfo->getDirection() != D2D && lteInfo->getDirection() != D2D_MULTI) { // UE in IM
            ue->emit(rlcThroughputSignal_[dir_], tputSample);
            ue->emit(rlcPacketLossSignal_[dir_], 0.0);
            ue->emit(rlcPacketLossTotalSignal_, 0.0);
            ue->emit(rlcDelaySignal_[dir_], (NOW - ts).dbl());
        }
        else {
            ue->emit(rlcThroughputD2DSignal_, tputSample);
            ue->emit(rlcPacketLossD2DSignal_, 0.0);
            ue->emit(rlcPacketLossTotalSignal_, 0.0);
            ue->emit(rlcDelayD2DSignal_, (NOW - ts).dbl());
        }
    }
    if (nodeB_ != nullptr) {
        nodeB_->emit(rlcCellThroughputSignal_[dir_], cellTputSample);
        nodeB_->emit(rlcCellPacketLossSignal_[dir_], 0.0);
    }

    EV << NOW << " UmRxEntity::toPdcp Created PDCP PDU with length " << pktAux->getByteLength() << " bytes" << endl;
    EV << NOW << " UmRxEntity::toPdcp Send packet to upper layer" << endl;

    lteRlc->sendDefragmented(pktAux);
}

void UmRxEntity::reassemble(unsigned int index)
{
    Enter_Method("reassemble()");

    if (received_.at(index) == false) {
        // consider the case when a PDU is missing or already delivered
        EV << NOW << " UmRxEntity::reassemble PDU at index " << index << " has not been received or already delivered" << endl;
        return;
    }

    EV << NOW << " UmRxEntity::reassemble Consider PDU at index " << index << " for reassembly" << endl;

    auto pktPdu = check_and_cast<Packet *>(pduBuffer_.get(index));
    auto pdu = pktPdu->removeAtFront<LteRlcUmDataPdu>();
    auto lteInfo = pktPdu->getTag<FlowControlInfo>();

    // get PDU seq number
    unsigned int pduSno = pdu->getPduSequenceNumber();

    if (resetFlag_) {
        // by doing this, the arrived PDU will be considered in order. For example, when D2D is enabled,
        // this helps to retrieve the synchronization between SNs at the tx and rx after a mode switch
        lastPduReassembled_ = pduSno - 1;
    }

    // get framing info
    FramingInfo fi = pdu->getFramingInfo();

    // get the number of (portions of) SDUs in the PDU
    unsigned int numSdu = pdu->getNumSdu();

    // for each SDU
    for (unsigned int i = 0; i < numSdu; i++) {
        size_t sduLengthPktLeng;
        auto pktSdu = check_and_cast<Packet *>(pdu->popSdu(sduLengthPktLeng));

        auto rlcSdu = pktSdu->peekAtFront<LteRlcSdu>();
        unsigned int sduSno = rlcSdu->getSnoMainPacket();
        unsigned int sduWholeLength = rlcSdu->getLengthMainPacket(); // the length of the whole sdu

        if (i == 0) { // first SDU
            bool ignoreFragment = false;
            if (resetFlag_) {
                // by doing this, the first extracted SDU will be considered in order. For example, when D2D is enabled,
                // this helps to retrieve the synchronization between SNs at the tx and rx after a mode switch
                lastSnoDelivered_ = sduSno - 1;
                resetFlag_ = false;
                ignoreFragment = true;
            }

            if (i == numSdu - 1) { // [first SDU, i==0] there is only one SDU in this PDU
                // read the FI field
                switch (fi) {
                    case 0: {  // FI=00
                        EV << NOW << " UmRxEntity::reassemble The PDU includes one whole SDU [sno=" << sduSno << "]" << endl;
                        if (sduLengthPktLeng != sduWholeLength)
                            throw cRuntimeError("UmRxEntity::reassemble(): failed reassembly, the reassembled SDU has size %zu B, while the original SDU had size %d B", sduLengthPktLeng, sduWholeLength);

                        toPdcp(pktSdu);
                        pktSdu = nullptr;

                        clearBufferedSdu();

                        break;
                    }
                    case 1: {  // FI=01
                        EV << NOW << " UmRxEntity::reassemble The PDU includes the first part [" << sduLengthPktLeng << " B] of an SDU [sno=" << sduSno << "]" << endl;

                        clearBufferedSdu();

                        // buffer the SDU and wait for the missing portion
                        buffered_.pkt = pktSdu;
                        pktSdu = nullptr;
                        buffered_.size = sduLengthPktLeng;
                        buffered_.currentPduSno = pduSno;

                        // for burst
                        ttiBits_ += sduLengthPktLeng;
                        EV << NOW << " UmRxEntity::reassemble Wait for the missing part..." << endl;

                        break;
                    }
                    case 2: {  // FI=10
                        // it is the last portion of an SDU, take the awaiting SDU
                        EV << NOW << " UmRxEntity::reassemble The PDU includes the last part [" << sduLengthPktLeng << " B] of an SDU [sno=" << sduSno << "]" << endl;

                        // check SDU SN
                        if (buffered_.pkt == nullptr ||
                            (rlcSdu->getSnoMainPacket() != buffered_.pkt->peekAtFront<LteRlcSdu>()->getSnoMainPacket()) ||
                            (pduSno != (buffered_.currentPduSno + 1)) ||  // first and only SDU in PDU. PduSno must be last+1, otherwise drop SDU.
                            ignoreFragment)
                        {
                            if (pduSno != buffered_.currentPduSno) {
                                EV << NOW << " UmRxEntity::reassemble The SDU cannot be reassembled, Pdu sequence numbers are not in sequence" << endl;
                            }
                            else {
                                EV << NOW << " UmRxEntity::reassemble The SDU cannot be reassembled, first part missing" << endl;
                            }
                            clearBufferedSdu();

                            //delete rlcSdu;
                            delete pktSdu;
                            pktSdu = nullptr;
                            continue;
                        }

                        EV << NOW << " UmRxEntity::reassemble The waiting SDU has size " << buffered_.size << " bytes" << endl;

                        unsigned int reassembledLength = buffered_.size + sduLengthPktLeng;
                        if (reassembledLength < sduWholeLength) {
                            clearBufferedSdu();
                            EV << NOW << " UmRxEntity::reassemble The SDU cannot be reassembled, mid part missing" << endl;

                            //delete rlcSdu;
                            delete pktSdu;
                            pktSdu = nullptr;
                            continue;
                        }
                        else if (reassembledLength > sduWholeLength) {
                            throw cRuntimeError("UmRxEntity::reassemble(): failed reassembly, the reassembled SDU has size %d B, while the original SDU had size %d B", reassembledLength, sduWholeLength);
                        }

                        // for burst
                        ttiBits_ += sduLengthPktLeng;
                        toPdcp(pktSdu);
                        pktSdu = nullptr;

                        clearBufferedSdu();

                        break;
                    }
                    case 3: {  // FI=11
                        // add the length of this SDU to the awaiting SDU and wait for the missing portion
                        EV << NOW << " UmRxEntity::reassemble The PDU includes the mid part [" << sduLengthPktLeng << " B] of an SDU [sno=" << sduSno << "]" << endl;

                        // check SDU SN
                        if (buffered_.pkt == nullptr ||
                            (rlcSdu->getSnoMainPacket() != buffered_.pkt->peekAtFront<LteRlcSdu>()->getSnoMainPacket()) ||
                            (pduSno != (buffered_.currentPduSno + 1)) ||  // first SDU but NOT only in PDU. PduSno must be last+1, otherwise drop SDU.
                            ignoreFragment)
                        {
                            if (pduSno != buffered_.currentPduSno) {
                                EV << NOW << " UmRxEntity::reassemble The SDU cannot be reassembled, Pdu sequence numbers are not in sequence" << endl;
                            }
                            else {
                                EV << NOW << " UmRxEntity::reassemble The SDU cannot be reassembled, first part missing" << endl;
                            }
                            clearBufferedSdu();

                            delete pktSdu;
                            pktSdu = nullptr;
                            continue;
                        }

                        // buffered_->setByteLength(buffered_->getByteLength() + rlcSdu->getByteLength());

                        // for burst
                        ttiBits_ += sduLengthPktLeng;
                        buffered_.size += sduLengthPktLeng;
                        buffered_.currentPduSno = pduSno;
                        delete pktSdu;
                        pktSdu = nullptr;

                        EV << NOW << " UmRxEntity::reassemble The waiting SDU has size " << buffered_.size << " bytes, was " << buffered_.size - sduLengthPktLeng << " bytes" << endl;
                        EV << NOW << " UmRxEntity::reassemble Wait for the missing part..." << endl;

                        break;
                    }
                    default: {
                        throw cRuntimeError("UmRxEntity::reassemble(): FI field was not valid %d ", fi);
                    }
                }
            }
            else { // [first SDU, i==0] there is more than one SDU in this PDU
                EV << NOW << " UmRxEntity::reassemble Read the first chunk of the PDU" << endl;

                // read the FI field
                switch (fi) {

                    case 0: case 1: {  // FI=00 or FI=01
                        // it is a whole SDU, send the SDU to the PDCP

                        EV << NOW << " UmRxEntity::reassemble This is a whole SDU [sno=" << sduSno << "]" << endl;
                        if (sduLengthPktLeng != sduWholeLength)
                            throw cRuntimeError("UmRxEntity::reassemble(): failed reassembly, the reassembled SDU has size %zu B, while the original SDU had size %d B", sduLengthPktLeng, sduWholeLength);

                        // for burst
                        ttiBits_ += sduLengthPktLeng;
                        toPdcp(pktSdu);
                        pktSdu = nullptr;

                        clearBufferedSdu();

                        break;
                    }
                    case 2: case 3: {  // FI=10 or FI=11
                        // it is the last portion of an SDU, take the awaiting SDU and send to the PDCP
                        EV << NOW << " UmRxEntity::reassemble This is the last part [" << sduLengthPktLeng << " B] of an SDU [sno=" << sduSno << "]" << endl;

                        // check SDU SN
                        if (buffered_.pkt == nullptr ||
                            (rlcSdu->getSnoMainPacket() != buffered_.pkt->peekAtFront<LteRlcSdu>()->getSnoMainPacket()) ||
                            (pduSno != (buffered_.currentPduSno + 1)) ||  // first SDU but NOT only in PDU. PduSno must be last+1, otherwise drop SDU.
                            ignoreFragment)
                        {
                            if (pduSno != (buffered_.currentPduSno + 1)) {
                                EV << NOW << " UmRxEntity::reassemble The SDU cannot be reassembled, Pdu sequence numbers are not in sequence" << endl;
                            }
                            else {
                                EV << NOW << " UmRxEntity::reassemble The SDU cannot be reassembled, first part missing" << endl;
                            }
                            clearBufferedSdu();
                            EV << NOW << " UmRxEntity::reassemble The SDU cannot be reassembled, first part missing" << endl;

                            delete pktSdu;
                            pktSdu = nullptr;

                            continue;
                        }

                        EV << NOW << " UmRxEntity::reassemble The waiting SDU has size " << buffered_.size << " bytes" << endl;

                        unsigned int reassembledLength = buffered_.size + sduLengthPktLeng;
                        if (reassembledLength < sduWholeLength) {
                            clearBufferedSdu();
                            EV << NOW << " UmRxEntity::reassemble The SDU cannot be reassembled, mid part missing" << endl;

                            delete pktSdu;

                            continue;
                        }
                        else if (reassembledLength > sduWholeLength) {
                            throw cRuntimeError("UmRxEntity::reassemble(): failed reassembly, the reassembled SDU has size %d B, while the original SDU had size %d B", reassembledLength, sduWholeLength);
                        }

                        // for burst
                        ttiBits_ += sduWholeLength; // remove the discarded SDU size from the throughput
                        toPdcp(pktSdu);
                        pktSdu = nullptr;

                        clearBufferedSdu();

                        break;
                    }
                    default: {
                        throw cRuntimeError("UmRxEntity::reassemble(): FI field was not valid %d ", fi);
                    }
                }
            }
        }
        else if (i == numSdu - 1) { // last SDU in PDU with at least 2 SDUs
            // read the FI field
            switch (fi) {
                case 0: case 2: {  // FI=00 or FI=10
                    // it is a whole SDU, send the SDU to the PDCP
                    EV << NOW << " UmRxEntity::reassemble This is a whole SDU [sno=" << sduSno << "]" << endl;
                    if (sduLengthPktLeng != sduWholeLength)
                        throw cRuntimeError("UmRxEntity::reassemble(): failed reassembly, the reassembled SDU has size %zu B, while the original SDU had size %d B", sduLengthPktLeng, sduWholeLength);

                    // for burst
                    ttiBits_ += sduLengthPktLeng;

                    toPdcp(pktSdu);
                    pktSdu = nullptr;

                    clearBufferedSdu();

                    break;
                }
                case 1: case 3: {  // FI=01 or FI=11
                    // it is the first portion of an SDU, buffer it
                    EV << NOW << " UmRxEntity::reassemble The PDU includes the first part [" << sduLengthPktLeng << " B] of an SDU [sno=" << sduSno << "]" << endl;

                    clearBufferedSdu();

                    // for burst
                    ttiBits_ += sduLengthPktLeng;

                    buffered_.pkt = pktSdu;
                    buffered_.size = sduLengthPktLeng;
                    buffered_.currentPduSno = pduSno;
                    pktSdu = nullptr;

                    EV << NOW << " UmRxEntity::reassemble Wait for the missing part..." << endl;

                    break;
                }
                default: {
                    throw cRuntimeError("UmRxEntity::reassemble(): FI field was not valid %d ", fi);
                }
            }
        }
        else {
            // it is a whole SDU, send to the PDCP
            EV << NOW << " UmRxEntity::reassemble This is a whole SDU [sno=" << sduSno << "]" << endl;
            if (sduLengthPktLeng != sduWholeLength)
                throw cRuntimeError("UmRxEntity::reassemble(): failed reassembly, the reassembled SDU has size %zu B, while the original SDU had size %d B", sduLengthPktLeng, sduWholeLength);

            // for burst
            ttiBits_ += sduLengthPktLeng;
            toPdcp(pktSdu);
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
    EV << NOW << " UmRxEntity::reassemble Removed PDU from position " << index << endl;

    // emit statistics
    MacNodeId ueId;
    if (lteInfo->getDirection() == DL || lteInfo->getDirection() == D2D || lteInfo->getDirection() == D2D_MULTI)                                                                                                                    // This module is at a UE
        ueId = ownerNodeId_;
    else           // UL. This module is at the eNB: get the node id of the sender
        ueId = lteInfo->getSourceId();

    cModule *ue = getRlcByMacNodeId(binder_, ueId, UM);
    // check whether some PDCP PDUs have not been delivered
    while (pduSno > lastPduReassembled_ + 1) {
        // emit statistic: packet loss
        if (lteInfo->getDirection() != D2D && lteInfo->getDirection() != D2D_MULTI) { // UE in IM
            ue->emit(rlcPduPacketLossSignal_[dir_], 1.0);
        }
        else {
            ue->emit(rlcPduPacketLossD2DSignal_, 1.0);
        }

        lastPduReassembled_++;
    }

    // update the last sno reassembled to the current sno
    lastPduReassembled_ = pduSno;

    // emit statistic: packet loss
    if (lteInfo->getDirection() != D2D && lteInfo->getDirection() != D2D_MULTI) { // UE in IM
        ue->emit(rlcPduPacketLossSignal_[dir_], 0.0);
    }
    else {
        ue->emit(rlcPduPacketLossD2DSignal_, 0.0);
    }

    pktPdu->insertAtFront(pdu);

    delete pktPdu;
}

/*
 * Main Functions
 */

void UmRxEntity::initialize()
{
    binder_.reference(this, "binderModule", true);
    timeout_ = par("timeout").doubleValue();
    rxWindowDesc_.clear();
    rxWindowDesc_.windowSize_ = par("rxWindowSize");
    received_.resize(rxWindowDesc_.windowSize_);

    totalRcvdBytes_ = 0;
    totalPduRcvdBytes_ = 0;

    rlc_.reference(this, "umModule", true);

    //statistics

    LteMacBase *mac = getModuleFromPar<LteMacBase>(par("macModule"), this);
    nodeB_ = getRlcByMacNodeId(binder_, mac->getMacCellId(), UM);

    resetFlag_ = false;

    if (mac->getNodeType() == ENODEB || mac->getNodeType() == GNODEB) {
        dir_ = UL;
    }
    else { // UE
        dir_ = DL;
    }

    // store the node id of the owner module (useful for statistics)
    ownerNodeId_ = mac->getMacNodeId();

    WATCH(timeout_);
}

void UmRxEntity::handleMessage(cMessage *msg)
{
    if (msg->isName("timer")) {
        t_reordering_.handle();

        EV << NOW << " UmRxEntity::handleMessage : t_reordering timer has expired " << endl;

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
}

void UmRxEntity::clearBufferedSdu() {
    if (buffered_.pkt != nullptr) {
        // for burst
        ttiBits_ -= buffered_.size; // remove the discarded SDU size from the throughput
        delete buffered_.pkt;
        buffered_.pkt = nullptr;
        buffered_.size = 0;
        buffered_.currentPduSno = 0;
    }
}

void UmRxEntity::rlcHandleD2DModeSwitch(bool oldConnection, bool oldMode, bool clearBuffer)
{
    Enter_Method_Silent("rlcHandleD2DModeSwitch()");

    if (oldConnection) {
        if (getNodeTypeById(ownerNodeId_) == UE && oldMode == IM) {
            EV << NOW << " UmRxEntity::rlcHandleD2DModeSwitch - nothing to do on DL leg of IM flow" << endl;
            return;
        }

        if (clearBuffer) {

            EV << NOW << " UmRxEntity::rlcHandleD2DModeSwitch - clear RX buffer of the RLC entity associated with the old mode" << endl;
            for (unsigned int i = 0; i < rxWindowDesc_.windowSize_; i++) {
                // try to reassemble
                reassemble(i);
            }

            // clear the buffer
            pduBuffer_.clear();

            for (auto && i : received_) {
                i = false;
            }

            clearBufferedSdu();

            // stop the timer
            if (t_reordering_.busy())
                t_reordering_.stop();
        }
    }
    else {
        EV << NOW << " UmRxEntity::rlcHandleD2DModeSwitch - handle numbering of the RLC entity associated with the newly selected mode" << endl;

        // reset sequence numbering
        rxWindowDesc_.clear();

        resetFlag_ = true;
    }
}

void UmRxEntity::handleBurst(BurstCheck event)
{
    /* burst management
     * According to ETSI TS 128 552 v15.00.0 and
     * ETSI 136 314 v15.1.0
     * check buffer size:
     *   - if 0:
     *      if(burst = true) //it is ended
     *           consider only total bytes, last_t1 and t2 ( a burst occurred only if t1-t2> TTI
     *           clear variables
     *           reset isBurst
     *      if burst = false nothing happens
     *          clear temp var
     *  - if 1:
     *      if burst = true //burst is still running
     *          update total var with temp value
     *      if burst = false // new burst activated
     *          burst = 1
     *          update total var with temp var
     */
    EV_FATAL << "UmRxEntity::handleBurst - size: " << pduBuffer_.size() + ((buffered_.pkt == nullptr) ? 0 : 1) << endl;

    simtime_t t1 = simTime();

    if (pduBuffer_.size() + (buffered_.pkt == nullptr ? 0 : 1) == 0) { // last TTI emptied the burst
        if (isBurst_) { // burst ends
            // send stats
            // if the transmission requires two TTIs and I do not count
            // the second last, in the simulator t1 - t2 is 0.

            if ((t1_ - t2_) > TTI) {
                Throughput throughput = { totalBits_, (t1_ - t2_) };
                rlc_->addUeThroughput(flowControlInfo_->getSourceId(), throughput);

                EV_FATAL << "BURST ENDED - size : " << totalBits_ << endl;
                EV_FATAL << "throughput: size " << totalBits_ << " time " << (t1_ - t2_) << endl;
            }
            totalBits_ = 0;
            t2_ = 0;
            t1_ = 0;

            isBurst_ = false;
        }
        else {
            EV_FATAL << "NO BURST - size : " << totalBits_ << endl;
        }
    }
    else {
        if (isBurst_) {
            if (event == ENQUE) { // handleBurst called at the end of the TTI
                totalBits_ += ttiBits_;
                t1_ = t1;
                EV_FATAL << "BURST CONTINUES - size : " << totalBits_ << endl;
            }
        }
        else {
            isBurst_ = true;
            totalBits_ = ttiBits_;
            t2_ = t1;
            t1_ = t1; // it will be updated
            EV_FATAL << "BURST STARTED - size : " << totalBits_ << endl;
        }
    }

    // reset temporary (per TTI) variables
    ttiBits_ = 0;
}

} //namespace

