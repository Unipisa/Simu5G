//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _NRPDCPRRCENB_H_
#define _NRPDCPRRCENB_H_

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/LteDefs.h"
#include "simu5g/stack/pdcp/NrTxPdcpEntity.h"
#include "simu5g/stack/pdcp/LtePdcpEnbD2D.h"
#include "simu5g/stack/dualConnectivityManager/DualConnectivityManager.h"

namespace simu5g {

/**
 * @class NrPdcpEnb
 * @brief PDCP Layer
 *
 * This is the PDCP/RRC layer of NR Stack
 *
 */
class NrPdcpEnb : public LtePdcpEnbD2D
{

  protected:

    // Reference to the Dual Connectivity Manager
    inet::ModuleRefByPar<DualConnectivityManager> dualConnectivityManager_;

    void initialize(int stage) override;

};

} //namespace

#endif

