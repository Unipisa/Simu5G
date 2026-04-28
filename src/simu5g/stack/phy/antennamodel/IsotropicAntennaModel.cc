#include "simu5g/stack/phy/antennamodel/IsotropicAntennaModel.h"

namespace simu5g {

Define_Module(IsotropicAntennaModel);

void IsotropicAntennaModel::initialize()
{
    txPower_ = par("txPower");
    txFeederLoss_ = par("txFeederLoss");
    rxFeederLoss_ = par("rxFeederLoss");
    rxLumpedLoss_ = par("rxLumpedLoss");
    noiseFigure_ = par("noiseFigure");
}

}
