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

namespace simu5g {

using namespace omnetpp;

LteHarqBufferTxD2D::LteHarqBufferTxD2D(Binder *binder, unsigned int numProc, LteMacBase *owner, LteMacBase *dstMac)
    : LteHarqBufferTx(binder, numProc, owner)
{
    nodeId_ = dstMac->getMacNodeId();
    for (unsigned int i = 0; i < numProc_; i++) {
        processes_[i] = new LteHarqProcessTxD2D(binder, i, MAX_CODEWORDS, numProc_, macOwner_, dstMac);
    }
}

} //namespace

