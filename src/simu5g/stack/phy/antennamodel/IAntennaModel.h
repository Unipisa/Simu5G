#ifndef __SIMU5G_IANTENNAMODEL_H_
#define __SIMU5G_IANTENNAMODEL_H_

#include <cstring>

#include <omnetpp.h>

namespace simu5g {

enum class AntennaPolarization {
    NONE,
    LINEAR,
    CIRCULAR
};

inline AntennaPolarization antennaPolarizationFromString(const char *polarization)
{
    if (!strcmp(polarization, "NONE"))
        return AntennaPolarization::NONE;
    if (!strcmp(polarization, "LINEAR"))
        return AntennaPolarization::LINEAR;
    if (!strcmp(polarization, "CIRCULAR"))
        return AntennaPolarization::CIRCULAR;
    throw omnetpp::cRuntimeError("Unknown antenna polarization '%s'", polarization);
}

//
// Interface class for antenna models
//
class IAntennaModel
{
  protected:
    double txPower_;           // in dBm
    double txFeederLoss_;      // in dB
    double rxFeederLoss_;      // in dB
    double rxLumpedLoss_;      // in dB
    double noiseFigure_;       // in dB
    AntennaPolarization antennaPolarization_ = AntennaPolarization::NONE;

  public:

    // return the tx power (in dBm)
    virtual double getTxPower() const { return txPower_; }

    // return the tx feeder loss (in dB)
    virtual double getTxFeederLoss() const { return txFeederLoss_; }
    // return the rx feeder loss (in dB)
    virtual double getRxFeederLoss() const { return rxFeederLoss_; }
    // return the rx lumped loss (in dB)
    virtual double getRxLumpedLoss() const { return rxLumpedLoss_; }
    // return the noise figure (in dB)
    virtual double getNoiseFigure() const { return noiseFigure_; }

    virtual double computeTxGain(double angle = -1.0, double frequency = -1.0)  const = 0;

    // returns the receiving antenna gain (in dBi)
    virtual double computeRxGain(double angle = -1.0, double frequency= -1.0) const = 0;

    virtual AntennaPolarization getAntennaPolarization() const { return antennaPolarization_; }
};

}

#endif
