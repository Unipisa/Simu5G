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

#ifndef _LTE_LTEMACPDU_H_
#define _LTE_LTEMACPDU_H_

#include "stack/mac/packet/LteMacPdu_m.h"
#include "common/LteCommon.h"
#include "common/LteControlInfo.h"

namespace simu5g {

/**
 * @class LteMacPdu
 * @brief Lte MAC Pdu
 *
 * Class derived from base class contained
 * in msg declaration: adds the SDU and control elements list
 * TODO: Add Control Elements
 */
class LteMacPdu : public LteMacPdu_Base
{
    void copy(const LteMacPdu& other) {
        macPduLength_ = other.macPduLength_;
        macPduId_ = other.macPduId_;
        sduList_ = other.sduList_->dup();
        take(sduList_);
        // duplicate MacControlElementsList (includes BSRs)
        ceList_ = std::list<MacControlElement *> ();
        MacControlElementsList otherCeList = other.ceList_;
        for (auto* ce : otherCeList) {
            MacBsr *bsr = dynamic_cast<MacBsr *>(ce);
            if (bsr) {
                ceList_.push_back(new MacBsr(*bsr));
            }
            else {
                ceList_.push_back(new MacControlElement(*ce));
            }
        }
        // duplication of the SDU queue duplicates all packets but not
        // the ControlInfo - iterate over all packets and restore ControlInfo if necessary
        cPacketQueue::Iterator iterOther(*other.sduList_);
        for (cPacketQueue::Iterator iter(*sduList_); !iter.end(); iter++) {
            cPacket *p1 = static_cast<cPacket *>(*iter);
            cPacket *p2 = static_cast<cPacket *>(*iterOther);
            if (p1->getControlInfo() == nullptr && p2->getControlInfo() != nullptr) {
                FlowControlInfo *fci = dynamic_cast<FlowControlInfo *>(p2->getControlInfo());
                if (fci) {
                    p1->setControlInfo(new FlowControlInfo(*fci));
                }
                else {
                    throw cRuntimeError("LteMacPdu.h::Unknown type of control info in SDU list!");
                }
            }
            iterOther++;
        }
    }

  protected:
    /// List Of MAC SDUs
    cPacketQueue *sduList_ = nullptr;

    /// List of MAC CEs
    MacControlElementsList ceList_;

    /// Length of the PDU
    int64_t macPduLength_ = 0;

    /// ID of the MAC PDU: incrementally set according to the static variable numMacPdus
    int64_t macPduId_;
    static int64_t numMacPdus_;

  public:

    /**
     * Constructor
     */
    LteMacPdu() : LteMacPdu_Base(), sduList_(new cPacketQueue("SDU List"))
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

    /*
     * Copy constructors
     */

    LteMacPdu(const LteMacPdu& other) :
        LteMacPdu_Base(other)
    {
        copy(other);
    }

    LteMacPdu& operator=(const LteMacPdu& other)
    {
        if (&other == this)
            return *this;

        LteMacPdu_Base::operator=(other);
        copy(other);

        return *this;
    }

    LteMacPdu *dup() const override
    {
        return new LteMacPdu(*this);
    }

    /**
     * info() prints a one line description of this object
     */
    std::string str() const override
    {
        std::stringstream ss;
        std::string s;
        ss << (std::string)getName() << " containing "
           << sduList_->getLength() << " SDUs and " << ceList_.size() << " CEs"
           << " with size " << getByteLength();
        s = ss.str();
        return s;
    }

    /**
     * Destructor
     */
    ~LteMacPdu() override
    {
        // delete the SDU queue
        // (since it is derived from cPacketQueue, it will automatically delete all contained SDUs)
        drop(sduList_);
        delete sduList_;

        for (auto* ce : ceList_) {
            delete ce;
        }
    }

    int64_t getByteLength() const
    {
        return macPduLength_ + getHeaderLength();
    }

    int64_t getBitLength() const
    {
        return getByteLength() * 8;
    }

    void setSduArraySize(size_t size) override
    {
        ASSERT(false);
    }

    size_t getSduArraySize() const override
    {
        return sduList_->getLength();
    }

    const inet::Packet& getSdu(size_t k) const override
    {
        auto pkt = dynamic_cast<Packet *>(sduList_->get(k));
        return *pkt;
    }

    void setSdu(size_t k, const Packet& sdu) override
    {
        ASSERT(false);
    }

    void appendSdu(const inet::Packet& sdu) override
    {
        ASSERT(false);
    }

    void insertSdu(size_t k, const inet::Packet& sdu) override
    {
        ASSERT(false);
    }

    void eraseSdu(size_t k) override
    {
        ASSERT(false);
    }

    /**
     * pushSdu() gets ownership of the packet
     * and stores it inside the MAC SDU list
     * in back position
     *
     * @param pkt packet to store
     */
    virtual void pushSdu(Packet *pkt)
    {
        take(pkt);
        macPduLength_ += pkt->getByteLength();

        // sduList_ will take ownership
        drop(pkt);
        sduList_->insert(pkt);
        this->setChunkLength(b(getBitLength()));
    }

    /**
     * popSdu() pops a packet from the front of
     * the SDU list and drops ownership before
     * returning it
     *
     * @return popped packet
     */
    virtual Packet *popSdu()
    {
        Packet *pkt = check_and_cast<Packet *>(sduList_->pop());
        macPduLength_ -= pkt->getByteLength();
        this->setChunkLength(b(getBitLength()));
        take(pkt);
        drop(pkt);
        return pkt;
    }

    /**
     * hasSdu() verifies if there are other
     * SDUs inside the SDU list
     *
     * @return true if the list is not empty, false otherwise
     */
    virtual bool hasSdu() const
    {
        return !sduList_->isEmpty();
    }

    /**
     * pushCe() stores a CE inside the
     * MAC CE list in back position
     *
     * @param pkt CE to store
     */
    virtual void pushCe(MacControlElement *ce)
    {
        ceList_.push_back(ce);
    }

    /**
     * popCe() pops a CE from the front of
     * the CE list and returns it
     *
     * @return popped CE
     */
    virtual MacControlElement *popCe()
    {
        MacControlElement *ce = ceList_.front();
        ceList_.pop_front();
        return ce;
    }

    /**
     * hasCe() verifies if there are other
     * CEs inside the CE list
     *
     * @return true if the list is not empty, false otherwise
     */
    virtual bool hasCe() const
    {
        return !ceList_.empty();
    }

    long getId() const
    {
        return macPduId_;
    }

    void setHeaderLength(unsigned int headerLength) override
    {
        LteMacPdu_Base::setHeaderLength(headerLength);
        this->setChunkLength(b(getBitLength()));
    }

};

Register_Class(LteMacPdu);

} //namespace

#endif

