//
//                  Simu5G
//
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/phy/radio/ConfiguredRateErrorModel.h"

#include <cmath>

namespace simu5g {

Define_Module(ConfiguredRateErrorModel);

void ConfiguredRateErrorModel::initialize()
{
    perDl_ = &par("perDl");
    perUl_ = &par("perUl");
    perD2D_ = &par("perD2D");
    harqReduction_ = par("harqReduction");
}

std::optional<double> ConfiguredRateErrorModel::computePacketErrorRate(Direction direction, Cqi cqi, const std::vector<double>& sinrPerBand, const RbMap& grantedBlocks,
        unsigned char transmissionAttempt) const
{
    if (transmissionAttempt == 0)
        throw cRuntimeError("ConfiguredRateErrorModel::computePacketErrorRate(): transmission number must be at least 1");

    cPar *per;
    switch (direction) {
        case DL: per = perDl_; break;
        case UL: per = perUl_; break;
        case D2D:
        case D2D_MULTI: per = perD2D_; break;
        default:
            throw cRuntimeError("ConfiguredRateErrorModel::computePacketErrorRate(): unexpected direction %d", (int)direction);
    }
    return per->doubleValue() * pow(harqReduction_, transmissionAttempt - 1);
}

} // namespace simu5g
