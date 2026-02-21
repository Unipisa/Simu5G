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

#ifndef _LTE_LTEPDCPRRCENBD2D_H_
#define _LTE_LTEPDCPRRCENBD2D_H_

#include "simu5g/common/LteDefs.h"
#include "simu5g/stack/pdcp/LtePdcp.h"

namespace simu5g {

using namespace omnetpp;

/**
 * @class LtePdcp
 * @brief PDCP Layer
 *
 * This is the PDCP/RRC layer of the LTE Stack (with D2D support).
 *
 */
class LtePdcpEnbD2D : public LtePdcpEnb
{
};

} //namespace

#endif

