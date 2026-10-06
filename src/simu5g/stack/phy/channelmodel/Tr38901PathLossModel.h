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

#ifndef STACK_PHY_CHANNELMODEL_TR38901PATHLOSSMODEL_H_
#define STACK_PHY_CHANNELMODEL_TR38901PATHLOSSMODEL_H_

#include "simu5g/stack/phy/channelmodel/Tr36873PathLossModel.h"

namespace simu5g {

/**
 * Path-loss, LOS-probability and shadowing-sigma formulas of 3GPP TR 38.901,
 * "Study on channel model for frequencies from 0.5 to 100 GHz", v16.1.0,
 * December 2019, for the Indoor Hotspot (InH), Urban Microcell (UMi-Street
 * Canyon), Urban Macrocell (UMa) and Rural Macrocell (RMa) deployment
 * scenarios, including the building-penetration loss for indoor UEs
 * (section 7.4.3).
 *
 * Scenario coverage is partial: Suburban Macrocell is not covered here and
 * falls through to the TR 36.873 formulas this class extends (which in turn
 * fall through to TR 36.814 for what neither covers). getShadowingStdDev()
 * computes this study's own breakpoint distance (Q3: 2*pi*hNodeB*hUe*fc/c,
 * unlike the base classes' 4*(hNodeB-1)*(hUe-1)*fc/c) and, for the scenarios
 * it does not cover, applies it against the shared TR 36.814 sigma table via
 * selectStdDev() rather than falling through to TR 36.873's
 * getShadowingStdDev() (which does not exist -- TR 36.873 has no shadowing
 * override of its own) or recomputing the base breakpoint.
 */
class Tr38901PathLossModel : public Tr36873PathLossModel
{
  public:
    double computePathLoss(double d3D, double d2D, bool los) override;
    double computeLosProbability(double d3D, double d2D) override;
    double getShadowingStdDev(double d3D, double d2D, bool losState) override;
    double drawEnvironmentHeight(double d2D) override;

    /*
     * Select the low-loss (default) or high-loss building-penetration model
     * of table 7.4.3-2 of TR 38.901 for the NLOS-through-wall term. Set by
     * the owning channel model from its "useBuildingPenetrationHighLossModel"
     * parameter.
     */
    void setUseBuildingPenetrationHighLossModel(bool value) { useBuildingPenetrationHighLossModel_ = value; }

  private:
    double computeIndoor3D(double threeDimDistance, double twoDimDistance, bool los);
    double computeUrbanMicro3D(double threeDimDistance, double twoDimDistance, bool los);
    double computeUrbanMacro3D(double threeDimDistance, double twoDimDistance, bool los);
    double computeRuralMacro3D(double threeDimDistance, double twoDimDistance, bool los);

    /*
     * Compute the building penetration loss for an indoor UE.
     * See section 7.4.3 of TR 38.901.
     */
    double computePenetrationLoss(double threeDimDistance);

    // flag for using the high-loss or low-loss building-penetration model
    // see table 7.4.3-2 in TR 38.901
    bool useBuildingPenetrationHighLossModel_ = false;
};

} //namespace

#endif
