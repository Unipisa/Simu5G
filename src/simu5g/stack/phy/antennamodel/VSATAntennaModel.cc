#include "simu5g/stack/phy/antennamodel/VSATAntennaModel.h"
#include "inet/common/INETMath.h"
#ifdef __APPLE__
// in MacOS, bessel functions do not come with clang, so we need to include boost libraries
#include <boost/math/special_functions/bessel.hpp>
#endif

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
}

double VSATAntennaModel::computeGain(double angle, double frequency) const
{
    double lambda = SPEED_OF_LIGHT / frequency;
    double v = M_PI * (diameter_ / lambda);
    double maxGain = inet::math::fraction2dB(efficiency_ * v * v);

    // compute actual gain via Bessel function

    double theta = inet::math::deg2rad(angle);
    double x = M_PI * (diameter_ / lambda) * std::sin(theta);

    if (std::abs(x) < 1e-9)
        return maxGain;

    #ifdef __APPLE__
    double bessel = boost::math::cyl_bessel_j(1, x);
    #else
    double bessel = std::cyl_bessel_j(1, x);
    #endif

    double d = (2.0 * bessel) / x;
    double besselFactor = d * d;

    return maxGain + inet::math::fraction2dB(besselFactor);
}

}
