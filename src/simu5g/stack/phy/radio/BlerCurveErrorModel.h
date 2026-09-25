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

#ifndef STACK_PHY_RADIO_BLERCURVEERRORMODEL_H_
#define STACK_PHY_RADIO_BLERCURVEERRORMODEL_H_

#include <optional>
#include <vector>

#include <omnetpp.h>
#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/LteCommon.h"

namespace simu5g {

using namespace omnetpp;

class Binder;

/**
 * See the NED documentation of BlerCurveErrorModel.
 */
class BlerCurveErrorModel : public cSimpleModule
{
  protected:
    inet::ModuleRefByPar<Binder> binder_;
    double harqReduction_ = NAN;

  protected:
    virtual void initialize() override;

  public:
    /**
     * The packet error rate of a reception with the given CQI, per-band SINR
     * (dB) and allocation, at the given transmission attempt (1 for the first
     * transmission); nullopt if the reception is lost for certain, without a
     * decision to draw.
     */
    virtual std::optional<double> computePacketErrorRate(Cqi cqi, const std::vector<double>& sinrPerBand, const RbMap& grantedBlocks, unsigned char transmissionAttempt) const;
};

} // namespace simu5g

#endif
