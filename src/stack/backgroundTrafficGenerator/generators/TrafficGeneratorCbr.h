//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef TRAFFICGENERATORCBR_H_
#define TRAFFICGENERATORCBR_H_

#include "stack/backgroundTrafficGenerator/generators/TrafficGeneratorBase.h"

//
// TrafficGeneratorCbr
//
class TrafficGeneratorCbr : public TrafficGeneratorBase
{
  protected:

    // message length (excluding headers)
    unsigned int size_[2];

    // inter-arrival time
    double period_[2];

    virtual void initialize(int stage) override;

    // -- re-implemented functions from the base class -- //

    // generate a new message with length size_
    virtual unsigned int generateTraffic(Direction dir) override;

    // returns the period_
    virtual simtime_t getNextGenerationTime(Direction dir) override;

  public:
    TrafficGeneratorCbr() {}
    virtual ~TrafficGeneratorCbr() {}

};

#endif
