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

#include "L2MeasBase.h"

namespace simu5g {


void L2MeasBase::init(std::string name, int period, bool movingAverage)
{
    name_ = name;
    values_.resize(period, 0.);
    period_ = period;
    size_ = 0;
    index_ = 0;
    sum_ = 0;
    mean_ = 0;
    movingAverage_ = movingAverage;
}


void L2MeasBase::addValue(double value) {
    lastValue_ = (int)value;
    sum_ += value;
    if (size_ < period_) {
        size_++;
    }
    else {
        index_ = index_ % period_;
        sum_ -= values_.at(index_);
    }
    values_.at(index_++) = value;

    if (movingAverage_ || index_ == period_) { // compute mean
        mean_ = computeMean();
        //outVector_.record(mean_);
        //histogram_.collect(mean_);
    }
}

int L2MeasBase::computeMean()
{
    if (index_ == 0)
        return 0;
    if (!movingAverage_ && size_ < period_) // not enough data
        return 0;
    else {
        int mean = floor(sum_ / size_);
        return mean < 0 ? 0 : mean; // round could return -0.00 -> -1
    }
}

int L2MeasBase::getMean()
{
    return mean_;
}

void L2MeasBase::reset()
{
    lastValue_ = 0;
    std::fill(values_.begin(), values_.end(), 0.);
    size_ = 0;
    index_ = 0;
    sum_ = 0;
    mean_ = 0;
}

} //namespace

