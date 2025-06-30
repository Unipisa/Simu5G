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

#include "stack/mac/buffer/harq_d2d/LteHarqProcessTxD2D.h"

namespace simu5g {

using namespace omnetpp;

LteHarqProcessTxD2D::LteHarqProcessTxD2D(Binder *binder, unsigned char acid, unsigned int numUnits, unsigned int numProcesses, LteMacBase *macOwner, LteMacBase *dstMac)
    : LteHarqProcessTx(binder, acid, numUnits, numProcesses, macOwner, dstMac)
{
    macOwner_ = macOwner;
    acid_ = acid;
    numProcesses_ = numProcesses;
    numEmptyUnits_ = numUnits; //++ @ insert, -- @ unit reset (ack or fourth nack)
    numSelected_ = 0; //++ @ markSelected and insert, -- @ extract/sendDown

    // H-ARQ unit instances (overwrite the items the base class allocated)
    for (unsigned int i = 0; i < numHarqUnits_; i++) {
        delete units_[i];
        units_[i] = new LteHarqUnitTxD2D(binder, acid, i, macOwner_, dstMac);
    }
}

Packet *LteHarqProcessTxD2D::extractPdu(Codeword cw)
{
    if (numSelected_ == 0)
        throw cRuntimeError("H-ARQ TX process: cannot extract PDU: numSelected = 0 ");

    numSelected_--;
    Packet *pkt = units_[cw]->extractPdu();
    auto pdu = pkt->peekAtFront<LteMacPdu>();
    auto infoVec = getTagsWithInherit<LteControlInfo>(pkt);
    if (infoVec.empty())
        throw cRuntimeError("No tag of type LteControlInfo found");
    auto info = infoVec.front();

    if (info.getDirection() == D2D_MULTI) {
        // if the PDU is for a multicast/broadcast connection, the selected unit has been emptied
        numEmptyUnits_++;
    }
    return pkt;
}

} //namespace

