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

#ifndef _L2MEASBASE_H_
#define _L2MEASBASE_H_

#include <vector>
#include <string>
#include <omnetpp.h>

namespace simu5g {

using namespace omnetpp;

class L2MeasBase
{
  private:
    std::string name_;
    std::vector<double> values_;
    double sum_;
    int lastValue_;
    int mean_;
    int index_;
    int period_;
    int size_;
    bool movingAverage_;

//        ::cOutVector outVector_;
//        ::cHistogram histogram_;

  public:
    virtual void init(std::string name, int period, bool movingAverage);
    virtual void addValue(double value);
    virtual int computeMean();
    virtual int getMean();
    virtual int getLastValue()
    {
        return lastValue_;
    }

    virtual void reset();
};

} //namespace

#endif //_L2MEASBASE_H_

