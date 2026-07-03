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

#ifndef STACK_PHY_ERRORMODEL_EESMERRORMODEL_H_
#define STACK_PHY_ERRORMODEL_EESMERRORMODEL_H_

#include "simu5g/stack/phy/errormodel/ErrorModel.h"

namespace simu5g {

class EesmErrorModel : public ErrorModel
{
  protected:
    double beta_ = NAN;
    bool useLambdaCalibration_ = true;

  protected:
    void initialize(int stage) override;
    double computePacketErrorRate(LteAirFrame *frame, UserControlInfo *lteInfo, const std::vector<double>& snrVector, LteChannelModel *channelModel, const ReceptionParams& params, bool useD2DMulticastThreshold, bool& forcedFailure) const override;
};

} //namespace

#endif
