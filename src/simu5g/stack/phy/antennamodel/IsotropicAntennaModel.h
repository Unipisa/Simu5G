#ifndef __SIMU5G_ISOTROPICANTENNAMODEL_H_
#define __SIMU5G_ISOTROPICANTENNAMODEL_H_

#include "simu5g/stack/phy/antennamodel/IAntennaModel.h"

namespace simu5g {
//
// Model for an isotropic antenna
//
class IsotropicAntennaModel : public omnetpp::cSimpleModule, public IAntennaModel
{

  protected:

    double boresightElevation_ = 90.0; // in degrees

    virtual void initialize() override;

  public:

    // returns the transmitting antenna gain (in dBi)
    virtual double computeTxGain(double angle = -1.0, double frequency = -1.0)  const override { return 0.0; }

    // returns the receiving antenna gain (in dBi)
    virtual double computeRxGain(double angle = -1.0, double frequency = -1.0) const override { return 0.0; }

    virtual double getBoresightElevation() const override { return boresightElevation_; }
};

}

#endif
