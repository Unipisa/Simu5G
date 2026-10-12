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

#ifndef _LTE_ID2DCHANNELMODEL_H_
#define _LTE_ID2DCHANNELMODEL_H_

#include <vector>

#include <inet/common/geometry/common/Coord.h>

#include "simu5g/common/LteCommon.h"

namespace simu5g {

class AirFrame;
class UserControlInfo;

/*
 * Interface implemented by the D2D-capable channel model (D2dChannelModel).
 *
 * D2D PHY code talks to the channel model's D2D reception machinery through
 * this interface instead of the concrete channel-model class, so that the core
 * channel model (StochasticChannelModel) carries no D2D code. Obtain it with
 * check_and_cast<ID2dChannelModel *>(channelModel): a D2D NIC always wires a D2D
 * channel model into its channelModel slots.
 */
class ID2dChannelModel
{
  public:
    virtual ~ID2dChannelModel() {}

    /// Compute the received useful signal (RSRP) for a D2D transmission, per band.
    virtual std::vector<double> getRSRP_D2D(AirFrame *frame, UserControlInfo *lteInfo_1, MacNodeId destId, inet::Coord destCoord) = 0;
};

} //namespace

#endif
