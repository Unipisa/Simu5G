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


LteMacPdu::LteMacPdu() : LteMacPdu_Base()
{
    /*
     * @author Alessandro Noferi
     *
     * getChunkId returns an int (static var in chunk.h)
     * TODO think about creating our own static variable
     */
    macPduId_ = getChunkId();
}

LteMacPdu::~LteMacPdu()
{
}

simu5g::LteMacPdu& LteMacPdu::operator=(const LteMacPdu &other)
{
    if (&other == this)
        return *this;

    LteMacPdu_Base::operator =(other);
    copy(other);
    return *this;
}

void LteMacPdu::copy(const LteMacPdu &other)
{
    macPduLength_ = other.macPduLength_;
    macPduId_ = other.macPduId_;

    // duplication of the SDU queue duplicates all packets but not
    // the ControlInfo - iterate over all packets and restore ControlInfo if necessary
    for (size_t idx = 0; idx < sdu_arraysize; idx++) {
        cPacket *p1 = static_cast<cPacket*>(sdu[idx]);
        cPacket *p2 = static_cast<cPacket*>(other.sdu[idx]);
        if (p1->getControlInfo() == nullptr && p2->getControlInfo() != nullptr) {
            FlowControlInfo *fci = dynamic_cast<FlowControlInfo*>(p2->getControlInfo());
            if (fci) {
                p1->setControlInfo(new FlowControlInfo(*fci));
            } else {
                throw cRuntimeError("LteMacPdu.h::Unknown type of control info in SDU list!");
            }
        }
    }
}

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
    for (int i = 0; i < sdu_arraysize; i++) {
        if (!v->visit(sdu[i]))
            return;
    }
}

void LteMacPdu_Base::pushSdu(Packet *pkt)
{
    appendSdu(pkt);
    macPduLength_ += pkt->getByteLength();
    // addChunkLength(pkt->getDataLength());
    this->setChunkLength(b(getBitLength()));
}

Packet* LteMacPdu_Base::popSdu()
{
    Packet *pkt = removeSdu(0);
    eraseSdu(0);
    macPduLength_ -= pkt->getByteLength();
    // addChunkLength(-pkt->getDataLength());
    this->setChunkLength(b(getBitLength()));
    return pkt;
}

} //namespace
