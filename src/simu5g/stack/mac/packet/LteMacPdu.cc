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


LteMacPdu::LteMacPdu() : LteMacPdu_Base(), sduList_(new cPacketQueue("SDU List"))
{
    macPduId_ = numMacPdus_++;
    take(sduList_);
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
    drop(sduList_);
    delete sduList_;
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
    sduList_ = other.sduList_->dup();
    take(sduList_);
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
    cPacketQueue::Iterator iterOther(*other.sduList_);
    for (cPacketQueue::Iterator iter(*sduList_); !iter.end(); iter++) {
        cPacket *p1 = static_cast<cPacket*>(*iter);
        cPacket *p2 = static_cast<cPacket*>(*iterOther);
        if (p1->getControlInfo() == nullptr
                && p2->getControlInfo() != nullptr) {
            FlowControlInfo *fci = dynamic_cast<FlowControlInfo*>(p2->getControlInfo());
            if (fci) {
                p1->setControlInfo(new FlowControlInfo(*fci));
            } else {
                throw cRuntimeError("LteMacPdu.h::Unknown type of control info in SDU list!");
            }
        }
        iterOther++;
    }
}

std::string LteMacPdu::str() const
{
    std::stringstream ss;
    std::string s;
    ss << (std::string) (getName()) << " containing " << sduList_->getLength()
            << " SDUs and " << ceList_.size() << " CEs" << " with size "
            << getByteLength();
    s = ss.str();
    return s;
}

void LteMacPdu::forEachChild(cVisitor *v)
{
    int sduCount = sduList_->getLength();
    for (int i = 0; i < sduCount; i++) {
        cPacket *pkt = static_cast<cPacket *>(sduList_->get(i));
        if (!v->visit(pkt))
            return;
    }
}

void LteMacPdu::pushSdu(Packet *pkt)
{
    take(pkt);
    macPduLength_ += pkt->getByteLength();
    // sduList_ will take ownership
    drop(pkt);
    sduList_->insert(pkt);
    this->setChunkLength(b(getBitLength()));
}

Packet* LteMacPdu::popSdu()
{
    Packet *pkt = check_and_cast<Packet*>(sduList_->pop());
    macPduLength_ -= pkt->getByteLength();
    this->setChunkLength(b(getBitLength()));
    take(pkt);
    drop(pkt);
    return pkt;
}

void LteMacPdu::parsimPack(omnetpp::cCommBuffer *b) const
{
    // Pack the base class fields first
    LteMacPdu_Base::parsimPack(b);

    // Pack macPduLength_
    omnetpp::doParsimPacking(b, this->macPduLength_);

    // Pack the SDU list
    int sduCount = sduList_->getLength();
    omnetpp::doParsimPacking(b, sduCount);

    for (int i = 0; i < sduCount; i++) {
        cPacket *pkt = static_cast<cPacket *>(sduList_->get(i));
        // Pack the packet using its own parsimPack method
        pkt->parsimPack(b);
    }

    // Pack the CE list
    int ceCount = ceList_.size();
    omnetpp::doParsimPacking(b, ceCount);

    for (auto* ce : ceList_) {
        // First pack the type information to distinguish between MacControlElement and MacBsr
        MacBsr *bsr = dynamic_cast<MacBsr *>(ce);
        bool isBsr = (bsr != nullptr);
        omnetpp::doParsimPacking(b, isBsr);

        // Pack the control element
        ce->parsimPack(b);
    }
}

void LteMacPdu::parsimUnpack(omnetpp::cCommBuffer *b)
{
    // Unpack the base class fields first
    LteMacPdu_Base::parsimUnpack(b);

    // Unpack macPduLength_
    omnetpp::doParsimUnpacking(b, this->macPduLength_);

    // Clear existing SDU list
    while (!sduList_->isEmpty()) {
        cPacket *pkt = static_cast<cPacket *>(sduList_->pop());
        delete pkt;
    }

    // Unpack the SDU list
    int sduCount;
    omnetpp::doParsimUnpacking(b, sduCount);

    for (int i = 0; i < sduCount; i++) {
        // Create a new packet and unpack it
        Packet *pkt = new Packet();
        pkt->parsimUnpack(b);
        sduList_->insert(pkt);
    }

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
