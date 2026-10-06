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

#include "simu5g/stack/phy/channelmodel/Tr36873PathLossModel.h"

namespace simu5g {

// attenuation value to be returned if the maximum distance of a scenario has been violated
// and tolerating the maximum distance violation is enabled
#define ATT_MAXDISTVIOLATED    1000

using namespace omnetpp;

double Tr36873PathLossModel::computePathLoss(double d3D, double d2D, bool los)
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
            // Suburban Macrocell falls through to the TR 36.814 formula, fed
            // with the 2D distance rather than the 3D distance its own
            // convention would suggest (pinned by the SMa-36873 and
            // SMa-38901 fingerprint rows).
            return Tr36814PathLossModel::computePathLoss(d2D, d2D, los);
    }
    return pathLoss;
}

double Tr36873PathLossModel::computeLosProbability(double d3D, double d2D)
{
    double d = d2D;
    double p = 0;
    switch (scenario_) {
        case URBAN_MICROCELL:
            if (d <= 18.0)
                p = 1.0;
            else
                p = (18 / d) + exp(-1 * d / 36) * (1 - (18 / d));
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
                p = exp(-1 * (d - 10) / 1000);
            break;
        default:
            return Tr36814PathLossModel::computeLosProbability(d3D, d2D);
    }
    return p;
}

double Tr36873PathLossModel::getShadowingStdDev(double d3D, double d2D, bool losState)
{
    // Breakpoint distance of the RMa/SMa path loss, the only scenarios whose
    // LOS branch uses different sigma values below and above it. Unlike
    // TR 36.814, the comparison uses the 2D distance (Q1).
    double dbp = 2 * M_PI * hNodeB_ * hUe_ * (carrierFrequencyHz_ / PROPAGATION_VELOCITY);
    return selectStdDev(d2D < dbp, losState);
}

double Tr36873PathLossModel::computeAngularAttenuation(double hAngle, double vAngle) {

    // --- compute horizontal pattern attenuation --- //
    double angularAttMin = 30;

    // compute attenuation due to horizontal angular position
    double hAngularAtt = 12 * pow(hAngle / 65.0, 2);
    if (hAngularAtt > angularAttMin)
        hAngularAtt = angularAttMin;

    // --- compute vertical pattern attenuation --- //
    double vTilt = 90;
    double vAngularAtt = 12 * pow((vAngle - vTilt) / 65.0, 2);
    if (vAngularAtt > angularAttMin)
        vAngularAtt = angularAttMin;

    double angularAtt = hAngularAtt + vAngularAtt;
    return (angularAtt < angularAttMin) ? angularAtt : angularAttMin;
}

double Tr36873PathLossModel::computeIndoor3D(double threeDimDistance, double twoDimDistance, bool los)
{
    double a, b;
    if (los) {
        if (twoDimDistance > 150 || twoDimDistance < 3)
            throw cRuntimeError("Error: LOS indoor path loss model is valid for 3<d<150");
        a = 16.9;
        b = 32.8;
    }
    else {
        if (twoDimDistance > 250 || twoDimDistance < 6)
            throw cRuntimeError("Error: NLOS indoor path loss model is valid for 6<d<250");
        a = 43.3;
        b = 11.5;
    }
    return a * log10(threeDimDistance) + b + 20 * log10CarrierFrequencyGHz_;
}

double Tr36873PathLossModel::computeUrbanMicro3D(double threeDimDistance, double twoDimDistance, bool los)
{
    if (twoDimDistance < 10)
        twoDimDistance = 10;

    if (twoDimDistance > 5000) {
        if (tolerateMaxDistViolation_)
            return ATT_MAXDISTVIOLATED;
        else
            throw cRuntimeError("Error: urban microcell path loss model is valid for d<5000 m");
    }

    // compute break-point distance
    double hNodeB = hNodeB_ - 1.0;
    double hUe = hUe_ - 1.0;
    double dbp = 4 * hNodeB * hUe * (carrierFrequencyHz_ / PROPAGATION_VELOCITY);

    double pLoss_los = 0.0;
    if (twoDimDistance < dbp)
        pLoss_los = 22 * log10(threeDimDistance) + 28 + 20 * log10CarrierFrequencyGHz_;
    else
        pLoss_los = 40 * log10(threeDimDistance) + 28 + 20 * log10CarrierFrequencyGHz_ - 9 * log10((dbp * dbp + (hNodeB_ - hUe_) * (hNodeB_ - hUe_)));

    if (los)
        return pLoss_los;

    // NLOS case

    if (twoDimDistance > 2000.0) {
        if (tolerateMaxDistViolation_)
            twoDimDistance = 2000.0;
        else
            throw cRuntimeError("Error: NLOS urban microcell path loss model is valid for d<2000 m");
    }

    double pLoss_nlos = 36.7 * log10(threeDimDistance) + 22.7
        + 26 * log10CarrierFrequencyGHz_ - 0.3 * (hUe_ - 1.5);

    return (pLoss_los > pLoss_nlos) ? pLoss_los : pLoss_nlos;
}

