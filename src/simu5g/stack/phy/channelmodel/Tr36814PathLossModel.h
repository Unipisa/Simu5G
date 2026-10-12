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

#ifndef STACK_PHY_CHANNELMODEL_TR36814PATHLOSSMODEL_H_
#define STACK_PHY_CHANNELMODEL_TR36814PATHLOSSMODEL_H_

#include "simu5g/stack/phy/channelmodel/PathLossModel.h"

namespace simu5g {

/**
 * Path-loss, LOS-probability and shadowing-sigma formulas of
 * 3GPP TR 36.814, "Further advancements for E-UTRA physical layer aspects",
 * v9.2.0, March 2017, for the Indoor Hotspot (InH), Urban Microcell (UMi),
 * Urban Macrocell (UMa), Suburban Macrocell (SMa) and Rural Macrocell (RMa)
 * deployment scenarios.
 *
 * Every formula here works on the 3D coordinate distance; computePathLoss
 * and computeLosProbability ignore their d2D argument entirely.
 */
class Tr36814PathLossModel : public PathLossModel
{
  public:
    double computePathLoss(double d3D, double d2D, bool los, const LinkConditions& conditions) override;
    double computeLosProbability(double d3D, double d2D) override;
    double getShadowingStdDev(double d3D, double d2D, bool losState) override;
    double computeAngularAttenuation(double hAngle, double vAngle) override;

  protected:
    /*
     * Select the shadowing sigma for the current scenario given whether the
     * link is below the scenario's breakpoint distance and its LOS state.
     * getShadowingStdDev() computes the breakpoint comparison itself and
     * calls this; Tr38901PathLossModel reuses it for the scenarios TR 38.901
     * does not cover, after computing its own (differently-defined) breakpoint.
     */
    double selectStdDev(bool belowBreakpoint, bool losState);

  private:
    double computeIndoor(double d, bool los);
    double computeUrbanMicro(double d, bool los);
    double computeUrbanMacro(double d, bool los);
    double computeSubUrbanMacro(double d, double& dbp, bool los);
    double computeRuralMacro(double d, double& dbp, bool los);
};

} //namespace

#endif
