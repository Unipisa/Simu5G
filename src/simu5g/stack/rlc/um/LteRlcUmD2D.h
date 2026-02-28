//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTERLCUMD2D_H_
#define _LTE_LTERLCUMD2D_H_

#include "simu5g/common/utils/utils.h"
#include "simu5g/stack/rlc/um/LteRlcUm.h"

namespace simu5g {

using namespace omnetpp;

/**
 * @class LteRlcUmD2D
 * @brief UM Module
 *
 * This is the UM Module of RLC (with support for D2D)
 *
 */
// Empty subclass — all D2D logic has been merged into LteRlcUm (guarded by hasD2DSupport_ flag).
// Kept temporarily for NED type compatibility; will be removed in a subsequent step.
class LteRlcUmD2D : public LteRlcUm
{
};

} //namespace

#endif

