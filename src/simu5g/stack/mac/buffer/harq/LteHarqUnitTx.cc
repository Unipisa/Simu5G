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

#include "stack/mac/buffer/harq/LteHarqUnitTx.h"
#include "stack/mac/LteMacEnb.h"
#include <omnetpp.h>

namespace simu5g {

using namespace omnetpp;

simsignal_t LteHarqUnitTx::macPacketLossSignal_[2] = { cComponent::registerSignal("macPacketLossDl"), cComponent::registerSignal("macPacketLossUl") };
simsignal_t LteHarqUnitTx::macCellPacketLossSignal_[2] = { cComponent::registerSignal("macCellPacketLossDl"), cComponent::registerSignal("macCellPacketLossUl") };
simsignal_t LteHarqUnitTx::harqErrorRateSignal_[2] = { cComponent::registerSignal("harqErrorRateDl"), cComponent::registerSignal("harqErrorRateUl") };
simsignal_t LteHarqUnitTx::harqErrorRate_1Signal_[2] = { cComponent::registerSignal("harqErrorRate_1st_Dl"), cComponent::registerSignal("harqErrorRate_1st_Ul") };
simsignal_t LteHarqUnitTx::harqErrorRate_2Signal_[2] = { cComponent::registerSignal("harqErrorRate_2nd_Dl"), cComponent::registerSignal("harqErrorRate_2nd_Ul") };
simsignal_t LteHarqUnitTx::harqErrorRate_3Signal_[2] = { cComponent::registerSignal("harqErrorRate_3rd_Dl"), cComponent::registerSignal("harqErrorRate_3rd_Ul") };
simsignal_t LteHarqUnitTx::harqErrorRate_4Signal_[2] = { cComponent::registerSignal("harqErrorRate_4th_Dl"), cComponent::registerSignal("harqErrorRate_4th_Ul") };
simsignal_t LteHarqUnitTx::harqTxAttemptsSignal_[2] = { cComponent::registerSignal("harqTxAttemptsDl"), cComponent::registerSignal("harqTxAttemptsUl") };

LteHarqUnitTx::LteHarqUnitTx(Binder *binder, unsigned char acid, Codeword cw,
        LteMacBase *macOwner, LteMacBase *dstMac) :  acid_(acid), cw_(cw),  txTime_(0), macOwner_(macOwner), dstMac_(dstMac), maxHarqRtx_(macOwner->par("maxHarqRtx"))
{

    if (macOwner_->getNodeType() == ENODEB || macOwner_->getNodeType() == GNODEB) {
        nodeB_ = macOwner_;
        dir_ = DL;
    }
    else { // UE
        nodeB_ = getMacByMacNodeId(binder, macOwner_->getMacCellId());
        if (dstMac_ == nodeB_) { // UL
            dir_ = UL;
        }
    }
}

void LteHarqUnitTx::insertPdu(Packet *pkt)
{
    if (!pkt)
        throw cRuntimeError("Trying to insert NULL macPdu pointer in harq unit");

    // cannot insert if the unit is not idle
    if (!this->isEmpty())
        throw cRuntimeError("Trying to insert macPdu in already busy harq unit");

    auto pdu = pkt->removeAtFront<LteMacPdu>();
    pdu_ = pkt;
    pduId_ = pdu->getId();
    // as unique MacPDUId the OMNET id is used (need separate member since it must not be changed by dup())
    pdu->setMacPduId(pdu->getId());
    pkt->insertAtFront(pdu);
    transmissions_ = 0;
    status_ = TXHARQ_PDU_SELECTED;
    pduLength_ = pdu_->getByteLength();
    auto lteInfo = pdu_->getTagForUpdate<UserControlInfo>();

    lteInfo->setAcid(acid_);
    Codeword cw_old = lteInfo->getCw();
    if (cw_ != cw_old)
        throw cRuntimeError("mismatch in cw settings");

    lteInfo->setCw(cw_);
}

void LteHarqUnitTx::markSelected()
{
    EV << NOW << " LteHarqUnitTx::markSelected trying to select buffer "
       << acid_ << " codeword " << cw_ << " for transmission " << endl;

    if (!(this->isReady()))
        throw cRuntimeError("ERROR acid %d codeword %d trying to select for transmission an empty buffer", acid_, cw_);

    status_ = TXHARQ_PDU_SELECTED;
}

Packet *LteHarqUnitTx::extractPdu()
{
    if (!(status_ == TXHARQ_PDU_SELECTED))
        throw cRuntimeError("Trying to extract macPdu from not selected H-ARQ unit");

    txTime_ = NOW;
    transmissions_++;
    status_ = TXHARQ_PDU_WAITING; // waiting for feedback

    auto lteInfo = pdu_->getTagForUpdate<UserControlInfo>();
    lteInfo->setTxNumber(transmissions_);
    lteInfo->setNdi(transmissions_ == 1);
    EV << "LteHarqUnitTx::extractPdu - ndi set to " << ((transmissions_ == 1) ? "true" : "false") << endl;

    auto extractedPdu = pdu_->dup();
    macOwner_->takeObj(extractedPdu);
    return extractedPdu;
}

bool LteHarqUnitTx::pduFeedback(HarqAcknowledgment a)
{
    EV << "LteHarqUnitTx::pduFeedback - Welcome!" << endl;
    double sample;
    bool reset = false;
    auto lteInfo = pdu_->getTag<UserControlInfo>();
    short unsigned int dir = lteInfo->getDirection();
    unsigned int ntx = transmissions_;
    if (!(status_ == TXHARQ_PDU_WAITING))
        throw cRuntimeError("Feedback sent to an H-ARQ unit not waiting for it");

    if (a == HARQACK) {
        // pdu_ has been sent and received correctly
        EV << "\t pdu_ has been sent and received correctly " << endl;
        resetUnit();
        reset = true;
        sample = 0;
    }
    else if (a == HARQNACK) {
        sample = 1;
        if (transmissions_ == (maxHarqRtx_ + 1)) {
            // discard
            EV << NOW << " LteHarqUnitTx::pduFeedback H-ARQ process  " << (unsigned int)acid_ << " Codeword " << cw_ << " PDU "
               << pdu_->getId() << " discarded (max retransmissions reached) : " << maxHarqRtx_ << endl;

            // @author Alessandro Noferi
            // notify discard macPduId to packetFlowManager

            macOwner_->discardMacPdu(pdu_);

            resetUnit();
            reset = true;
        }
        else {
            // pdu_ ready for next transmission
            macOwner_->takeObj(pdu_);
            status_ = TXHARQ_PDU_BUFFERED;
            EV << NOW << " LteHarqUnitTx::pduFeedback H-ARQ process  " << (unsigned int)acid_ << " Codeword " << cw_ << " PDU "
               << pdu_->getId() << " set for RTX " << endl;

            if (macOwner_->getNodeType() == ENODEB || macOwner_->getNodeType() == GNODEB) {
                // signal the MAC the need for retransmission
                check_and_cast<LteMacEnb *>(macOwner_.get())->signalProcessForRtx(lteInfo->getDestId(), lteInfo->getCarrierFrequency(), (Direction)lteInfo->getDirection());
            }
        }
    }
    else {
        throw cRuntimeError("LteHarqUnitTx::pduFeedback unknown feedback received from process %d , Codeword %d", acid_, cw_);
    }

    LteMacBase *ue;
    if (dir == DL) {
        ue = dstMac_;
    }
    else if (dir == UL) {
        ue = macOwner_;
    }
    else {
        throw cRuntimeError("LteHarqUnitTx::pduFeedback(): unknown direction");
    }

    // emit H-ARQ statistics
    switch (ntx) {
        case 1:
            ue->emit(harqErrorRate_1Signal_[dir_], sample);
            break;
        case 2:
            ue->emit(harqErrorRate_2Signal_[dir_], sample);
            break;
        case 3:
            ue->emit(harqErrorRate_3Signal_[dir_], sample);
            break;
        case 4:
            ue->emit(harqErrorRate_4Signal_[dir_], sample);
            break;
        default:
            break;
    }

    ue->emit(harqErrorRateSignal_[dir_], sample);

    if (ntx < 4)
        ue->recordHarqErrorRate(sample, (Direction)dir);
    else if (ntx == 4)
        ue->recordHarqErrorRate(0, (Direction)dir);

    if (a == HARQACK)
        ue->emit(harqTxAttemptsSignal_[dir_], ntx);

    if (reset) {
        ue->emit(macPacketLossSignal_[dir_], sample);
        nodeB_->emit(macCellPacketLossSignal_[dir_], sample);
    }

    return reset;
}

bool LteHarqUnitTx::isEmpty()
{
    return status_ == TXHARQ_PDU_EMPTY;
}

bool LteHarqUnitTx::isReady()
{
    return status_ == TXHARQ_PDU_BUFFERED;
}

bool LteHarqUnitTx::selfNack()
{
    if (status_ == TXHARQ_PDU_WAITING)
        // wrong usage, manual nack now is dangerous (a real one may arrive too)
        throw cRuntimeError("LteHarqUnitTx::selfNack(): Trying to send self NACK to a unit waiting for feedback");

    if (status_ != TXHARQ_PDU_BUFFERED)
        throw cRuntimeError("LteHarqUnitTx::selfNack(): Trying to send self NACK to an idle or selected unit");

    transmissions_++;
    txTime_ = NOW;
    bool result = this->pduFeedback(HARQNACK);
    return result;
}

void LteHarqUnitTx::dropPdu()
{
    if (status_ != TXHARQ_PDU_BUFFERED)
        throw cRuntimeError("LteHarqUnitTx::dropPdu(): H-ARQ TX unit: cannot drop pdu if state is not BUFFERED");

    resetUnit();
}

void LteHarqUnitTx::forceDropUnit()
{
    if (status_ == TXHARQ_PDU_BUFFERED || status_ == TXHARQ_PDU_SELECTED || status_ == TXHARQ_PDU_WAITING) {
        delete pdu_;
        pdu_ = nullptr;
    }
    resetUnit();
}

Packet *LteHarqUnitTx::getPdu()
{
    auto temp = pdu_->peekAtFront<LteMacPdu>();
    return pdu_;
}

LteHarqUnitTx::~LteHarqUnitTx()
{
    resetUnit();
}

void LteHarqUnitTx::resetUnit()
{
    transmissions_ = 0;
    pduId_ = -1;
    if (pdu_ != nullptr) {
        delete pdu_;
        pdu_ = nullptr;
    }

    status_ = TXHARQ_PDU_EMPTY;
    pduLength_ = 0;
}

} //namespace

