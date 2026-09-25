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

#include "simu5g/stack/phy/radio/CellularAntenna.h"

namespace simu5g {

Define_Module(CellularAntenna);

void CellularAntenna::initialize()
{
    gain_ = par("gain");
    const char *txDirection = par("txDirection");
    txDirection_ = static_cast<TxDirectionType>(cEnum::get("simu5g::TxDirectionType")->lookup(txDirection));
    switch (txDirection_) {
        case OMNI: txAngle_ = 0.0;
            break;
        case ANISOTROPIC: txAngle_ = par("txAngle");
            break;
        default: throw cRuntimeError("unknown txDirection: '%s'", txDirection);
    }
}

} // namespace simu5g
