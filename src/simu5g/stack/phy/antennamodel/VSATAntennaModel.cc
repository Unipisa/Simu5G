#include "simu5g/stack/phy/antennamodel/VSATAntennaModel.h"
#include "inet/common/INETMath.h"

#include <cmath>

namespace simu5g {

Define_Module(VSATAntennaModel);

void VSATAntennaModel::initialize()
{
    txPower_ = par("txPower");
    efficiency_ = par("efficiency");
    diameter_ = par("diameter");
    txFeederLoss_ = par("txFeederLoss");
    rxFeederLoss_ = par("rxFeederLoss");
    rxLumpedLoss_ = par("rxLumpedLoss");
    noiseFigure_ = par("noiseFigure");
    antennaPolarization_ = antennaPolarizationFromString(par("antennaPolarization").stringValue());
}

double VSATAntennaModel::computeGain(double angle, double frequency) const
{
    if (frequency <= 0.0)
        throw omnetpp::cRuntimeError("VSATAntennaModel::computeGain - frequency must be positive");
    if (angle < 0.0 || angle > 90.0)
        throw omnetpp::cRuntimeError("VSATAntennaModel::computeGain - angle must be in the [0, 90] degree range");

    double lambda = SPEED_OF_LIGHT / frequency;
    double v = M_PI * (diameter_ / lambda);
    double maxGain = inet::math::fraction2dB(efficiency_ * v * v);

    // compute actual gain via Bessel function

    double theta = inet::math::deg2rad(angle);
    double x = M_PI * (diameter_ / lambda) * std::sin(theta);

    if (std::abs(x) < 1e-9)
        return maxGain;

    double bessel = ::j1(x);

    double d = (2.0 * bessel) / x;
    double besselFactor = d * d;

    return maxGain + inet::math::fraction2dB(besselFactor);
}

}
