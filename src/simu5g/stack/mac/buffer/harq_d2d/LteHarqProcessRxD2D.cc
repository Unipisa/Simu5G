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

#include "stack/mac/buffer/harq_d2d/LteHarqProcessRxD2D.h"
#include "stack/mac/LteMacBase.h"
#include "stack/mac/LteMacEnb.h"
#include "common/LteControlInfo.h"
#include "stack/mac/packet/LteHarqFeedback_m.h"
#include "stack/mac/packet/LteMacPdu.h"

namespace simu5g {

using namespace omnetpp;

LteHarqProcessRxD2D::LteHarqProcessRxD2D(unsigned char acid, LteMacBase *owner, Binder *binder)
    : LteHarqProcessRx(acid, owner, binder)
{
}


Packet *LteHarqProcessRxD2D::createFeedback(Codeword cw)
{
    if (!isEvaluated(cw))
        throw cRuntimeError("Cannot send feedback for a PDU not in EVALUATING state");

    Packet *pkt = nullptr;

    auto pduInfo = (pdu_.at(cw)->getTag<UserControlInfo>());
    auto pdu = pdu_.at(cw)->peekAtFront<LteMacPdu>();

    // if the PDU belongs to a multicast connection, then do not create feedback
    // (i.e., in all other cases, feedback is created)
    if (pduInfo->getDirection() != D2D_MULTI) {
        // TODO: Change LteHarqFeedback from chunk to tag,
        pkt = new Packet();
        auto fb = makeShared<LteHarqFeedback>();
        fb->setAcid(acid_);
        fb->setCw(cw);
        fb->setResult(result_.at(cw));
        fb->setFbMacPduId(pdu->getMacPduId());
        //fb->setByteLength(0);
        fb->setChunkLength(b(1));
        pkt->addTagIfAbsent<UserControlInfo>()->setSourceId(pduInfo->getDestId());
        pkt->addTagIfAbsent<UserControlInfo>()->setDestId(pduInfo->getSourceId());
        pkt->addTagIfAbsent<UserControlInfo>()->setFrameType(HARQPKT);
        pkt->addTagIfAbsent<UserControlInfo>()->setDirection(pduInfo->getDirection());
        pkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(pduInfo->getCarrierFrequency());

        pkt->insertAtFront(fb);
    }

    if (!result_.at(cw)) {
        if (pduInfo->getDirection() == D2D_MULTI) {
            // if the PDU belongs to a multicast/broadcast connection, then reset the codeword, since there will be no retransmission
            EV << NOW << " LteHarqProcessRxD2D::createFeedback - PDU for CW " << cw << " belonged to a multicast/broadcast connection. Resetting CW " << endl;
            delete pdu_.at(cw);
            pdu_.at(cw) = nullptr;
            resetCodeword(cw);
        }
        else {
            // NACK will be sent
            status_.at(cw) = RXHARQ_PDU_CORRUPTED;

            EV << "LteHarqProcessRx::createFeedback - TX number " << (unsigned int)transmissions_ << endl;
            if (transmissions_ == (maxHarqRtx_ + 1)) {
                EV << NOW << " LteHarqProcessRxD2D::createFeedback - max number of TX reached for CW " << cw << ". Resetting CW" << endl;

                // purge PDU
                purgeCorruptedPdu(cw);
                resetCodeword(cw);
            }
            else {
                if (macOwner_->getNodeType() == ENODEB || macOwner_->getNodeType() == GNODEB) {
                    // signal the MAC the need for retransmission
                    check_and_cast<LteMacEnb *>(macOwner_.get())->signalProcessForRtx(pduInfo->getSourceId(), pduInfo->getCarrierFrequency(), (Direction)pduInfo->getDirection());
                }
            }
        }
    }
    else {
        status_.at(cw) = RXHARQ_PDU_CORRECT;
    }

    return pkt;
}

Packet *LteHarqProcessRxD2D::createFeedbackMirror(Codeword cw)
{
    if (!isEvaluated(cw))
        throw cRuntimeError("Cannot send feedback for a PDU not in EVALUATING state");

    auto pduInfo = pdu_.at(cw)->getTag<UserControlInfo>();
    auto pdu = pdu_.at(cw)->peekAtFront<LteMacPdu>();

    Packet *pkt = nullptr;

    // if the PDU belongs to a multicast connection, then do not create feedback
    // (i.e., in all other cases, feedback is created)
    if (pduInfo->getDirection() != D2D_MULTI) {
        // TODO: Change LteHarqFeedbackMirror from chunk to tag,
        pkt = new Packet();
        auto fb = makeShared<LteHarqFeedbackMirror>();
        fb->setAcid(acid_);
        fb->setCw(cw);
        fb->setResult(result_.at(cw));
        fb->setFbMacPduId(pdu->getMacPduId());
        fb->setChunkLength(b(1)); // TODO: should be 0
        fb->setPduLength(pdu->getByteLength());
        fb->setD2dSenderId(pduInfo->getSourceId());
        fb->setD2dReceiverId(pduInfo->getDestId());

        pkt->insertAtFront(fb);
        pkt->addTagIfAbsent<UserControlInfo>()->setSourceId(pduInfo->getDestId());
        pkt->addTagIfAbsent<UserControlInfo>()->setDestId(macOwner_->getMacCellId());
        pkt->addTagIfAbsent<UserControlInfo>()->setFrameType(HARQPKT);
        pkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(pduInfo->getCarrierFrequency());
    }
    return pkt;
}

} //namespace

