#ifndef __SIMU5G_IANTENNAMODEL_H_
#define __SIMU5G_IANTENNAMODEL_H_

#include <omnetpp.h>

namespace simu5g {
//
// Interface class for antenna models
//
class IAntennaModel
{
  public:

    virtual double computeTxGain(double angle = -1.0, double frequency = -1.0)  const = 0;

    // returns the receiving antenna gain (in dBi)
    virtual double computeRxGain(double angle = -1.0, double frequency= -1.0) const = 0;

};

}

#endif
