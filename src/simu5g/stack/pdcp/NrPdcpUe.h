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

#ifndef _NRPDCPRRCUE_H_
#define _NRPDCPRRCUE_H_

#include "simu5g/common/LteDefs.h"
#include "simu5g/stack/pdcp/LtePdcpUeD2D.h"

namespace simu5g {

/**
 * @class NrPdcpUe
 * @brief PDCP Layer
 *
 * This is the PDCP/RRC layer of NR Stack
 *
 */
class NrPdcpUe : public LtePdcpUeD2D
{
  protected:
    // this function overrides the one in the base class because we need to distinguish the nodeId of the sender
    // i.e. whether to use the NR nodeId or the LTE one
    Direction getDirection(MacNodeId srcId, MacNodeId destId)
    {
        if (binder_->getD2DCapability(srcId, destId) && binder_->getD2DMode(srcId, destId) == DM)
            return D2D;
        return UL;
    }

};

} //namespace

#endif
