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

#ifndef STACK_PHY_CHANNELMODEL_TR36873PATHLOSSMODEL_H_
#define STACK_PHY_CHANNELMODEL_TR36873PATHLOSSMODEL_H_

#include "simu5g/stack/phy/channelmodel/Tr36814PathLossModel.h"

namespace simu5g {

/**
 * Path-loss and LOS-probability formulas of 3GPP TR 36.873, "Study on 3D
 * channel model for LTE", v12.7.0, December 2017, for the Indoor Hotspot
 * (InH), Urban Microcell (UMi), Urban Macrocell (UMa) and Rural Macrocell
 * (RMa) deployment scenarios.
 *
 * Path loss is evaluated over the 3D distance between the endpoints, while
 * the LOS probability and the scenario validity limits remain functions of
 * the 2D distance, so the base-station and UE heights enter the result
 * through both.
 *
 * Scenario coverage is partial: Suburban Macrocell path loss and Indoor
 * Hotspot / Suburban Macrocell LOS probability are not covered here and
 * fall through to the TR 36.814 formulas this class extends. The shadowing
 * sigma table is shared with TR 36.814 too (getShadowingStdDev() only
 * overrides which distance the breakpoint comparison uses).
 *
 * computeAngularAttenuation() adds a vertical antenna-pattern component
 * alongside the horizontal one TR 36.814 alone has; Tr38901PathLossModel
 * inherits it unchanged.
 */
class Tr36873PathLossModel : public Tr36814PathLossModel
{
  public:
    double computePathLoss(double d3D, double d2D, bool los) override;
    double computeLosProbability(double d3D, double d2D) override;
    double getShadowingStdDev(double d3D, double d2D, bool losState) override;
    double drawEnvironmentHeight(double d2D) override;
    double computeAngularAttenuation(double hAngle, double vAngle) override;

  private:
    double computeIndoor3D(double threeDimDistance, double twoDimDistance, bool los);
    double computeUrbanMicro3D(double threeDimDistance, double twoDimDistance, bool los);
    double computeUrbanMacro3D(double threeDimDistance, double twoDimDistance, bool los);
    double computeRuralMacro3D(double threeDimDistance, double twoDimDistance, bool los);
};

} //namespace

#endif
