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

//        ::omnetpp::cOutVector outVector_;
//        ::omnetpp::cHistogram histogram_;

    public:
        L2MeasBase();
        virtual void init(std::string name, int period, bool movingAverage);
        virtual ~L2MeasBase();
        virtual void addValue(double value);
        virtual int computeMean();
        virtual int getMean();
        virtual int getLastValue()
        {
            return lastValue_;
        }
        virtual void reset();
};


#endif //_L2MEASBASE_H_
