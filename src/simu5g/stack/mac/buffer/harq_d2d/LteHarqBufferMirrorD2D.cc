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

#include "stack/mac/buffer/harq_d2d/LteHarqBufferMirrorD2D.h"
#include "common/LteControlInfo.h"

namespace simu5g {

using namespace omnetpp;

LteHarqBufferMirrorD2D::LteHarqBufferMirrorD2D(unsigned int numProc, unsigned char maxHarqRtx, LteMacEnb *macOwner) : numProc_(numProc), maxHarqRtx_(maxHarqRtx)
{
    processes_.resize(numProc_, nullptr);
    for (unsigned int i = 0; i < numProc_; i++)
        processes_[i] = new LteHarqProcessMirrorD2D(MAX_CODEWORDS, maxHarqRtx_, macOwner);
}

void LteHarqBufferMirrorD2D::receiveHarqFeedback(inet::Packet *pkt)
{
    EV << "LteHarqBufferMirrorD2D::receiveHarqFeedback - start" << endl;

    auto fbpkt = pkt->peekAtFront<LteHarqFeedbackMirror>();
    bool result = fbpkt->getResult();
    HarqAcknowledgment harqResult = result ? HARQACK : HARQNACK;
    Codeword cw = fbpkt->getCw();
    unsigned char acid = fbpkt->getAcid();
    unsigned int pduLength = fbpkt->getPduLength();
    MacNodeId d2dSenderId = fbpkt->getD2dSenderId();
    double carrierFrequency = pkt->getTag<UserControlInfo>()->getCarrierFrequency();
    processes_[acid]->storeFeedback(harqResult, pduLength, d2dSenderId, carrierFrequency, cw);

    // debug output
    const char *ack = result ? "ACK" : "NACK";
    EV << "H-ARQ MIRROR: feedback received for process " << (int)acid << " codeword " << (int)cw << " result is " << ack << endl;
    delete pkt;
}

void LteHarqBufferMirrorD2D::markSelectedAsWaiting()
{
    for (unsigned int i = 0; i < numProc_; i++) {
        std::vector<TxHarqPduStatus> status = processes_[i]->getProcessStatus();
        for (unsigned int cw = 0; cw < status.size(); cw++) {
            if (status[cw] == TXHARQ_PDU_SELECTED)
                processes_[i]->markWaiting(cw);
        }
    }
}


} //namespace

