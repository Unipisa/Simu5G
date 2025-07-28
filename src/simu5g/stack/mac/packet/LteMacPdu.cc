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

#include "simu5g/stack/mac/packet/LteMacPdu.h"

using namespace inet;

namespace simu5g {

Register_Class(LteMacPdu);


std::string LteMacPdu::str() const
{
    std::stringstream ss;
    std::string s;
    ss << (std::string) (getName()) << " containing " << sdu_arraysize
            << " SDUs and " << ce_arraysize << " CEs" << " with size "
            << getByteLength();
    s = ss.str();
    return s;
}

void LteMacPdu::forEachChild(cVisitor *v)
{
    FieldsChunk::forEachChild(v);
    for (int i = 0; i < sdu_arraysize; i++) {
        if (!v->visit(sdu[i]))
            return;
    }
}

void LteMacPdu::pushSdu(Packet *pkt)
{
    appendSdu(pkt);
    macPduLength_ += pkt->getByteLength();
    // addChunkLength(pkt->getDataLength());
    this->setChunkLength(b(getBitLength()));
}

Packet* LteMacPdu::popSdu()
{
    Packet *pkt = removeSdu(0);
    eraseSdu(0);
    macPduLength_ -= pkt->getByteLength();
    // addChunkLength(-pkt->getDataLength());
    this->setChunkLength(b(getBitLength()));
    return pkt;
}

} //namespace
