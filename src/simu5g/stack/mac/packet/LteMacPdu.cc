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

int64_t LteMacPdu::numMacPdus_ = 0;

Register_Class(LteMacPdu);


LteMacPdu::LteMacPdu() : LteMacPdu_Base()
{
    macPduId_ = numMacPdus_++;
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
    // delete the SDU queue
    // (since it is derived from cPacketQueue, it will automatically delete all contained SDUs)
    for (auto *ce : ceList_) {
        delete ce;
    }
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
    // duplicate MacControlElementsList (includes BSRs)
    ceList_ = std::list<MacControlElement*>();
    MacControlElementsList otherCeList = other.ceList_;
    for (auto *ce : otherCeList) {
        MacBsr *bsr = dynamic_cast<MacBsr*>(ce);
        if (bsr) {
            ceList_.push_back(new MacBsr(*bsr));
        } else {
            ceList_.push_back(new MacControlElement(*ce));
        }
    }
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
            << " SDUs and " << ceList_.size() << " CEs" << " with size "
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

void LteMacPdu::parsimPack(omnetpp::cCommBuffer *b) const
{
    // Pack the base class fields first
    LteMacPdu_Base::parsimPack(b);

    // Pack the CE list
    int ceCount = ceList_.size();
    omnetpp::doParsimPacking(b, ceCount);

    for (auto* ce : ceList_) {
        // First pack the type information to distinguish between MacControlElement and MacBsr
        MacBsr *bsr = dynamic_cast<MacBsr *>(ce);
        bool isBsr = (bsr != nullptr);
        omnetpp::doParsimPacking(b, isBsr);

        // Pack the control element
        //TODO ce->parsimPack(b);
    }
}

void LteMacPdu::parsimUnpack(omnetpp::cCommBuffer *b)
{
    // Unpack the base class fields first
    LteMacPdu_Base::parsimUnpack(b);

    // Clear existing CE list
    for (auto* ce : ceList_) {
        delete ce;
    }
    ceList_.clear();

    // Unpack the CE list
    int ceCount;
    omnetpp::doParsimUnpacking(b, ceCount);

    for (int i = 0; i < ceCount; i++) {
        // Unpack the type information
        bool isBsr;
        omnetpp::doParsimUnpacking(b, isBsr);

        // Create the appropriate control element type and unpack it
        MacControlElement *ce;
        if (isBsr) {
            ce = new MacBsr();
        } else {
            ce = new MacControlElement();
        }
        ce->parsimUnpack(b);
        ceList_.push_back(ce);
    }
}

} //namespace
