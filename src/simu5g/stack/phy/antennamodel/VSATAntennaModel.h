#ifndef __SIMU5G_VSATANTENNAMODEL_H_
#define __SIMU5G_VSATANTENNAMODEL_H_

#include "simu5g/stack/phy/antennamodel/IAntennaModel.h"

namespace simu5g {
//
// Model for an VSAT antenna
//
class VSATAntennaModel : public omnetpp::cSimpleModule, public IAntennaModel
{

  protected:

    double efficiency_;
    double diameter_;

    virtual void initialize() override;

    double computeGain(double angle, double frequency) const;

  public:

    // returns the transmitting antenna gain (in dBi)
    virtual double computeTxGain(double angle = -1.0, double frequency = -1.0) const override { return computeGain(angle, frequency); }

    // returns the receiving antenna gain (in dBi)
    virtual double computeRxGain(double angle = -1.0, double frequency = -1.0) const override  { return computeGain(angle, frequency); }
};

}

#endif
