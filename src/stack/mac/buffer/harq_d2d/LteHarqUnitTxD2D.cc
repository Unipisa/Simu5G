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

#include "stack/mac/buffer/harq_d2d/LteHarqUnitTxD2D.h"
#include "stack/mac/LteMacEnb.h"

namespace simu5g {

using namespace omnetpp;

simsignal_t LteHarqUnitTxD2D::macCellPacketLossD2DSignal_ = cComponent::registerSignal("macCellPacketLossD2D");
simsignal_t LteHarqUnitTxD2D::macPacketLossD2DSignal_ = cComponent::registerSignal("macPacketLossD2D");
simsignal_t LteHarqUnitTxD2D::harqErrorRateD2DSignal_ = cComponent::registerSignal("harqErrorRateD2D");
simsignal_t LteHarqUnitTxD2D::harqErrorRateD2D_1Signal_ = cComponent::registerSignal("harqErrorRate_1st_D2D");
simsignal_t LteHarqUnitTxD2D::harqErrorRateD2D_2Signal_ = cComponent::registerSignal("harqErrorRate_2nd_D2D");
simsignal_t LteHarqUnitTxD2D::harqErrorRateD2D_3Signal_ = cComponent::registerSignal("harqErrorRate_3rd_D2D");
simsignal_t LteHarqUnitTxD2D::harqErrorRateD2D_4Signal_ = cComponent::registerSignal("harqErrorRate_4th_D2D");

LteHarqUnitTxD2D::LteHarqUnitTxD2D(Binder *binder, unsigned char acid, Codeword cw, LteMacBase *macOwner, LteMacBase *dstMac)
    : LteHarqUnitTx(binder, acid, cw, macOwner, dstMac)
{
    check_and_cast<LteMacEnbD2D *>(nodeB_.get());
    check_and_cast<LteMacUeD2D *>(macOwner_.get());
}


bool LteHarqUnitTxD2D::pduFeedback(HarqAcknowledgment a)
{
    EV << "LteHarqUnitTxD2D::pduFeedback - Welcome!" << endl;
    double sample;
    bool reset = false;
    auto lteInfo = (pdu_->getTag<UserControlInfo>());
    short unsigned int dir = lteInfo->getDirection();
    unsigned int ntx = transmissions_;
    if (!(status_ == TXHARQ_PDU_WAITING))
        throw cRuntimeError("Feedback sent to an H-ARQ unit not waiting for it");

    if (a == HARQACK) {
        // pdu_ has been sent and received correctly
        EV << "\t pdu_ has been sent and received correctly " << endl;
        removeAllSimu5GTags(pdu_);
        resetUnit();
        reset = true;
        sample = 0;
    }
    else if (a == HARQNACK) {
        sample = 1;
        if (transmissions_ == (maxHarqRtx_ + 1)) {
            // discard
            EV << NOW << " LteHarqUnitTxD2D::pduFeedback H-ARQ process " << (unsigned int)acid_ << " Codeword " << cw_ << " PDU "
               << pdu_->getId() << " discarded "
                                "(max retransmissions reached): " << maxHarqRtx_ << endl;
            removeAllSimu5GTags(pdu_);
            resetUnit();
            reset = true;
        }
        else {
            // pdu_ ready for next transmission
            macOwner_->takeObj(pdu_);
            status_ = TXHARQ_PDU_BUFFERED;
            EV << NOW << " LteHarqUnitTxD2D::pduFeedback H-ARQ process " << (unsigned int)acid_ << " Codeword " << cw_ << " PDU "
               << pdu_->getId() << " set for RTX " << endl;
        }
    }
    else {
        throw cRuntimeError("LteHarqUnitTxD2D::pduFeedback unknown feedback received from process %d, Codeword %d", acid_, cw_);
    }

    LteMacBase *ue;
    if (dir == DL || dir == D2D || dir == D2D_MULTI) {
        ue = dstMac_;
    }
    else if (dir == UL) {
        ue = macOwner_;
    }
    else {
        throw cRuntimeError("LteHarqUnitTxD2D::pduFeedback(): unknown direction");
    }

    // emit H-ARQ statistics
    if (dir == D2D || dir == D2D_MULTI) {
        switch (ntx) {
            case 1:
                check_and_cast<LteMacUeD2D *>(ue)->emit(harqErrorRateD2D_1Signal_, sample);
                break;
            case 2:
                check_and_cast<LteMacUeD2D *>(ue)->emit(harqErrorRateD2D_2Signal_, sample);
                break;
            case 3:
                check_and_cast<LteMacUeD2D *>(ue)->emit(harqErrorRateD2D_3Signal_, sample);
                break;
            case 4:
                check_and_cast<LteMacUeD2D *>(ue)->emit(harqErrorRateD2D_4Signal_, sample);
                break;
            default:
                break;
        }

        check_and_cast<LteMacUeD2D *>(ue)->emit(harqErrorRateD2DSignal_, sample);

        if (reset || dir == D2D_MULTI) {
            check_and_cast<LteMacUeD2D *>(ue)->emit(macPacketLossD2DSignal_, sample);
            check_and_cast<LteMacEnbD2D *>(nodeB_.get())->emit(macCellPacketLossD2DSignal_, sample);
        }
    }
    else {
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

        if (a == HARQACK)
            ue->emit(harqTxAttemptsSignal_[dir_], ntx);

        if (reset) {
            ue->emit(macPacketLossSignal_[dir_], sample);
            nodeB_->emit(macCellPacketLossSignal_[dir_], sample);
        }
    }

    return reset;
}

Packet *LteHarqUnitTxD2D::extractPdu()
{
    if (!(status_ == TXHARQ_PDU_SELECTED))
        throw cRuntimeError("Trying to extract macPdu from non-selected H-ARQ unit");

    txTime_ = NOW;
    transmissions_++;
    status_ = TXHARQ_PDU_WAITING; // waiting for feedback
    auto lteInfo = pdu_->getTagForUpdate<UserControlInfo>();
    lteInfo->setTxNumber(transmissions_);
    lteInfo->setNdi(transmissions_ == 1);
    EV << "LteHarqUnitTxD2D::extractPdu - ndi set to " << (transmissions_ == 1 ? "true" : "false") << endl;

    auto extractedPdu = pdu_->dup();
    if (lteInfo->getDirection() == D2D_MULTI) {
        // for multicast, there is no feedback to wait, so reset the unit.
        EV << NOW << " LteHarqUnitTxD2D::extractPdu - the extracted pdu belongs to a multicast/broadcast connection. "
           << "Since the feedback is not expected, reset the unit. " << endl;
        resetUnit();
    }

    return extractedPdu;
}

} //namespace

