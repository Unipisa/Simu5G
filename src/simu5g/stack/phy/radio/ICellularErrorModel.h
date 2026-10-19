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

#ifndef STACK_PHY_RADIO_ICELLULARERRORMODEL_H_
#define STACK_PHY_RADIO_ICELLULARERRORMODEL_H_

#include <optional>
#include <vector>

#include "simu5g/common/LteCommon.h"

namespace simu5g {

/**
 * The error model of a cellular receiver (see the NED module interface
 * ICellularErrorModel): the packet error rate of a reception, which the
 * receiver decides the reception against.
 */
class ICellularErrorModel
{
  public:
    virtual ~ICellularErrorModel() = default;

    /**
     * The packet error rate of a reception in the given direction, with the
     * given CQI, per-band SINR (dB) and allocation, at the given transmission
     * attempt (1 for the first transmission); nullopt if the reception is lost
     * for certain, without a decision to draw.
     */
    virtual std::optional<double> computePacketErrorRate(Direction direction, Cqi cqi, const std::vector<double>& sinrPerBand, const RbMap& grantedBlocks,
            unsigned char transmissionAttempt) const = 0;
};

} // namespace simu5g

#endif
