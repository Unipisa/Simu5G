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

#include "stack/rlc/am/packet/LteRlcAmPdu.h"

namespace simu5g {

void LteRlcAmPdu::setBitmapArraySize(size_t size)
{
    bitmap_.resize(size);
}

size_t LteRlcAmPdu::getBitmapArraySize() const
{
    return bitmap_.size();
}

bool LteRlcAmPdu::getBitmap(size_t k) const
{
    return bitmap_.at(k);
}

void LteRlcAmPdu::setBitmap(size_t k, bool bitmap_)
{
    this->bitmap_[k] = bitmap_;
}

void LteRlcAmPdu::setBitmapVec(std::vector<bool> bitmap_vec)
{
    bitmap_ = bitmap_vec;
}

std::vector<bool> LteRlcAmPdu::getBitmapVec()
{
    return bitmap_;
}

bool LteRlcAmPdu::isWhole() const
{
    return firstSn == lastSn;
}

bool LteRlcAmPdu::isFirst() const
{
    return firstSn == snoFragment;
}

bool LteRlcAmPdu::isMiddle() const
{
    return !isFirst() && !isLast();
}

bool LteRlcAmPdu::isLast() const
{
    return lastSn == snoFragment;
}

} //namespace

