#ifndef __SIMU5G_NTNPLATFORMANTENNAMODEL_H_
#define __SIMU5G_NTNPLATFORMANTENNAMODEL_H_

#include "simu5g/stack/phy/antennamodel/IAntennaModel.h"

namespace simu5g {
//
// Model for an NTN platform antenna
//
class NtnPlatformAntennaModel : public omnetpp::cSimpleModule, public IAntennaModel
{

  protected:

    double txEfficiency_;
    double rxEfficiency_;
    double txDiameter_;        // in m
    double rxDiameter_;        // in m
    double offNadirAngle_ = 0.0; // in degrees

    virtual void initialize() override;

    double computeGain(double angle, double frequency, double diameter, double efficiency) const;

  public:

    // returns the transmitting antenna gain (in dBi)
    virtual double computeTxGain(double angle = -1.0, double frequency = -1.0) const override { return computeGain(angle, frequency, txDiameter_, txEfficiency_); }

    // returns the receiving antenna gain (in dBi)
    virtual double computeRxGain(double angle = -1.0, double frequency = -1.0) const override { return computeGain(angle, frequency, rxDiameter_, rxEfficiency_); }

    virtual double getOffNadirAngle() const override { return offNadirAngle_; }
};

}

#endif
