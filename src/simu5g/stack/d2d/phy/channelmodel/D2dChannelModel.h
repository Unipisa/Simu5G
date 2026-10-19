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

#ifndef STACK_D2D_PHY_CHANNELMODEL_D2DCHANNELMODEL_H_
#define STACK_D2D_PHY_CHANNELMODEL_D2DCHANNELMODEL_H_

#include "simu5g/stack/d2d/phy/channelmodel/ID2dChannelModel.h"
#include "simu5g/stack/phy/channelmodel/StochasticChannelModel.h"

namespace simu5g {

/**
 * The channel model of D2D-capable networks: StochasticChannelModel, which
 * evaluates D2D links itself, under the interface the D2D UE PHY's capture
 * effect asks for a frame's RSRP through.
 */
class D2dChannelModel : public StochasticChannelModel, public ID2dChannelModel
{
  public:
    // ---- ID2dChannelModel ----
    std::vector<double> getRSRP_D2D(AirFrame *frame, UserControlInfo *lteInfo_1, MacNodeId destId, inet::Coord destCoord) override
    {
        return StochasticChannelModel::getRSRP_D2D(frame, lteInfo_1, destId, destCoord);
    }
};

} //namespace

#endif /* STACK_D2D_PHY_CHANNELMODEL_D2DCHANNELMODEL_H_ */
