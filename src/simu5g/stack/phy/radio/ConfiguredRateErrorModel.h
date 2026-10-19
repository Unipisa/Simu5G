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

#ifndef STACK_PHY_RADIO_CONFIGUREDRATEERRORMODEL_H_
#define STACK_PHY_RADIO_CONFIGUREDRATEERRORMODEL_H_

#include <omnetpp.h>

#include "simu5g/stack/phy/radio/ICellularErrorModel.h"

namespace simu5g {

using namespace omnetpp;

/**
 * See the NED documentation of ConfiguredRateErrorModel.
 */
class ConfiguredRateErrorModel : public cSimpleModule, public ICellularErrorModel
{
  protected:
    // volatile, so that the error rate can be made a function of simulation time --
    // which is how a coverage loss is scripted
    cPar *perDl_ = nullptr;
    cPar *perUl_ = nullptr;
    cPar *perD2D_ = nullptr;
    double harqReduction_ = NAN;

  protected:
    virtual void initialize() override;

  public:
    /**
     * The configured error rate of the direction, reduced by harqReduction for
     * every retransmission; the CQI, the SINR and the allocation are not used.
     * Never nullopt.
     */
    std::optional<double> computePacketErrorRate(Direction direction, Cqi cqi, const std::vector<double>& sinrPerBand, const RbMap& grantedBlocks,
            unsigned char transmissionAttempt) const override;
};

} // namespace simu5g

#endif
