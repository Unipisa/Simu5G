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

#ifndef STACK_PHY_CHANNELMODEL_PATHLOSSMODEL_H_
#define STACK_PHY_CHANNELMODEL_PATHLOSSMODEL_H_

#include "simu5g/common/LteCommon.h"

namespace simu5g {

using namespace omnetpp;

/**
 * Stateless propagation-formula strategy: one concrete subclass per 3GPP
 * propagation study (TR 36.814, TR 36.873, TR 38.901). A channel-model shell
 * owns an instance, feeds it the deployment-scenario parameters once at
 * initialize() time, and calls it per link with the geometry (both
 * distances, LOS state) the shell has already resolved.
 *
 * All per-link state -- LOS map, shadowing history, fading maps, position
 * history -- stays in the owning channel model; an instance of this class
 * holds nothing but the cached scenario parameters and a back-pointer to the
 * owner for RNG draws, so that every random draw a formula makes still comes
 * from the same RNG stream as before.
 */
class PathLossModel
{
  protected:
    cComponent *owner_ = nullptr;  // RNG + parameter context

    DeploymentScenario scenario_ = UNKNOWN_SCENARIO;
    double hNodeB_ = 0;
    double hUe_ = 0;
    double hBuilding_ = 0;
    double wStreet_ = 0;
    bool inside_building_ = false;
    double inside_distance_ = 0;
    double carrierFrequencyHz_ = 0;
    double carrierFrequencyGHz_ = 0;
    double log10CarrierFrequencyGHz_ = 0;
    bool tolerateMaxDistViolation_ = false;
    double environmentHeight_ = 1.0;    // h_E of the link being evaluated, see setEnvironmentHeight()

    /*
     * Propagation velocity in free space, as the breakpoint-distance notes of
     * all three reports define it. This is the rounded 3.0e8 m/s they write,
     * not the exact speed of light: the breakpoint distance appears in the
     * beyond-breakpoint path-loss formulas themselves and not only in the
     * branch condition, so the two choices differ by about 0.005 dB on every
     * LOS link past a breakpoint.
     */
    static constexpr double PROPAGATION_VELOCITY = 3.0e8;

  public:
    virtual ~PathLossModel() {}

    /*
     * Copy the deployment-scenario parameters the formulas need. Parameter
     * ownership stays with the owning channel model; this only receives the
     * values it has already read from its NED parameters.
     */
    virtual void initialize(cComponent *owner, DeploymentScenario scenario,
            double hNodeB, double hUe, double hBuilding, double wStreet,
            bool insideBuilding, double insideDistance,
            double carrierFrequencyHz, double carrierFrequencyGHz, double log10CarrierFrequencyGHz,
            bool tolerateMaxDistViolation);

    /*
     * Set whether the UE end of the link about to be evaluated is inside a
     * building, and how far inside
     */
    virtual void setIndoor(bool insideBuilding, double insideDistance)
    {
        inside_building_ = insideBuilding;
        inside_distance_ = insideDistance;
    }

    /*
     * Draw the effective environment height h_E of a link between a base
     * station site and a UE (TR 38.901 Table 7.4.1-1 NOTE 1, TR 36.873 7.2.1
     * NOTE 4), at the given 2D distance. 1 m, without a draw, where the study
     * and scenario have no other value or the probability of 1 m is 1.
     */
    virtual double drawEnvironmentHeight(double d2D) { return 1.0; }

    /*
     * Set the h_E of the link about to be evaluated
     */
    virtual void setEnvironmentHeight(double environmentHeight) { environmentHeight_ = environmentHeight; }

    /*
     * Compute the path-loss attenuation according to the selected scenario.
     * A concrete model that needs only one of the two distances ignores the
     * other.
     */
    virtual double computePathLoss(double d3D, double d2D, bool los) = 0;

    /*
     * Compute the LOS probability according to the selected scenario.
     * Returns the probability p; the caller draws against it and stores the
     * outcome.
     */
    virtual double computeLosProbability(double d3D, double d2D) = 0;

    /*
     * Compute the standard deviation of the log-normal shadowing according
     * to the selected scenario and the LOS state of the link.
     */
    virtual double getShadowingStdDev(double d3D, double d2D, bool losState) = 0;

    /*
     * Compute the attenuation caused by the antenna pattern for a given
     * reception direction.
     */
    virtual double computeAngularAttenuation(double hAngle, double vAngle) = 0;
};

} //namespace

#endif