double Tr36873PathLossModel::drawEnvironmentHeight(double d2D)
{
    if (scenario_ != URBAN_MACROCELL)
        return 1.0;
    double C = (hUe_ <= 13.0) ? 0 : pow(((hUe_ - 13.0) / 10.0), 1.5);
    double prob = 1.0 / (1.0 + C);
    // drawn only when it can come out otherwise
    if (prob >= 1.0 || owner_->uniform(0.0, 1.0) < prob)
        return 1.0;
    double bound = hUe_ - 1.5;
    std::vector<double> hVec;
    for (double h = 12; h < bound; h += 3)
        hVec.push_back(h);
    hVec.push_back(bound);
    int index = owner_->intuniform(0, hVec.size() - 1);
    return hVec.at(index);
}

double Tr36873PathLossModel::computeUrbanMacro3D(double threeDimDistance, double twoDimDistance, bool los)
{
    if (twoDimDistance < 10)
        twoDimDistance = 10;

    if (threeDimDistance < 10)
        threeDimDistance = 10;

    if (twoDimDistance > 5000) {
        if (tolerateMaxDistViolation_)
            return ATT_MAXDISTVIOLATED;
        else
            throw cRuntimeError("Error: LOS urban macrocell path loss model is valid for d<5000 m");
    }

    // O2I penetration loss of clause 7.2.3: a flat 20 dB through the external
    // wall plus 0.5 dB per metre of the distance inside, at every frequency.
    double penetrationLoss = 0.0;
    if (inside_building_) {
        double inside_distance = (inside_distance_ < threeDimDistance) ? inside_distance_ : threeDimDistance;
        penetrationLoss = 20.0 + 0.5 * inside_distance;
    }

    // compute break-point distance
    double hEnvir = environmentHeight_;

    double hNodeB = hNodeB_ - hEnvir;
    double hUe = hUe_ - hEnvir;

    double dbp = 4 * hNodeB * hUe * (carrierFrequencyHz_  / PROPAGATION_VELOCITY);

    double pLoss_los = 0.0;
    if (twoDimDistance < dbp)
        pLoss_los = 22 * log10(threeDimDistance) + 28 + 20 * log10CarrierFrequencyGHz_;
    else
        pLoss_los = 40 * log10(threeDimDistance) + 28 + 20 * log10CarrierFrequencyGHz_ - 9 * log10((dbp * dbp + (hNodeB_ - hUe_) * (hNodeB_ - hUe_)));

    if (los)
        return pLoss_los + penetrationLoss;

    // NLOS case

    double pLoss_nlos = 161.04 - 7.1 * log10(wStreet_) + 7.5 * log10(hBuilding_)
        - (24.37 - 3.7 * pow(hBuilding_ / hNodeB_, 2)) * log10(hNodeB_)
        + (43.42 - 3.1 * log10(hNodeB_)) * (log10(threeDimDistance) - 3) + 20 * log10CarrierFrequencyGHz_
        - (3.2 * (pow(log10(17.625), 2)) - 4.97) - 0.6 * (hUe_ - 1.5);

    return (pLoss_los > pLoss_nlos) ? pLoss_los + penetrationLoss : pLoss_nlos + penetrationLoss;
}

double Tr36873PathLossModel::computeRuralMacro3D(double threeDimDistance, double twoDimDistance, bool los)
{
    if (twoDimDistance < 10)
        twoDimDistance = 10;

    if (los) {
        // LOS situation
        if (twoDimDistance > 10000) {
            if (tolerateMaxDistViolation_)
                return ATT_MAXDISTVIOLATED;
            else
                throw cRuntimeError("Error: rural macrocell path loss model is valid for d < 10000 m");
        }

        double dbp = 2 * M_PI * hNodeB_ * hUe_ * (carrierFrequencyHz_ / PROPAGATION_VELOCITY);

        double a1 = (0.03 * pow(hBuilding_, 1.72));
        double b1 = 0.044 * pow(hBuilding_, 1.72);
        double a = (a1 < 10) ? a1 : 10;
        double b = (b1 < 14.77) ? b1 : 14.77;

        if (twoDimDistance < dbp)
            return 20 * log10((40 * M_PI * threeDimDistance * carrierFrequencyGHz_) / 3)
                   + a * log10(threeDimDistance) - b + 0.002 * log10(hBuilding_) * threeDimDistance;
        else
            return 20 * log10((40 * M_PI * dbp * carrierFrequencyGHz_) / 3)
                   + a * log10(dbp) - b + 0.002 * log10(hBuilding_) * dbp
                   + 40 * log10(threeDimDistance / dbp);
    }

    // NLOS situation
    if (twoDimDistance > 5000) {
        if (tolerateMaxDistViolation_)
            return ATT_MAXDISTVIOLATED;
        else
            throw cRuntimeError("Error: NLOS rural macrocell path loss model is valid for d<5000 m");
    }

    double pLoss_nlos = 161.04 - 7.1 * log10(wStreet_) + 7.5 * log10(hBuilding_)
        - (24.37 - 3.7 * pow(hBuilding_ / hNodeB_, 2)) * log10(hNodeB_)
        + (43.42 - 3.1 * log10(hNodeB_)) * (log10(threeDimDistance) - 3) + 20 * log10CarrierFrequencyGHz_
        - (3.2 * (pow(log10(11.75 * hUe_), 2)) - 4.97);
    return pLoss_nlos;
}

} //namespace
