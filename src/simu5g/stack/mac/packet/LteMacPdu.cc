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

namespace simu5g {

int64_t LteMacPdu::numMacPdus_ = 0;

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
