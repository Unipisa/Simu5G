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

#include "stack/mac/buffer/harq_d2d/LteHarqBufferTxD2D.h"

using namespace omnetpp;

LteHarqBufferTxD2D::LteHarqBufferTxD2D(unsigned int numProc, LteMacBase *owner, LteMacBase *dstMac)
{
    numProc_ = numProc;
    macOwner_ = owner;
    nodeId_ = dstMac->getMacNodeId();
    selectedAcid_ = HARQ_NONE;
    processes_ = new std::vector<LteHarqProcessTx *>(numProc);
    numEmptyProc_ = numProc;
    for (unsigned int i = 0; i < numProc_; i++)
    {
        (*processes_)[i] = new LteHarqProcessTxD2D(i, MAX_CODEWORDS, numProc_, macOwner_, dstMac);
    }
}

void LteHarqBufferTxD2D::receiveHarqFeedback(LteHarqFeedback *fbpkt)
{
    EV << "LteHarqBufferTxD2D::receiveHarqFeedback - start" << endl;

    bool result = fbpkt->getResult();
    HarqAcknowledgment harqResult = result ? HARQACK : HARQNACK;
    Codeword cw = fbpkt->getCw();
    unsigned char acid = fbpkt->getAcid();
    long fbPduId = fbpkt->getFbMacPduId(); // id of the pdu that should receive this fb
    long unitPduId = (*processes_)[acid]->getPduId(cw);

    // After handover or a D2D mode switch, the process nay have been dropped. The received feedback must be ignored.
    if ((*processes_)[acid]->isDropped())
    {
        EV << "H-ARQ TX buffer: received pdu for acid " << (int)acid << ". The corresponding unit has been "
        " reset after handover or a D2D mode switch (the contained pdu was dropped). Ignore feedback." << endl;
        delete fbpkt;
        return;
    }

    if (fbPduId != unitPduId)
    {
        // fb is not for the pdu in this unit, maybe the addressed one was dropped
        EV << "H-ARQ TX buffer: received pdu for acid " << (int)acid << "Codeword " << cw << " not addressed"
        " to the actually contained pdu (maybe it was dropped)" << endl;
        EV << "Received id: " << fbPduId << endl;
        EV << "PDU id: " << unitPduId << endl;
        // todo: comment endsim after tests
        throw cRuntimeError("H-ARQ TX: fb is not for the pdu in this unit, maybe the addressed one was dropped");
    }
    bool reset = (*processes_)[acid]->pduFeedback(harqResult, cw);
    if (reset)
    {
        numEmptyProc_++;
    }

    // debug output
    const char *ack = result ? "ACK" : "NACK";
    EV << "H-ARQ TX: feedback received for process " << (int)acid << " codeword " << (int)cw << ""
    " result is " << ack << endl;
    delete fbpkt;
}

LteHarqBufferTxD2D::~LteHarqBufferTxD2D()
{
}
