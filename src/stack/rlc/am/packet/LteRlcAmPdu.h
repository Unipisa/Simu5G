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

#ifndef _LTE_LTERLCAMPDU_H_
#define _LTE_LTERLCAMPDU_H_

#include "stack/rlc/am/packet/LteRlcAmPdu_m.h"

namespace simu5g {

using namespace omnetpp;

class LteRlcAmPdu : public LteRlcAmPdu_Base
{
    std::vector<bool> bitmap_;

  public:

    LteRlcAmPdu() :
        LteRlcAmPdu_Base()
    {
    }

    LteRlcAmPdu(const LteRlcAmPdu& other) :
        LteRlcAmPdu_Base(other)
    {
        operator=(other);
    }

    LteRlcAmPdu& operator=(const LteRlcAmPdu& other)
    {
        LteRlcAmPdu_Base::operator=(other);
        bitmap_ = other.bitmap_;
        return *this;
    }

    LteRlcAmPdu *dup() const override
    {
        return new LteRlcAmPdu(*this);
    }

    void setBitmapArraySize(size_t size) override;
    size_t getBitmapArraySize() const override;
    bool getBitmap(size_t k) const override;
    void setBitmap(size_t k, bool bitmap_var) override;
    virtual void setBitmapVec(std::vector<bool> bitmap_vec);

    void appendBitmap(bool bitmap) override { throw cRuntimeError("Method not implemented"); }
    void insertBitmap(size_t k, bool bitmap) override { throw cRuntimeError("Method not implemented"); }
    void eraseBitmap(size_t k)  override { throw cRuntimeError("Method not implemented"); }

    virtual std::vector<bool> getBitmapVec();
    // sequence check functions
    virtual bool isWhole() const;
    virtual bool isFirst() const;
    virtual bool isMiddle() const;
    virtual bool isLast() const;
};

Register_Class(LteRlcAmPdu);

} //namespace

#endif

