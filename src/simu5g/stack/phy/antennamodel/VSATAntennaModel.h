#ifndef __SIMU5G_VSATANTENNAMODEL_H_
#define __SIMU5G_VSATANTENNAMODEL_H_

#include "simu5g/stack/phy/antennamodel/IAntennaModel.h"

namespace simu5g {
//
// Model for an VSAT antenna
//
class VSATAntennaModel : public omnetpp::cSimpleModule, IAntennaModel
{

  protected:

    double txPower_;           // in dBm
    double efficiency_;
    double diameter_;
    double txFeederLoss_;      // in dB
    double rxFeederLoss_;      // in dB
    double rxLumpedLoss_;      // in dB
    double noiseFigure_;       // in dB

    virtual void initialize() override;

    double computeGain(double angle, double frequency) const;

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

    // returns the transmitting antenna gain (in dBi)
    virtual double computeTxGain(double angle = -1.0, double frequency = -1.0) const override { return computeGain(angle, frequency); }

    // returns the receiving antenna gain (in dBi)
    virtual double computeRxGain(double angle = -1.0, double frequency = -1.0) const override  { return computeGain(angle, frequency); }
};

}

#endif
