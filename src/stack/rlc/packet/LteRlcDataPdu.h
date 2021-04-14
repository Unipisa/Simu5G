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

#ifndef LTERLCDATAPDU_H_
#define LTERLCDATAPDU_H_

#include "stack/rlc/packet/LteRlcDataPdu_m.h"
#include "stack/rlc/LteRlcDefs.h"

/**
 * @class LteRlcDataPdu
 * @brief Base class for Lte RLC UM/AM Data Pdu
 *
 * Class derived from base class contained
 * in msg declaration: define common fields for UM/AM PDU
 * A Data PDU contains a list of SDUs: a SDU can be
 * a whole SDU or a fragment
 */
class LteRlcDataPdu : public LteRlcDataPdu_Base {
private:
    void copy(const LteRlcDataPdu& other) {
        // "the copy constructor of a container should dup() the owned objects and take() the copies"
        sduList_.clear();
        RlcSduList::const_iterator sit;
        for (sit = other.sduList_.begin(); sit != other.sduList_.end(); ++sit)
        {
            auto newPkt = (*sit)->dup();
            take(newPkt);
            sduList_.push_back(newPkt);
        }
        sduSizes_ = other.sduSizes_;
        numSdu_ = other.numSdu_;
        fi_ = other.fi_;
        pduSequenceNumber_ = other.pduSequenceNumber_;
        rlcPduLength_ = other.rlcPduLength_;
    }
protected:

    /// List Of MAC SDUs
    RlcSduList sduList_;
    RlcSduListSizes sduSizes_;

    // number of SDU stored in the message
    unsigned int numSdu_;

    // define the segmentation info for the PDU
    FramingInfo fi_;

    // Sequence number of the PDU
    unsigned int pduSequenceNumber_;

    // Length of the PDU
    inet::int64 rlcPduLength_;


public:

    /**
     * Constructor
     */
    LteRlcDataPdu() : LteRlcDataPdu_Base()
    {
        this->setChunkLength(inet::b(1));
        numSdu_ = 0;
        pduSequenceNumber_ = 0;
        fi_ = 0;
        rlcPduLength_ = 0;
    }

    virtual ~LteRlcDataPdu()
    {

        // Needs to delete all contained packets
        RlcSduList::iterator sit;
        for (sit = sduList_.begin(); sit != sduList_.end(); sit++)
            dropAndDelete(*sit);
    }

    LteRlcDataPdu(const LteRlcDataPdu& other) : LteRlcDataPdu_Base(other)
    {
        copy(other);
    }

    LteRlcDataPdu& operator=(const LteRlcDataPdu& other)
    {
        if (&other == this)
            return *this;
        LteRlcDataPdu_Base::operator=(other);

        copy(other);
        return *this;
    }

    virtual LteRlcDataPdu *dup() const override
    {
        return new LteRlcDataPdu(*this);
    }

    void setPduSequenceNumber(unsigned int sno);
    unsigned int getPduSequenceNumber() const;

    void setFramingInfo(FramingInfo fi);
    FramingInfo getFramingInfo() const;

    unsigned int getNumSdu() const { return numSdu_; }

    /**
     * pushSdu() gets ownership of the packet
     * and stores it inside the rlc sdu list
     * in back position
     *
     * @param pkt packet to store
     */
    virtual void pushSdu(inet::Packet* pkt)
    {
        take(pkt);
        rlcPduLength_ += pkt->getByteLength();
        sduList_.push_back(pkt);
        sduSizes_.push_back(pkt->getByteLength());
        numSdu_++;
    }

    virtual void pushSdu(inet::Packet* pkt, int size)
    {
        take(pkt);
        rlcPduLength_ += size;
        sduList_.push_back(pkt);
        sduSizes_.push_back(size);
        numSdu_++;
    }

    /**
     * popSdu() pops a packet from front of
     * the sdu list and drops ownership before
     * returning it
     *
     * @return popped packet
     */
    virtual inet::Packet* popSdu(size_t &size)
    {
        auto pkt = sduList_.front();
        sduList_.pop_front();
        size = sduSizes_.front();
        rlcPduLength_ -= (sduSizes_.front());
        sduSizes_.pop_front();
        numSdu_--;
        drop(pkt);
        return pkt;
    }
};

/**
 * @class LteRlcUmDataPdu
 * @brief Lte RLC UM Data Pdu
 *
 * Define additional fields for UM PDU
 */
class LteRlcUmDataPdu : public LteRlcDataPdu {
private:
    void copy(const LteRlcUmDataPdu &other) {

    }

public:

    LteRlcUmDataPdu() : LteRlcDataPdu()
    {
        rlcPduLength_ = RLC_HEADER_UM;
        this->setChunkLength(inet::B(rlcPduLength_));
    }

    virtual ~LteRlcUmDataPdu() {}

    LteRlcUmDataPdu(const LteRlcUmDataPdu& other) : LteRlcDataPdu(other)
    {
        copy(other);
    }

    LteRlcUmDataPdu& operator=(const LteRlcUmDataPdu& other)
    {
        if (&other == this)
            return *this;
        LteRlcDataPdu::operator=(other);
        return *this;
    }

    virtual LteRlcUmDataPdu *dup() const
    {
        return new LteRlcUmDataPdu(*this);
    }
};

/**
 * @class LteRlcAmDataPdu
 * @brief Lte RLC AM Data Pdu
 *
 * Define additional fields for AM PDU
 */
class LteRlcAmDataPdu : public LteRlcDataPdu {
private:
    void copy(const LteRlcAmDataPdu &other) {
        pollStatus_ = other.pollStatus_;
    }
protected:

    // if true, a status PDU is required
    bool pollStatus_;

public:

    LteRlcAmDataPdu() : LteRlcDataPdu()
    {
        rlcPduLength_ = RLC_HEADER_UM;
        this->setChunkLength(inet::B(rlcPduLength_));
    }

    LteRlcAmDataPdu(const LteRlcAmDataPdu& other) : LteRlcDataPdu(other)
    {
        copy(other);
    }

    LteRlcAmDataPdu& operator=(const LteRlcAmDataPdu& other)
    {
        if (&other == this)
            return *this;
        copy(other);
        LteRlcDataPdu::operator=(other);
        return *this;
    }


    virtual ~LteRlcAmDataPdu() {}

    void setPollStatus(bool p) { pollStatus_ = p; }
    bool getPollStatus() const { return pollStatus_; }
};


Register_Class(LteRlcDataPdu);
Register_Class(LteRlcUmDataPdu);
Register_Class(LteRlcAmDataPdu);

#endif /* LTERLCDATAPDU_H_ */
