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

#include "simu5g/stack/phy/channelmodel/Tr38901PathLossModel.h"

namespace simu5g {

using namespace omnetpp;

double Tr38901PathLossModel::computePathLoss(double d3D, double d2D, bool los)
{
    // compute attenuation based on selected scenario and based on LOS or NLOS
    double pathLoss = 0;
    switch (scenario_) {
        case INDOOR_HOTSPOT:
            pathLoss = computeIndoor3D(d3D, d2D, los);
            break;
        case URBAN_MICROCELL:
            pathLoss = computeUrbanMicro3D(d3D, d2D, los);
            break;
        case URBAN_MACROCELL:
            pathLoss = computeUrbanMacro3D(d3D, d2D, los);
            break;
        case RURAL_MACROCELL:
            pathLoss = computeRuralMacro3D(d3D, d2D, los);
            break;
        default:
            return Tr36873PathLossModel::computePathLoss(d3D, d2D, los);
    }
    return pathLoss;
}

double Tr38901PathLossModel::computeLosProbability(double d3D, double d2D)
{
    double d = d2D;
    double p = 0;
    switch (scenario_) {
        case URBAN_MICROCELL:
            if (d <= 18.0)
                p = 1.0;
            else
                p = (18.0 / d) + exp(-1 * d / 36.0) * (1.0 - (18.0 / d));
            break;
        case URBAN_MACROCELL:
            if (d <= 18.0)
                p = 1.0;
            else {
                double C = (hUe_ <= 13.0) ? 0 : pow((hUe_ - 13.0) / 10.0, 1.5);
                p = ((18 / d) + exp(-1 * d / 63) * (1 - (18 / d))) * (1 + C * (5.0 / 4.0) * pow(d / 100.0, 3) * exp(-1 * d / 150.0));
            }
            break;
        case RURAL_MACROCELL:
            if (d <= 10)
                p = 1;
            else
                p = exp(-1 * (d - 10.0) / 1000);
            break;
        case INDOOR_HOTSPOT:
            if (d <= 5.0)
                p = 1;
            else if (d <= 49.0)
                p = exp(-1 * (d - 5.0) / 70.8);
            else
                p = 0.54 * exp(-1 * (d - 49.0) / 211.7);
            break;
        default:
            return Tr36873PathLossModel::computeLosProbability(d3D, d2D);
    }
    return p;
}

double Tr38901PathLossModel::getShadowingStdDev(double d3D, double d2D, bool losState)
{
    // Breakpoint distance of the RMa path loss (Q3: TR 38.901 uses
    // 2*pi*hNodeB*hUe*fc/c, not the base classes' 4*(hNodeB-1)*(hUe-1)*fc/c);
    // the LOS branch of that scenario uses different sigma values below and
    // above it. The comparison uses the 2D distance (Q1), like the base
    // classes' shadowing breakpoint.
    double dbp = 2 * M_PI * hNodeB_ * hUe_ * (carrierFrequencyHz_ / PROPAGATION_VELOCITY);
    bool belowBreakpoint = d2D < dbp;

    switch (scenario_) {
        case URBAN_MICROCELL:
            if (losState)
                return 4.;
            else
                return 7.82;
        case INDOOR_HOTSPOT:
            if (losState)
                return 3.;
            else
                return 8.03;
        case URBAN_MACROCELL:
            if (losState)
                return 4.;
            else
                return 6.;
        case RURAL_MACROCELL:
            if (losState) {
                if (belowBreakpoint)
                    return 4.;
                else
                    return 6.;
            }
            else
                return 8.;
        default:
            // scenarios TR 38.901 does not cover fall back to the shared
            // sigma table, still keyed off this class's own breakpoint.
            return selectStdDev(belowBreakpoint, losState);
    }
    return 0.0;
}

double Tr38901PathLossModel::computePenetrationLoss(double threeDimDistance)
{
    double inside_distance = (inside_distance_ < threeDimDistance) ? inside_distance_ : threeDimDistance;
    double pLoss_in = 0.5 * inside_distance;

    // Which through-wall model applies is a function of the scenario. Table
    // 7.4.3-3 -- a flat 20 dB whose sigma_P is 0, kept for backwards
    // compatibility with TR 36.873 -- is offered for UMa and UMi
    // single-frequency simulations below 6 GHz. TR 38.901 defines no O2I loss
    // for the indoor-office scenario at all, so it follows the urban ones for
    // want of anything to derive.
    bool singleFrequencyModel = carrierFrequencyGHz_ <= 6.0
        && (scenario_ == URBAN_MACROCELL || scenario_ == URBAN_MICROCELL
            || scenario_ == INDOOR_HOTSPOT);
    if (singleFrequencyModel)
        return 20.0 + pLoss_in;

    // Otherwise table 7.4.3-2, whose high-loss variant is not offered for RMa.
    double Lconcrete = 5 + 4 * carrierFrequencyGHz_;
    double pLoss_tw = 0.0;
    if (useBuildingPenetrationHighLossModel_ && scenario_ != RURAL_MACROCELL) {
        double LiirGlass = 23 + 0.3 * carrierFrequencyGHz_;
        pLoss_tw = 5 - 10 * log10(0.7 * pow(10, (-LiirGlass / 10)) + 0.3 * pow(10, (-Lconcrete / 10))) + owner_->normal(0.0, 6.5);
    }
    else {
        double Lglass = 2 + 0.2 * carrierFrequencyGHz_;
        pLoss_tw = 5 - 10 * log10(0.3 * pow(10, (-Lglass / 10)) + 0.7 * pow(10, (-Lconcrete / 10))) + owner_->normal(0.0, 4.4);
    }
    return pLoss_tw + pLoss_in;
}

double Tr38901PathLossModel::drawEnvironmentHeight(double d2D)
{
    if (scenario_ != URBAN_MACROCELL)
        return 1.0;
    double G_2d = (d2D < 18.0) ? 0 : (5.0 / 4.0) * pow(d2D / 100.0, 3) * exp(-d2D / 150);
    double C = (hUe_ < 13.0) ? 0 : pow(((hUe_ - 13.0) / 10.0), 1.5) * G_2d;
    double prob = 1.0 / (1.0 + C);
    // drawn only when it can come out otherwise
    if (prob >= 1.0 || owner_->uniform(0.0, 1.0) < prob)
        return 1.0;
    double bound = hUe_ - 1.5;
    std::vector<double> hVec;
    for (double h = 12; h < bound; h += 3)
        hVec.push_back(h);
    hVec.push_back(bound);
    return hVec.at(owner_->intuniform(0, hVec.size() - 1));
}

double Tr38901PathLossModel::computeUrbanMacro3D(double threeDimDistance, double twoDimDistance, bool los)
{
    if (twoDimDistance < 10)
        twoDimDistance = 10;

    if (threeDimDistance < 10)
        threeDimDistance = 10;

    if (twoDimDistance > 5000 && !tolerateMaxDistViolation_)
        throw cRuntimeError("Error: Urban macrocell path loss model is valid for d<5000m only");

    // Compute penetration loss
    double penetrationLoss = 0.0;
    if (inside_building_)
        penetrationLoss = computePenetrationLoss(threeDimDistance);

    // Compute break-point distance
    double hEnvir = environmentHeight_;
    double hNodeB = hNodeB_ - hEnvir;
    double hUe = hUe_ - hEnvir;
    double dbp = 4 * hNodeB * hUe * (carrierFrequencyHz_  / PROPAGATION_VELOCITY);

    // Compute LOS path loss
    double pLoss_los = 0.0;
    if (twoDimDistance < dbp)
        pLoss_los = 28 + 22 * log10(threeDimDistance) + 20 * log10CarrierFrequencyGHz_;
    else
        pLoss_los = 28 + 40 * log10(threeDimDistance) + 20 * log10CarrierFrequencyGHz_ - 9 * log10((dbp * dbp + (hNodeB_ - hUe_) * (hNodeB_ - hUe_)));
    pLoss_los += penetrationLoss;

    double pLoss = 0.0;
    if (los)
        pLoss = pLoss_los;
    else {
        // Compute NLOS path loss
        double pLoss_nlos = 13.54 + 39.08 * log10(threeDimDistance) + 20 * log10CarrierFrequencyGHz_ - 0.6 * (hUe_ - 1.5);
        pLoss_nlos += penetrationLoss;

        pLoss = (pLoss_los > pLoss_nlos) ? pLoss_los : pLoss_nlos;
    }
    return pLoss;
}

double Tr38901PathLossModel::computeUrbanMicro3D(double threeDimDistance, double twoDimDistance, bool los)
{
    if (twoDimDistance < 10)
        twoDimDistance = 10;

    if (twoDimDistance > 5000 && !tolerateMaxDistViolation_)
        throw cRuntimeError("Error: Urban microcell path loss model is valid for d<5000m only");

    // Compute penetration loss
    double penetrationLoss = 0.0;
    if (inside_building_)
        penetrationLoss = computePenetrationLoss(threeDimDistance);

    // Compute break-point distance
    double hEnvir = 1.0;
    double hNodeB = hNodeB_ - hEnvir;
    double hUe = hUe_ - hEnvir;
    double dbp = 4 * hNodeB * hUe * (carrierFrequencyHz_  / PROPAGATION_VELOCITY);

    // Compute LOS path loss
    double pLoss_los = 0.0;
    if (twoDimDistance < dbp)
        pLoss_los = 32.4 + 21 * log10(threeDimDistance) + 20 * log10CarrierFrequencyGHz_;
    else
        pLoss_los = 32.4 + 40 * log10(threeDimDistance) + 20 * log10CarrierFrequencyGHz_ - 9.5 * log10((dbp * dbp + (hNodeB_ - hUe_) * (hNodeB_ - hUe_)));
    pLoss_los += penetrationLoss;

    double pLoss = 0.0;
    if (los)
        pLoss = pLoss_los;
    else {
        // Compute NLOS path loss
        double pLoss_nlos = 35.3 * log10(threeDimDistance) + 22.4 + 21.3 * log10CarrierFrequencyGHz_ - 0.3 * (hUe_ - 1.5);
        pLoss_nlos += penetrationLoss;

        pLoss = (pLoss_los > pLoss_nlos) ? pLoss_los : pLoss_nlos;
    }
    return pLoss;
}

double Tr38901PathLossModel::computeRuralMacro3D(double threeDimDistance, double twoDimDistance, bool los)
{
    if (twoDimDistance < 10)
        twoDimDistance = 10;

    if (los) {
        if (twoDimDistance > 10000 && !tolerateMaxDistViolation_)
            throw cRuntimeError("Error: LOS rural macrocell path loss model is valid for d<10000m only");
    }
    else {
        if (twoDimDistance > 5000 && !tolerateMaxDistViolation_)
            throw cRuntimeError("Error: NLOS rural macrocell path loss model is valid for d<5000m only");
    }
    // Compute penetration loss
    double penetrationLoss = 0.0;
    if (inside_building_) {
        penetrationLoss = computePenetrationLoss(threeDimDistance);
    }

    // Compute break-point distance
    double dbp = 2 * M_PI * hNodeB_ * hUe_ * (carrierFrequencyHz_  / PROPAGATION_VELOCITY);

    double h = 5.0; // Average building height
    double A = 0.03 * pow(h, 1.72);
    double B = 0.044 * pow(h, 1.72);
    double min1 = (A < 10) ? A : 10;
    double min2 = (B < 14.77) ? B : 14.77;
    double pLoss_los = 0.0;
    if (twoDimDistance < dbp) {
        pLoss_los = 20 * log10(40 * M_PI * threeDimDistance * (carrierFrequencyGHz_ / 3.0)) + min1 * log10(threeDimDistance) - min2 + 0.002 * log10(h) * threeDimDistance;
    }
    else {
        pLoss_los = 20 * log10(40 * M_PI * dbp * (carrierFrequencyGHz_ / 3.0)) + min1 * log10(dbp) - min2 + 0.002 * log10(h) * dbp
            + 40 * log10(threeDimDistance / dbp);
    }
    pLoss_los += penetrationLoss;

    double pLoss = 0.0;
    if (los)
        pLoss = pLoss_los;
    else {
        double W = 20.0;  // Average street width
        double pLoss_nlos = 161.04 - 7.1 * log10(W) + 7.5 * log10(h) - (24.37 - 3.7 * pow(h / hNodeB_, 2)) * log10(hNodeB_)
            + (43.42 - 3.1 * log10(hNodeB_)) * (log10(threeDimDistance) - 3.0) + 20 * log10CarrierFrequencyGHz_
            - (3.2 * pow((log10(11.75 * hUe_)), 2) - 4.97);
        pLoss_nlos += penetrationLoss;
        pLoss = (pLoss_los > pLoss_nlos) ? pLoss_los : pLoss_nlos;
    }

    return pLoss;
}

double Tr38901PathLossModel::computeIndoor3D(double threeDimDistance, double twoDimDistance, bool los)
{
    if (threeDimDistance < 1)
        threeDimDistance = 1;

    if (threeDimDistance > 150 && !tolerateMaxDistViolation_)
        throw cRuntimeError("Error: Indoor hotspot path loss model is valid for d<150m only");

    // Compute penetration loss
    double penetrationLoss = 0.0;
    if (inside_building_)
        penetrationLoss = computePenetrationLoss(threeDimDistance);

    // Compute LOS path loss
    double pLoss_los = 32.4 + 17.3 * log10(threeDimDistance) + 20 * log10CarrierFrequencyGHz_;
    pLoss_los += penetrationLoss;

    double pLoss = 0.0;
    if (los)
        pLoss = pLoss_los;
    else {
        // Compute NLOS path loss
        double pLoss_nlos = 38.3 * log10(threeDimDistance) + 17.3 + 24.9 * log10CarrierFrequencyGHz_;
        pLoss_nlos += penetrationLoss;

        pLoss = (pLoss_los > pLoss_nlos) ? pLoss_los : pLoss_nlos;
    }
    return pLoss;
}

} //namespace
