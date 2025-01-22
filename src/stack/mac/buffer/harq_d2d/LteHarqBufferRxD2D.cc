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

#include "stack/mac/buffer/harq_d2d/LteHarqBufferRxD2D.h"
#include "stack/mac/packet/LteMacPdu.h"
#include "common/LteControlInfo.h"
#include "stack/mac/packet/LteHarqFeedback_m.h"
#include "stack/mac/LteMacBase.h"
#include "stack/mac/LteMacEnb.h"
#include "stack/mac/LteMacUeD2D.h"
#include "stack/mac/LteMacEnbD2D.h"

namespace simu5g {

using namespace omnetpp;

simsignal_t LteHarqBufferRxD2D::macThroughputD2D_ = cComponent::registerSignal("macThroughputD2D");
simsignal_t LteHarqBufferRxD2D::macDelayD2D_ = cComponent::registerSignal("macDelayD2D");
simsignal_t LteHarqBufferRxD2D::macCellThroughputD2D_ = cComponent::registerSignal("macCellThroughputD2D");

LteHarqBufferRxD2D::LteHarqBufferRxD2D(unsigned int num, LteMacBase *owner, Binder *binder, MacNodeId srcId, bool isMulticast)
    : LteHarqBufferRx(binder, owner, num, srcId)
{
    initMacUe();
    isMulticast_ = isMulticast;

    for (unsigned int i = 0; i < numHarqProcesses_; i++) {
        processes_[i] = new LteHarqProcessRxD2D(i, macOwner_, binder);
    }

    // Signals initialization: these are used to gather statistics

    if (macOwner_->getNodeType() == ENODEB || macOwner_->getNodeType() == GNODEB) {
        nodeB_ = macOwner_;
        dir = UL;
    }
    else { // this is a UE
        nodeB_ = getMacByMacNodeId(binder, macOwner_->getMacCellId());
        dir = DL;
    }
}

void LteHarqBufferRxD2D::insertPdu(Codeword cw, Packet *pkt)
{
    auto pdu = pkt->peekAtFront<LteMacPdu>();
    auto uInfo = pkt->getTag<UserControlInfo>();

    MacNodeId srcId = uInfo->getSourceId();
    if (macOwner_->isHarqReset(srcId)) {
        // if the HARQ processes have been aborted during this TTI (e.g. due to a D2D mode switch),
        // incoming packets should not be accepted
        delete pkt;
        return;
    }

    unsigned char acid = uInfo->getAcid();
    // TODO add codeword to insertPdu
    processes_[acid]->insertPdu(cw, pkt);
    // debug output
    EV << "H-ARQ RX: new PDU (id " << pdu->getId() << " ) inserted into process " << (int)acid << endl;
}

void LteHarqBufferRxD2D::sendFeedback()
{
    for (unsigned int i = 0; i < numHarqProcesses_; i++) {
        for (Codeword cw = 0; cw < MAX_CODEWORDS; ++cw) {
            if (processes_[i]->isEvaluated(cw)) {
                // create a copy of the feedback to be sent to the eNB
                auto pkt = check_and_cast<LteHarqProcessRxD2D *>(processes_[i])->createFeedbackMirror(cw);
                if (pkt == nullptr) {
                    EV << NOW << "LteHarqBufferRxD2D::sendFeedback - cw " << cw << " of process " << i
                       << " contains a PDU belonging to a multicast/broadcast connection. Don't send feedback mirror." << endl;
                }
                else {
                    macOwner_->sendLowerPackets(pkt);
                }

                auto pktHbf = (processes_[i])->createFeedback(cw);

                if (pktHbf == nullptr) {
                    EV << NOW << "LteHarqBufferRxD2D::sendFeedback - cw " << cw << " of process " << i
                       << " contains a PDU belonging to a multicast/broadcast connection. Don't send feedback." << endl;
                    continue;
                }

                auto hfb = pktHbf->peekAtFront<LteHarqFeedback>();
                // debug output:
                auto cInfo = pktHbf->getTag<UserControlInfo>();
                const char *r = hfb->getResult() ? "ACK" : "NACK";
                EV << "H-ARQ RX: feedback sent to TX process "
                   << (int)hfb->getAcid() << " Codeword  " << (int)cw
                   << " of node with id "
                   << cInfo->getDestId()
                   << " result: " << r << endl;

                macOwner_->sendLowerPackets(pktHbf);
            }
        }
    }
}

std::list<Packet *> LteHarqBufferRxD2D::extractCorrectPdus()
{
    this->sendFeedback();
    std::list<Packet *> ret;
    unsigned char acid = 0;
    for (unsigned int i = 0; i < numHarqProcesses_; i++) {
        for (Codeword cw = 0; cw < MAX_CODEWORDS; ++cw) {
            if (processes_[i]->isCorrect(cw)) {
                auto temp = processes_[i]->extractPdu(cw);
                unsigned int size = temp->getByteLength();
                auto info = temp->getTag<UserControlInfo>();

                // emit delay statistic
                if (info->getDirection() == D2D)
                    macUe_emit(macDelayD2D_, (NOW - temp->getCreationTime()).dbl());
                else
                    macUe_emit(macDelaySignal_[dir], (NOW - temp->getCreationTime()).dbl()); // TODO `info->getDirection()` and `dir` maybe differs

                // Calculate Throughput by sending the number of bits for this packet
                totalRcvdBytes_ += size;
                totalCellRcvdBytes_ += size;

                double den = (NOW - getSimulation()->getWarmupPeriod()).dbl();

                if (den > 0) {
                    double tputSample = (double)totalRcvdBytes_ / den;
                    double cellTputSample = (double)totalCellRcvdBytes_ / den;

                    // emit throughput statistics
                    if (info->getDirection() == D2D) {
                        check_and_cast<LteMacEnbD2D *>(nodeB_.get())->emit(macCellThroughputD2D_, cellTputSample);
                        macUe_emit(macThroughputD2D_, tputSample);
                    }
                    else {
                        nodeB_->emit(macCellThroughputSignal_[dir], cellTputSample); // TODO `info->getDirection()` and `dir` maybe differs
                        macUe_emit(macThroughputSignal_[dir], tputSample); // TODO `info->getDirection()` and `dir` maybe differs
                    }
                }

                ret.push_back(temp);
                acid = i;

                EV << "LteHarqBufferRxD2D::extractCorrectPdus H-ARQ RX: PDU (id " << ret.back()->getId()
                   << " ) extracted from process " << (int)acid
                   << " to be sent upper" << endl;
            }
        }
    }

    return ret;
}


} //namespace

