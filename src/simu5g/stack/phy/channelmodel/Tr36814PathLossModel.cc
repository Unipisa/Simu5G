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

#include "simu5g/stack/phy/channelmodel/Tr36814PathLossModel.h"

namespace simu5g {

// attenuation value to be returned if the maximum distance of a scenario has been violated
// and tolerating the maximum distance violation is enabled
#define ATT_MAXDISTVIOLATED    1000

using namespace omnetpp;

double Tr36814PathLossModel::computePathLoss(double d3D, double d2D, bool los, const LinkConditions& conditions)
{
    // compute attenuation based on selected scenario and based on LOS or NLOS
    double pathLoss = 0;
    double dbp = 0;
    switch (scenario_) {
        case INDOOR_HOTSPOT:
            pathLoss = computeIndoor(d3D, los);
            break;
        case URBAN_MICROCELL:
            pathLoss = computeUrbanMicro(d3D, los);
            break;
        case URBAN_MACROCELL:
            pathLoss = computeUrbanMacro(d3D, los);
            break;
        case RURAL_MACROCELL:
            pathLoss = computeRuralMacro(d3D, dbp, los);
            break;
        case SUBURBAN_MACROCELL:
            pathLoss = computeSubUrbanMacro(d3D, dbp, los);
            break;
        default:
            throw cRuntimeError("Wrong value %d for path-loss scenario", scenario_);
    }
    return pathLoss;
}

double Tr36814PathLossModel::computeLosProbability(double d3D, double d2D)
{
    double d = d3D;
    double p = 0;
    switch (scenario_) {
        case INDOOR_HOTSPOT:
            if (d < 18)
                p = 1;
            else if (d >= 37)
                p = 0.5;
            else
                p = exp((-1) * ((d - 18) / 27));
            break;
        case URBAN_MICROCELL:
            p = (((18 / d) > 1) ? 1 : 18 / d) * (1 - exp(-1 * d / 36))
                + exp(-1 * d / 36);
            break;
        case URBAN_MACROCELL:
            // same form as UMi, decaying over 63 m rather than 36 m
            p = (((18 / d) > 1) ? 1 : 18 / d) * (1 - exp(-1 * d / 63))
                + exp(-1 * d / 63);
            break;
        case RURAL_MACROCELL:
            if (d <= 10)
                p = 1;
            else
                p = exp(-1 * (d - 10) / 1000);
            break;
        case SUBURBAN_MACROCELL:
            if (d <= 10)
                p = 1;
            else
                p = exp(-1 * (d - 10) / 200);
            break;
        default:
            throw cRuntimeError("Wrong path-loss scenario value %d", scenario_);
    }
    return p;
}

double Tr36814PathLossModel::getShadowingStdDev(double d3D, double d2D, bool losState)
{
    // Breakpoint distance of the RMa/SMa path loss, the only scenarios whose
    // LOS branch uses different sigma values below and above it.
    double dbp = 2 * M_PI * hNodeB_ * hUe_ * (carrierFrequencyHz_ / PROPAGATION_VELOCITY);
    return selectStdDev(d3D < dbp, losState);
}

double Tr36814PathLossModel::selectStdDev(bool belowBreakpoint, bool losState)
{
    switch (scenario_) {
        case URBAN_MICROCELL:
        case INDOOR_HOTSPOT:
            if (losState)
                return 3.;
            else
                return 4.;
        case URBAN_MACROCELL:
            if (losState)
                return 4.;
            else
                return 6.;
        case RURAL_MACROCELL:
        case SUBURBAN_MACROCELL:
            if (losState) {
                if (belowBreakpoint)
                    return 4.;
                else
                    return 6.;
            }
            else
                return 8.;
        default:
            throw cRuntimeError("Wrong path-loss scenario value %d", scenario_);
    }
    return 0.0;
}

double Tr36814PathLossModel::computeAngularAttenuation(double hAngle, double vAngle) {

    // in this implementation, vertical angle is not considered

    double angularAtt;
    double angularAttMin = 25;
    // compute attenuation due to angular position
    // see TR 36.814 V9.0.0 for more details
    angularAtt = 12 * pow(hAngle / 70.0, 2);

    //  EV << "\t angularAtt[" << angularAtt << "]" << endl;
    // max value for angular attenuation is 25 dB
    if (angularAtt > angularAttMin)
        angularAtt = angularAttMin;

    return angularAtt;
}

double Tr36814PathLossModel::computeIndoor(double d, bool los)
{
    double a, b;
    if (los) {
        if (d > 150 || d < 3)
            throw cRuntimeError("Error LOS indoor path loss model is valid for 3<d<150");
        a = 16.9;
        b = 32.8;
    }
    else {
        if (d > 250 || d < 6)
            throw cRuntimeError("Error NLOS indoor path loss model is valid for 6<d<250");
        a = 43.3;
        b = 11.5;
    }
    return a * log10(d) + b + 20 * log10CarrierFrequencyGHz_;
}

double Tr36814PathLossModel::computeUrbanMicro(double d, bool los)
{
    if (d < 10)
        d = 10;

    double dbp = 4 * (hNodeB_ - 1) * (hUe_ - 1)
        * (carrierFrequencyHz_ / PROPAGATION_VELOCITY);
    if (los) {
        // LOS situation
        if (d > 5000) {
            if (tolerateMaxDistViolation_)
                return ATT_MAXDISTVIOLATED;
            else
                throw cRuntimeError("Error: LOS urban microcell path loss model is valid for d < 5000 m");
        }
        if (d < dbp)
            return 22 * log10(d) + 28 + 20 * log10CarrierFrequencyGHz_;
        else
            return 40 * log10(d) + 7.8 - 18 * log10(hNodeB_ - 1)
                   - 18 * log10(hUe_ - 1) + 2 * log10CarrierFrequencyGHz_;
    }
    // NLOS situation
    if (d < 10)
        throw cRuntimeError("Error: NLOS urban microcell path loss model is valid for 10 m < d ");
    if (d > 5000) {
        if (tolerateMaxDistViolation_)
            return ATT_MAXDISTVIOLATED;
        else
            throw cRuntimeError("Error: NLOS urban microcell path loss model is valid for d < 2000 m");
    }
    return 36.7 * log10(d) + 22.7 + 26 * log10CarrierFrequencyGHz_;
}

double Tr36814PathLossModel::computeUrbanMacro(double d, bool los)
{
    if (d < 10)
        d = 10;

    double dbp = 4 * (hNodeB_ - 1) * (hUe_ - 1)
        * (carrierFrequencyHz_ / PROPAGATION_VELOCITY);
    if (los) {
        if (d > 5000) {
            if (tolerateMaxDistViolation_)
                return ATT_MAXDISTVIOLATED;
            else
                throw cRuntimeError("Error: LOS urban macrocell path loss model is valid for d < 5000 m");
        }
        if (d < dbp)
            return 22 * log10(d) + 28 + 20 * log10CarrierFrequencyGHz_;
        else
            return 40 * log10(d) + 7.8 - 18 * log10(hNodeB_ - 1)
                   - 18 * log10(hUe_ - 1) + 2 * log10CarrierFrequencyGHz_;
    }

    if (d < 10)
        throw cRuntimeError("Error: NLOS urban macrocell path loss model is valid for 10 m < d ");
    if (d > 5000) {
        if (tolerateMaxDistViolation_)
            return ATT_MAXDISTVIOLATED;
        else
            throw cRuntimeError("Error: NLOS urban macrocell path loss model is valid for d < 5000 m");
    }

    double att = 161.04 - 7.1 * log10(wStreet_) + 7.5 * log10(hBuilding_)
        - (24.37 - 3.7 * pow(hBuilding_ / hNodeB_, 2)) * log10(hNodeB_)
        + (43.42 - 3.1 * log10(hNodeB_)) * (log10(d) - 3)
        + 20 * log10CarrierFrequencyGHz_
        - (3.2 * (pow(log10(11.75 * hUe_), 2)) - 4.97);
    return att;
}

double Tr36814PathLossModel::computeSubUrbanMacro(double d, double& dbp, bool los)
{
    if (d < 10)
        d = 10;

    dbp = 2 * M_PI * hNodeB_ * hUe_
        * (carrierFrequencyHz_ / PROPAGATION_VELOCITY);
    if (los) {
        if (d > 5000) {
            if (tolerateMaxDistViolation_)
                return ATT_MAXDISTVIOLATED;
            else
                throw cRuntimeError("Error: LOS suburban macrocell path loss model is valid for d < 5000 m");
        }
        double a1 = (0.03 * pow(hBuilding_, 1.72));
        double b1 = 0.044 * pow(hBuilding_, 1.72);
        double a = (a1 < 10) ? a1 : 10;
        double b = (b1 < 14.77) ? b1 : 14.77;
        if (d < dbp) {
            double first = 20 * log10((40 * M_PI * d * carrierFrequencyGHz_) / 3);
            double second = a * log10(d);
            double fourth = 0.002 * log10(hBuilding_) * d;
            return first + second - b + fourth;
        }
        else
            return 20 * log10((40 * M_PI * dbp * carrierFrequencyGHz_) / 3)
                   + a * log10(dbp) - b + 0.002 * log10(hBuilding_) * dbp
                   + 40 * log10(d / dbp);
    }
    if (d > 5000) {
        if (tolerateMaxDistViolation_)
            return ATT_MAXDISTVIOLATED;
        else
            throw cRuntimeError("Error: NLOS suburban macrocell path loss model is valid for 10 < d < 5000 m");
    }
    double att = 161.04 - 7.1 * log10(wStreet_) + 7.5 * log10(hBuilding_)
        - (24.37 - 3.7 * pow(hBuilding_ / hNodeB_, 2)) * log10(hNodeB_)
        + (43.42 - 3.1 * log10(hNodeB_)) * (log10(d) - 3)
        + 20 * log10CarrierFrequencyGHz_
        - (3.2 * (pow(log10(11.75 * hUe_), 2)) - 4.97);
    return att;
}

double Tr36814PathLossModel::computeRuralMacro(double d, double& dbp, bool los)
{
    if (d < 10)
        d = 10;

    dbp = 2 * M_PI * hNodeB_ * hUe_
        * (carrierFrequencyHz_ / PROPAGATION_VELOCITY);
    if (los) {
        // LOS situation
        if (d > 10000) {
            if (tolerateMaxDistViolation_)
                return ATT_MAXDISTVIOLATED;
            else
                throw cRuntimeError("Error: LOS rural macrocell path loss model is valid for d < 10000 m");
        }

        double a1 = (0.03 * pow(hBuilding_, 1.72));
        double b1 = 0.044 * pow(hBuilding_, 1.72);
        double a = (a1 < 10) ? a1 : 10;
        double b = (b1 < 14.77) ? b1 : 14.77;
        if (d < dbp)
            return 20 * log10((40 * M_PI * d * carrierFrequencyGHz_) / 3)
                   + a * log10(d) - b + 0.002 * log10(hBuilding_) * d;
        else
            return 20 * log10((40 * M_PI * dbp * carrierFrequencyGHz_) / 3)
                   + a * log10(dbp) - b + 0.002 * log10(hBuilding_) * dbp
                   + 40 * log10(d / dbp);
    }
    // NLOS situation
    if (d > 5000) {
        if (tolerateMaxDistViolation_)
            return ATT_MAXDISTVIOLATED;
        else
            throw cRuntimeError("Error: NLOS rural macrocell path loss model is valid for d < 5000 m");
    }

    double att = 161.04 - 7.1 * log10(wStreet_) + 7.5 * log10(hBuilding_)
        - (24.37 - 3.7 * pow(hBuilding_ / hNodeB_, 2)) * log10(hNodeB_)
        + (43.42 - 3.1 * log10(hNodeB_)) * (log10(d) - 3)
        + 20 * log10CarrierFrequencyGHz_
        - (3.2 * (pow(log10(11.75 * hUe_), 2)) - 4.97);
    return att;
}

} //namespace
