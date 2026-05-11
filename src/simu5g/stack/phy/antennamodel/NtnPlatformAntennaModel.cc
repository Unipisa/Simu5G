#include "simu5g/stack/phy/antennamodel/NtnPlatformAntennaModel.h"
#include "inet/common/INETMath.h"

#include <cmath>

namespace simu5g {

Define_Module(NtnPlatformAntennaModel);

void NtnPlatformAntennaModel::initialize()
{
    txPower_ = par("txPower");
    txEfficiency_ = par("txEfficiency");
    rxEfficiency_ = par("rxEfficiency");
    txDiameter_ = par("txDiameter");
    rxDiameter_ = par("rxDiameter");
    txFeederLoss_ = par("txFeederLoss");
    rxFeederLoss_ = par("rxFeederLoss");
    rxLumpedLoss_ = par("rxLumpedLoss");
    noiseFigure_ = par("noiseFigure");
    temperature_ = par("temperature");
    antennaPolarization_ = antennaPolarizationFromString(par("antennaPolarization").stringValue());
}

double NtnPlatformAntennaModel::computeGain(double angle, double frequency, double diameter, double efficiency) const
{
    if (frequency <= 0.0)
        throw omnetpp::cRuntimeError("NtnPlatformAntennaModel::computeGain - frequency must be positive");
    if (angle < 0.0 || angle > 90.0)
        throw omnetpp::cRuntimeError("NtnPlatformAntennaModel::computeGain - angle must be in the [0, 90] degree range");

    double lambda = SPEED_OF_LIGHT / frequency;
    double apertureRadius = diameter / 2.0;
    double ka = 2.0 * M_PI * apertureRadius / lambda;
    double maxGain = inet::math::fraction2dB(efficiency * ka * ka);

    double theta = inet::math::deg2rad(angle);
    double x = ka * std::sin(theta);

    if (std::abs(x) < 1e-9)
        return maxGain;

    double j = ::j1(x) / x;
    double normalizedGain = 4.0 * j * j;
    return maxGain + inet::math::fraction2dB(normalizedGain);
}

}
