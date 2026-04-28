//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/phy/channelmodel/NtnChannelModel.h"

#include <algorithm>
#include <array>
#include <boost/math/distributions/normal.hpp>
#include <cmath>
#include <complex>
#include <limits>

#include "simu5g/mobility/georeference/GeographicReferenceSystem.h"

namespace simu5g {

// LOS probabilities from 3GPP TR 38.811, Section 6.6.1, Table 6.6.1-1,
// for the reference elevation angles 10, 20, ..., 90 degrees.
static constexpr std::array<double, 9> kDenseUrbanLosProb = {0.282, 0.331, 0.398, 0.468, 0.537, 0.612, 0.738, 0.820, 0.981};
static constexpr std::array<double, 9> kUrbanLosProb = {0.246, 0.386, 0.493, 0.613, 0.726, 0.805, 0.919, 0.968, 0.992};
static constexpr std::array<double, 9> kSuburbanRuralLosProb = {0.782, 0.869, 0.919, 0.929, 0.935, 0.940, 0.949, 0.952, 0.998};

// Shadow-fading standard deviations sigma_SF [dB] from 3GPP TR 38.811, Section 6.6.2,
// Table 6.6.2-1, for the reference elevation angles 10, 20, ..., 90 degrees.
static constexpr std::array<double, 9> kDenseUrbanSbandLosSf = {3.5, 3.4, 2.9, 3.0, 3.1, 2.7, 2.5, 2.3, 1.2};
static constexpr std::array<double, 9> kDenseUrbanSbandNlosSf = {15.5, 13.9, 12.4, 11.7, 10.6, 10.5, 10.1, 9.2, 9.2};
static constexpr std::array<double, 9> kDenseUrbanKabandLosSf = {2.9, 2.4, 2.7, 2.4, 2.4, 2.7, 2.6, 2.8, 0.6};
static constexpr std::array<double, 9> kDenseUrbanKabandNlosSf = {17.1, 17.1, 15.6, 14.6, 14.2, 12.6, 12.1, 12.3, 12.3};
static constexpr std::array<double, 9> kUrbanLosSf = {4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0};
static constexpr std::array<double, 9> kUrbanNlosSf = {6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0};
static constexpr std::array<double, 9> kSuburbanRuralSbandLosSf = {1.79, 1.14, 1.14, 0.92, 1.42, 1.56, 0.85, 0.72, 0.72};
static constexpr std::array<double, 9> kSuburbanRuralSbandNlosSf = {8.93, 9.08, 8.78, 10.25, 10.56, 10.74, 10.17, 11.52, 11.52};
static constexpr std::array<double, 9> kSuburbanRuralKabandLosSf = {1.9, 1.6, 1.9, 2.3, 2.7, 3.1, 3.0, 3.6, 0.4};
static constexpr std::array<double, 9> kSuburbanRuralKabandNlosSf = {10.7, 10.0, 11.2, 11.6, 11.8, 10.8, 10.8, 10.8, 10.8};

// TR 38.811 section 6.6.2 uses scenario-specific shadow-fading sigmas, but the clutter-loss
// values in tables 6.6.2-1/2/3 are shared across dense urban, urban, suburban, and rural cases.
static constexpr std::array<double, 9> kSbandClutterLoss = {34.3, 30.9, 29.0, 27.7, 26.8, 26.2, 25.8, 25.5, 25.5};
static constexpr std::array<double, 9> kKabandClutterLoss = {44.3, 39.9, 37.5, 35.8, 34.6, 33.8, 33.3, 33.0, 32.9};

// Zenith attenuation A_zenith(f) [dB] sampled at integer frequencies 0..100 GHz from
// ITU-R P.676 Figure 6, as referenced by the baseline method in 3GPP TR 38.811 section 6.6.4.
static constexpr std::array<double, 101> kAtmosphericAbsorption = {
    0.0, 0.0300, 0.0350, 0.0380, 0.0390, 0.0410, 0.0420, 0.0450, 0.0480, 0.0500,
    0.0530, 0.0587, 0.0674, 0.0789, 0.0935, 0.1113, 0.1322, 0.1565, 0.1841, 0.2153,
    0.2500, 0.3362, 0.4581, 0.5200, 0.5200, 0.5000, 0.4500, 0.3850, 0.3200, 0.2700,
    0.2500, 0.2517, 0.2568, 0.2651, 0.2765, 0.2907, 0.3077, 0.3273, 0.3493, 0.3736,
    0.4000, 0.4375, 0.4966, 0.5795, 0.6881, 0.8247, 0.9912, 1.1900, 1.4229, 1.6922,
    2.0000, 4.2654, 10.1504, 19.2717, 31.2457, 45.6890, 62.2182, 80.4496, 100.0000, 140.0205,
    170.0000, 100.0000, 78.1682, 59.3955, 43.5434, 30.4733, 20.0465, 12.1244, 6.5683, 3.2397,
    2.0000, 1.7708, 1.5660, 1.3858, 1.2298, 1.0981, 0.9905, 0.9070, 0.8475, 0.8119,
    0.8000, 0.8000, 0.8000, 0.8000, 0.8000, 0.8000, 0.8000, 0.8000, 0.8000, 0.8000,
    0.8000, 0.8029, 0.8112, 0.8243, 0.8416, 0.8625, 0.8864, 0.9127, 0.9408, 0.9701,
    1.0000};

// Tropospheric scintillation loss [dB] with 99% probability at 20 GHz in Toulouse from
// 3GPP TR 38.811, Table 6.6.6.2.1-1, for the reference elevation angles 10, 20, ..., 90 degrees.
static constexpr std::array<double, 9> kTroposphericScintillationLoss = {1.08, 0.48, 0.30, 0.22, 0.17, 0.13, 0.12, 0.12, 0.12};

struct BuildingPenetrationCoefficients
{
    double r;
    double s;
    double t;
    double u;
    double v;
    double w;
    double x;
    double y;
    double z;
};

// Building entry loss coefficients from 3GPP TR 38.811, Table 6.6.3-1.
static constexpr BuildingPenetrationCoefficients kTraditionalBuildingCoefficients = {12.64, 3.72, 0.96, 9.6, 2.0, 9.1, -3.0, 4.5, -2.0};
static constexpr BuildingPenetrationCoefficients kThermallyEfficientBuildingCoefficients = {28.19, -3.00, 8.48, 13.5, 3.8, 27.8, -2.9, 9.4, -2.1};

// ITU-R P.681 Annex 2 two-state narrowband flat-fading parameters.
// Frequency-selective system-level parameters from 3GPP TR 38.811 section 6.7.2.
// The 38.901 fast-fading generation process is reused, with these NTN-specific DS/K/rTau/N tables.
static constexpr FrequencySelectiveScenarioParameters kDenseUrbanLosSband = {
    {-7.12, -7.28, -7.45, -7.73, -7.91, -8.14, -8.23, -8.28, -8.36},
    {0.80, 0.67, 0.68, 0.66, 0.62, 0.51, 0.45, 0.31, 0.08},
    {4.4, 9.0, 9.3, 7.9, 7.4, 7.0, 6.9, 6.5, 6.8},
    {3.3, 6.6, 6.1, 4.0, 3.0, 2.6, 2.2, 2.1, 1.9},
    {2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5},
    {3, 3, 3, 3, 3, 3, 3, 3, 3},
    true};
static constexpr FrequencySelectiveScenarioParameters kDenseUrbanLosKaband = {
    {-7.43, -7.62, -7.76, -8.02, -8.13, -8.30, -8.34, -8.39, -8.45},
    {0.90, 0.78, 0.80, 0.72, 0.61, 0.47, 0.39, 0.26, 0.01},
    {6.1, 13.7, 12.9, 10.3, 9.2, 8.4, 8.0, 7.4, 7.6},
    {2.6, 6.8, 6.0, 3.3, 2.2, 1.9, 1.5, 1.6, 1.3},
    {2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5},
    {3, 3, 3, 3, 3, 3, 3, 3, 3},
    true};
static constexpr FrequencySelectiveScenarioParameters kDenseUrbanNlosSband = {
    {-6.84, -6.81, -6.94, -7.14, -7.34, -7.53, -7.67, -7.82, -7.84},
    {0.82, 0.61, 0.49, 0.49, 0.51, 0.47, 0.44, 0.42, 0.55},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {2.3, 2.3, 2.3, 2.3, 2.3, 2.3, 2.3, 2.3, 2.3},
    {4, 4, 4, 4, 4, 4, 4, 4, 4},
    false};
static constexpr FrequencySelectiveScenarioParameters kDenseUrbanNlosKaband = {
    {-6.86, -6.84, -7.00, -7.21, -7.42, -7.86, -7.76, -8.07, -7.95},
    {0.81, 0.61, 0.56, 0.56, 0.57, 0.55, 0.47, 0.42, 0.59},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {2.3, 2.3, 2.3, 2.3, 2.3, 2.3, 2.3, 2.3, 2.3},
    {4, 4, 4, 4, 4, 4, 4, 4, 4},
    false};
static constexpr FrequencySelectiveScenarioParameters kUrbanLosSband = {
    {-7.97, -8.12, -8.21, -8.31, -8.37, -8.39, -8.38, -8.35, -8.34},
    {1.0, 0.83, 0.68, 0.48, 0.38, 0.24, 0.18, 0.13, 0.09},
    {31.83, 18.78, 10.49, 7.46, 6.52, 5.47, 4.54, 4.03, 3.68},
    {13.84, 13.78, 10.42, 8.01, 8.27, 7.26, 5.53, 4.49, 3.14},
    {2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5},
    {4, 3, 3, 3, 3, 3, 3, 3, 3},
    true};
static constexpr FrequencySelectiveScenarioParameters kUrbanLosKaband = {
    {-8.52, -8.59, -8.51, -8.49, -8.48, -8.44, -8.4, -8.37, -8.35},
    {0.92, 0.79, 0.65, 0.48, 0.46, 0.34, 0.27, 0.19, 0.14},
    {40.18, 23.62, 12.48, 8.56, 7.42, 5.97, 4.88, 4.22, 3.81},
    {16.99, 18.96, 14.23, 11.06, 11.21, 9.47, 7.24, 5.79, 4.25},
    {2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5},
    {4, 3, 3, 3, 3, 3, 3, 3, 3},
    true};
static constexpr FrequencySelectiveScenarioParameters kUrbanNlosSband = {
    {-7.21, -7.63, -7.75, -7.97, -7.99, -8.01, -8.09, -7.97, -8.17},
    {1.19, 0.98, 0.84, 0.73, 0.73, 0.72, 0.71, 0.78, 0.67},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {2.3, 2.3, 2.3, 2.3, 2.3, 2.3, 2.3, 2.3, 2.3},
    {3, 3, 3, 3, 3, 3, 2, 2, 2},
    false};
static constexpr FrequencySelectiveScenarioParameters kUrbanNlosKaband = {
    {-7.24, -7.7, -7.82, -8.04, -8.08, -8.1, -8.16, -8.03, -8.33},
    {1.26, 0.99, 0.86, 0.75, 0.77, 0.76, 0.73, 0.79, 0.7},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {2.3, 2.3, 2.3, 2.3, 2.3, 2.3, 2.3, 2.3, 2.3},
    {3, 3, 3, 3, 3, 3, 2, 2, 2},
    false};
static constexpr FrequencySelectiveScenarioParameters kSuburbanLosSband = {
    {-8.16, -8.56, -8.72, -8.71, -8.72, -8.66, -8.38, -8.34, -8.34},
    {0.99, 0.96, 0.79, 0.81, 1.12, 1.23, 0.55, 0.63, 0.63},
    {11.40, 19.45, 20.80, 21.20, 21.60, 19.75, 12.00, 12.85, 12.85},
    {6.26, 10.32, 16.34, 15.63, 14.22, 14.19, 5.70, 9.91, 9.91},
    {2.20, 3.36, 3.50, 2.81, 2.39, 2.73, 2.07, 2.04, 2.04},
    {3, 3, 3, 3, 3, 3, 2, 2, 2},
    true};
static constexpr FrequencySelectiveScenarioParameters kSuburbanLosKaband = {
    {-8.07, -8.61, -8.72, -8.63, -8.54, -8.48, -8.42, -8.39, -8.37},
    {0.46, 0.45, 0.28, 0.17, 0.14, 0.15, 0.09, 0.05, 0.02},
    {8.9, 14.0, 11.3, 9.0, 7.5, 6.6, 5.9, 5.5, 5.4},
    {4.4, 4.6, 3.7, 3.5, 3.0, 2.6, 1.7, 0.7, 0.3},
    {2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5},
    {3, 3, 3, 3, 3, 3, 2, 2, 2},
    true};
static constexpr FrequencySelectiveScenarioParameters kSuburbanNlosSband = {
    {-7.91, -8.39, -8.69, -8.59, -8.64, -8.74, -8.98, -9.28, -9.28},
    {1.42, 1.46, 1.46, 1.21, 1.18, 1.13, 1.37, 1.50, 1.50},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {2.28, 2.33, 2.43, 2.26, 2.71, 2.10, 2.19, 2.06, 2.06},
    {4, 4, 4, 4, 4, 3, 3, 3, 3},
    false};
static constexpr FrequencySelectiveScenarioParameters kSuburbanNlosKaband = {
    {-7.43, -7.63, -7.86, -7.96, -7.98, -8.45, -8.21, -8.69, -8.69},
    {0.50, 0.61, 0.56, 0.58, 0.59, 0.47, 0.36, 0.29, 0.29},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {2.3, 2.3, 2.3, 2.3, 2.3, 2.3, 2.3, 2.3, 2.3},
    {4, 4, 4, 4, 4, 3, 3, 3, 3},
    false};
static constexpr FrequencySelectiveScenarioParameters kRuralLosSband = {
    {-9.55, -8.68, -8.46, -8.36, -8.29, -8.26, -8.22, -8.2, -8.19},
    {0.66, 0.44, 0.28, 0.19, 0.14, 0.1, 0.1, 0.05, 0.06},
    {24.72, 12.31, 8.05, 6.21, 5.04, 4.42, 3.92, 3.65, 3.59},
    {5.07, 5.75, 5.46, 5.23, 3.95, 3.75, 2.56, 1.77, 1.77},
    {3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8},
    {2, 2, 2, 2, 2, 2, 2, 2, 2},
    true};
static constexpr FrequencySelectiveScenarioParameters kRuralLosKaband = {
    {-9.68, -8.86, -8.59, -8.46, -8.36, -8.3, -8.26, -8.22, -8.21},
    {0.46, 0.29, 0.18, 0.19, 0.14, 0.15, 0.13, 0.03, 0.07},
    {25.43, 12.72, 8.40, 6.52, 5.24, 4.57, 4.02, 3.70, 3.62},
    {7.04, 7.47, 7.18, 6.88, 5.28, 4.92, 3.40, 2.22, 2.28},
    {3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8},
    {2, 2, 2, 2, 2, 2, 2, 2, 2},
    true};
static constexpr FrequencySelectiveScenarioParameters kRuralNlosSband = {
    {-9.01, -8.37, -8.05, -7.92, -7.92, -7.96, -7.91, -7.79, -7.74},
    {1.59, 0.95, 0.92, 0.92, 0.87, 0.87, 0.82, 0.86, 0.81},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {1.7, 1.7, 1.7, 1.7, 1.7, 1.7, 1.7, 1.7, 1.7},
    {3, 3, 2, 2, 2, 2, 2, 2, 2},
    false};
static constexpr FrequencySelectiveScenarioParameters kRuralNlosKaband = {
    {-9.13, -8.39, -8.1, -7.96, -7.99, -8.05, -8.01, -8.05, -7.91},
    {1.91, 0.94, 0.92, 0.94, 0.89, 0.87, 0.82, 1.65, 0.76},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {1.7, 1.7, 1.7, 1.7, 1.7, 1.7, 1.7, 1.7, 1.7},
    {3, 3, 2, 2, 2, 2, 2, 2, 2},
    false};

Define_Module(NtnChannelModel);

void NtnChannelModel::initialize(int stage)
{
    LteRealisticChannelModel::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        if (inside_building_)
            useBuildingPenetrationHighLossModel_ = par("useBuildingPenetrationHighLossModel").boolValue();
    }
}

double NtnChannelModel::getAttenuation(MacNodeId nodeId, Direction dir, inet::Coord coord, bool cqiDl)
{
    (void)dir;
    (void)coord;

    double speed = computeSpeed(nodeId, lastTerrestrialEndpointCoord_);
    double correlationDist = computeCorrelationDistance(nodeId, lastTerrestrialEndpointCoord_);
    inet::Coord groundEcef = ecefFromWgs84(lastTerrestrialEndpointWgs84_);
    inet::Coord ntnEcef = lastSatelliteEndpointEcefCoord_;
    double distance = groundEcef.distance(ntnEcef);

    // compute LOS probability
    if (correlationDist > correlationDistance_ || losMap_.find(nodeId) == losMap_.end())
        computeLosProbability(distance, nodeId);

    bool los = losMap_[nodeId];
    double dbp = 0;

    // compute base path loss
    double attenuation = computePathLoss(distance, dbp, los);
    // add O2I penetration loss
    attenuation += computeBuildingPenetrationLoss(nodeId);
    // add atmospheric loss
    attenuation += computeAtmosphericLoss();
    // add scintillation loss
    attenuation += computeScintillationLoss();
    // add shadowing
    if (num(nodeId) < BGUE_MIN_ID && shadowing_)
        attenuation += computeShadowing(distance, nodeId, speed, cqiDl);

    updatePositionHistory(nodeId, lastTerrestrialEndpointCoord_);
    updateCorrelationDistance(nodeId, lastTerrestrialEndpointCoord_);

    EV << "NtnChannelModel::getAttenuation - computed attenuation at distance " << distance << " is " << attenuation << endl;

    return attenuation;
}

double NtnChannelModel::computePathLoss(double distance, double dbp, bool los)
{
    (void)dbp;

    if (distance <= 0.0)
        throw cRuntimeError("NtnChannelModel::computePathLoss - invalid distance %g", distance);

    // 3GPP TR 38.811, Section 6.6.2:
    // PLb = FSPL(d, fc) + SF + CL(alpha, fc).
    // Shadow fading is already handled separately by computeShadowing(), so here we apply
    // free-space path loss from (6.6-2) and clutter loss from (6.6-4). In LOS, clutter
    // loss is negligible and set to 0 dB as stated by the specification.
    // TODO revisit whether the same clutter-loss model should be applied to feeder links,
    // where the terrestrial endpoint is a gateway rather than a service-link terminal.
    double pathLoss = 32.45 + 20 * log10CarrierFrequencyGHz_ + 20 * std::log10(distance);
    if (los)
        return pathLoss;

    // add clutter loss if non-LOS
    inet::Coord terrestrialEndpointEcef = ecefFromWgs84(lastTerrestrialEndpointWgs84_);
    double elevationAngle = computeElevationFromEcefEndpoints(lastTerrestrialEndpointWgs84_, terrestrialEndpointEcef, lastSatelliteEndpointEcefCoord_);
    int roundedAngle = static_cast<int>(std::round(elevationAngle / 10.0)) * 10;
    roundedAngle = std::max(10, std::min(90, roundedAngle));
    int angleIndex = roundedAngle / 10 - 1;

    double clutterLoss = carrierFrequency_.get() < 13.0 ? kSbandClutterLoss[angleIndex] : kKabandClutterLoss[angleIndex];
    return pathLoss + clutterLoss;
}

double NtnChannelModel::computeBuildingPenetrationLoss(MacNodeId nodeId)
{
    if (!inside_building_)
        return 0.0;

    auto probabilityIt = buildingPenetrationProbabilityMap_.find(nodeId);
    if (probabilityIt == buildingPenetrationProbabilityMap_.end()) {
        constexpr double epsilon = 1e-6;
        double probability = uniform(epsilon, 1.0 - epsilon);
        probabilityIt = buildingPenetrationProbabilityMap_.emplace(nodeId, probability).first;
    }

    inet::Coord terrestrialEndpointEcef = ecefFromWgs84(lastTerrestrialEndpointWgs84_);
    double elevationAngle = computeElevationFromEcefEndpoints(lastTerrestrialEndpointWgs84_, terrestrialEndpointEcef, lastSatelliteEndpointEcefCoord_);
    double carrierFrequencyGHz = carrierFrequency_.get();
    double logFrequency = std::log10(carrierFrequencyGHz);

    const BuildingPenetrationCoefficients& coeffs = useBuildingPenetrationHighLossModel_
        ? kThermallyEfficientBuildingCoefficients
        : kTraditionalBuildingCoefficients;

    double horizontalMedianLoss = coeffs.r + coeffs.s * logFrequency + coeffs.t * std::pow(logFrequency, 2);
    double elevationCorrection = 0.212 * elevationAngle;
    double mu1 = horizontalMedianLoss + elevationCorrection;
    double mu2 = coeffs.w + coeffs.x * logFrequency;
    double sigma1 = coeffs.u + coeffs.v * logFrequency;
    double sigma2 = coeffs.y + coeffs.z * logFrequency;

    boost::math::normal_distribution<double> standardNormal;
    double inverseNormalCdf = boost::math::quantile(standardNormal, probabilityIt->second);
    double a = inverseNormalCdf * sigma1 + mu1;
    double b = inverseNormalCdf * sigma2 + mu2;
    constexpr double c = -3.0;

    return 10.0 * std::log10(std::pow(10.0, 0.1 * a) + std::pow(10.0, 0.1 * b) + std::pow(10.0, 0.1 * c));
}

double NtnChannelModel::computeAtmosphericLoss()
{
    inet::Coord terrestrialEndpointEcef = ecefFromWgs84(lastTerrestrialEndpointWgs84_);
    double elevationAngle = computeElevationFromEcefEndpoints(lastTerrestrialEndpointWgs84_, terrestrialEndpointEcef, lastSatelliteEndpointEcefCoord_);
    double carrierFrequencyGHz = carrierFrequency_.get();

    // 3GPP TR 38.811 section 6.6.4 baseline:
    // - ignore atmospheric absorption below 10 GHz,
    //   except for elevation angles below 10 degrees when f > 1 GHz
    // - use A_zenith(f) / sin(alpha), with A_zenith taken from ITU-R P.676 Figure 6
    if (!((elevationAngle < 10.0 && carrierFrequencyGHz > 1.0) || carrierFrequencyGHz >= 10.0))
        return 0.0;

    int roundedFrequencyGHz = static_cast<int>(std::round(carrierFrequencyGHz));
    roundedFrequencyGHz = std::max(0, std::min(static_cast<int>(kAtmosphericAbsorption.size()) - 1, roundedFrequencyGHz));

    double sinElevation = std::sin(elevationAngle * M_PI / 180.0);
    sinElevation = std::max(sinElevation, std::numeric_limits<double>::epsilon());
    return kAtmosphericAbsorption[roundedFrequencyGHz] / sinElevation;
}

double NtnChannelModel::computeScintillationLoss()
{
    inet::Coord terrestrialEndpointEcef = ecefFromWgs84(lastTerrestrialEndpointWgs84_);
    double elevationAngle = computeElevationFromEcefEndpoints(lastTerrestrialEndpointWgs84_, terrestrialEndpointEcef, lastSatelliteEndpointEcefCoord_);
    double carrierFrequencyGHz = carrierFrequency_.get();

    // 3GPP TR 38.811 section 6.6.6 baseline:
    // - below 6 GHz, use the ionospheric scintillation approximation of 6.6.6.1.4
    // - otherwise, use the tropospheric scintillation table of 6.6.6.2.1 at the nearest
    //   reference elevation angle
    if (carrierFrequencyGHz < 6.0)
        return 6.22 / std::pow(carrierFrequencyGHz, 1.5);

    int roundedAngle = static_cast<int>(std::round(elevationAngle / 10.0)) * 10;
    roundedAngle = std::max(10, std::min(90, roundedAngle));
    int angleIndex = roundedAngle / 10 - 1;
    return kTroposphericScintillationLoss[angleIndex];
}

const FrequencySelectiveScenarioParameters& NtnChannelModel::getFrequencySelectiveScenarioParameters(bool los, bool isSband) const
{
    switch (scenario_) {
        case INDOOR_HOTSPOT:
        case URBAN_MICROCELL:
            if (los)
                return isSband ? kDenseUrbanLosSband : kDenseUrbanLosKaband;
            return isSband ? kDenseUrbanNlosSband : kDenseUrbanNlosKaband;
        case URBAN_MACROCELL:
            if (los)
                return isSband ? kUrbanLosSband : kUrbanLosKaband;
            return isSband ? kUrbanNlosSband : kUrbanNlosKaband;
        case SUBURBAN_MACROCELL:
            if (los)
                return isSband ? kSuburbanLosSband : kSuburbanLosKaband;
            return isSband ? kSuburbanNlosSband : kSuburbanNlosKaband;
        case RURAL_MACROCELL:
            if (los)
                return isSband ? kRuralLosSband : kRuralLosKaband;
            return isSband ? kRuralNlosSband : kRuralNlosKaband;
        default:
            throw cRuntimeError("NtnChannelModel::getFrequencySelectiveScenarioParameters - unsupported scenario value %d", scenario_);
    }
}

double NtnChannelModel::getFadingRefreshDistance(bool los) const
{
    switch (scenario_) {
        case INDOOR_HOTSPOT:
        case URBAN_MICROCELL:
        case URBAN_MACROCELL:
            return los ? 30.0 : 40.0;
        case SUBURBAN_MACROCELL:
            return los ? 30.0 : 40.0;
        case RURAL_MACROCELL:
            return los ? 50.0 : 36.0;
        default:
            throw cRuntimeError("NtnChannelModel::getFadingRefreshDistance - unsupported scenario value %d", scenario_);
    }
}

std::vector<double> NtnChannelModel::getBandCenterFrequenciesHz() const
{
    double subcarrierSpacingHz = 15000.0 * static_cast<double>(1u << getNumerologyIndex());
    double bandWidthHz = 12.0 * subcarrierSpacingHz;
    double centerOffset = 0.5 * static_cast<double>(numBands_ - 1);

    std::vector<double> frequencies(numBands_, carrierFrequencyHz_);
    for (unsigned int i = 0; i < numBands_; ++i)
        frequencies[i] += (static_cast<double>(i) - centerOffset) * bandWidthHz;
    return frequencies;
}

void NtnChannelModel::initializeFrequencySelectiveFadingState(FrequencySelectiveFadingState& state, const FrequencySelectiveScenarioParameters& params, int angleIndex, double maxDopplerHz)
{
    constexpr double epsilon = 1e-12;

    double delaySpreadSeconds = std::pow(10.0, normal(params.muLgDs[angleIndex], params.sigmaLgDs[angleIndex]));
    delaySpreadSeconds = std::max(delaySpreadSeconds, epsilon);

    int numClusters = std::max(params.numClusters[angleIndex], 1);
    std::vector<double> delays(numClusters, 0.0);
    for (int i = 0; i < numClusters; ++i) {
        double sample = std::max(uniform(0.0, 1.0), epsilon);
        delays[i] = -params.rTau[angleIndex] * delaySpreadSeconds * std::log(sample);
    }
    std::sort(delays.begin(), delays.end());
    double minDelay = delays.front();
    for (double& delay : delays)
        delay -= minDelay;

    std::vector<double> powers(numClusters, 0.0);
    double powerSum = 0.0;
    for (int i = 0; i < numClusters; ++i) {
        double shadowingDb = normal(0.0, 3.0);
        double decay = std::exp(-delays[i] * (params.rTau[angleIndex] - 1.0) / (params.rTau[angleIndex] * delaySpreadSeconds));
        powers[i] = decay * std::pow(10.0, -shadowingDb / 10.0);
        powerSum += powers[i];
    }
    powerSum = std::max(powerSum, epsilon);
    for (double& power : powers)
        power /= powerSum;

    state.losLinearPower = 0.0;
    if (params.hasLosComponent) {
        double kDb = normal(params.muK[angleIndex], params.sigmaK[angleIndex]);
        double kLinear = std::pow(10.0, kDb / 10.0);
        state.losLinearPower = kLinear / (kLinear + 1.0);
        for (double& power : powers)
            power /= (kLinear + 1.0);
    }

    state.delaysSeconds = std::move(delays);
    state.linearPowers = std::move(powers);
    state.dopplerHz.resize(numClusters);
    state.initialPhaseRad.resize(numClusters);
    for (int i = 0; i < numClusters; ++i) {
        double angle = uniform(0.0, 2.0 * M_PI);
        state.dopplerHz[i] = maxDopplerHz * std::cos(angle);
        state.initialPhaseRad[i] = uniform(0.0, 2.0 * M_PI);
    }

    double losAngle = uniform(0.0, 2.0 * M_PI);
    state.losDopplerHz = maxDopplerHz * std::cos(losAngle);
    state.losInitialPhaseRad = uniform(0.0, 2.0 * M_PI);
    state.referenceTime = NOW;
    state.lastRefreshCoord = lastTerrestrialEndpointCoord_;
    state.initialized = true;
}

std::vector<double> NtnChannelModel::computeFrequencySelectiveFading(MacNodeId nodeId, double speed)
{
    if (!fading_)
        return std::vector<double>(numBands_, 0.0);

    inet::Coord terrestrialEndpointEcef = ecefFromWgs84(lastTerrestrialEndpointWgs84_);
    double elevationAngle = computeElevationFromEcefEndpoints(lastTerrestrialEndpointWgs84_, terrestrialEndpointEcef, lastSatelliteEndpointEcefCoord_);
    int roundedAngle = static_cast<int>(std::round(elevationAngle / 10.0)) * 10;
    roundedAngle = std::max(10, std::min(90, roundedAngle));
    int angleIndex = roundedAngle / 10 - 1;
    bool los = losMap_[nodeId];
    bool isSband = carrierFrequency_.get() < 13.0;

    const FrequencySelectiveScenarioParameters& params = getFrequencySelectiveScenarioParameters(los, isSband);
    FrequencySelectiveFadingState& state = fadingStateMap_[nodeId];
    double maxDopplerHz = speed * carrierFrequencyHz_ / SPEED_OF_LIGHT;
    double refreshDistance = getFadingRefreshDistance(los);

    bool shouldRefresh = !state.initialized ||
        state.los != los ||
        state.isSband != isSband ||
        state.angleIndex != angleIndex ||
        state.scenario != scenario_ ||
        state.lastRefreshCoord.distance(lastTerrestrialEndpointCoord_) > refreshDistance;

    if (shouldRefresh) {
        state.los = los;
        state.isSband = isSband;
        state.angleIndex = angleIndex;
        state.scenario = scenario_;
        state.refreshDistance = refreshDistance;
        initializeFrequencySelectiveFadingState(state, params, angleIndex, maxDopplerHz);
    }

    double elapsedTime = SIMTIME_DBL(NOW - state.referenceTime);
    std::vector<double> bandFrequenciesHz = getBandCenterFrequenciesHz();
    std::vector<double> fadingAttenuation(numBands_, 0.0);
    for (unsigned int band = 0; band < numBands_; ++band) {
        std::complex<double> response(0.0, 0.0);
        for (size_t cluster = 0; cluster < state.linearPowers.size(); ++cluster) {
            double phase = state.initialPhaseRad[cluster]
                + 2.0 * M_PI * state.dopplerHz[cluster] * elapsedTime
                - 2.0 * M_PI * bandFrequenciesHz[band] * state.delaysSeconds[cluster];
            response += std::polar(std::sqrt(state.linearPowers[cluster]), phase);
        }
        if (state.losLinearPower > 0.0) {
            double losPhase = state.losInitialPhaseRad + 2.0 * M_PI * state.losDopplerHz * elapsedTime;
            response += std::polar(std::sqrt(state.losLinearPower), losPhase);
        }
        double linearGain = std::max(std::norm(response), std::numeric_limits<double>::epsilon());
        fadingAttenuation[band] = linearToDb(linearGain);
    }
    return fadingAttenuation;
}

double NtnChannelModel::computeShadowing(double distance, MacNodeId nodeId, double speed, bool cqiDl)
{
    (void)distance;

    ShadowFadingMap *actualShadowingMap;
    if (cqiDl)
        actualShadowingMap = obtainShadowingMap(nodeId);
    else
        actualShadowingMap = &lastComputedSF_;

    if (actualShadowingMap == nullptr)
        throw cRuntimeError("NtnChannelModel::computeShadowing - actualShadowingMap not found (nullptr)");

    inet::Coord terrestrialEndpointEcef = ecefFromWgs84(lastTerrestrialEndpointWgs84_);
    double elevationAngle = computeElevationFromEcefEndpoints(lastTerrestrialEndpointWgs84_, terrestrialEndpointEcef, lastSatelliteEndpointEcefCoord_);
    int roundedAngle = static_cast<int>(std::round(elevationAngle / 10.0)) * 10;
    roundedAngle = std::max(10, std::min(90, roundedAngle));
    int angleIndex = roundedAngle / 10 - 1;

    const bool isSband = carrierFrequency_.get() < 13.0;
    const bool los = losMap_[nodeId];

    double stdDev = 0.0;
    switch (scenario_) {
        case INDOOR_HOTSPOT:
        case URBAN_MICROCELL:
            if (isSband)
                stdDev = los ? kDenseUrbanSbandLosSf[angleIndex] : kDenseUrbanSbandNlosSf[angleIndex];
            else
                stdDev = los ? kDenseUrbanKabandLosSf[angleIndex] : kDenseUrbanKabandNlosSf[angleIndex];
            break;
        case URBAN_MACROCELL:
            stdDev = los ? kUrbanLosSf[angleIndex] : kUrbanNlosSf[angleIndex];
            break;
        case SUBURBAN_MACROCELL:
        case RURAL_MACROCELL:
            if (isSband)
                stdDev = los ? kSuburbanRuralSbandLosSf[angleIndex] : kSuburbanRuralSbandNlosSf[angleIndex];
            else
                stdDev = los ? kSuburbanRuralKabandLosSf[angleIndex] : kSuburbanRuralKabandNlosSf[angleIndex];
            break;
        default:
            throw cRuntimeError("NtnChannelModel::computeShadowing - unsupported scenario value %d", scenario_);
    }

    double att;
    if (actualShadowingMap->find(nodeId) == actualShadowingMap->end()) {
        att = normal(0.0, stdDev);
        (*actualShadowingMap)[nodeId] = std::make_pair(NOW, att);
    }
    else if ((NOW - actualShadowingMap->at(nodeId).first).dbl() * speed > correlationDistance_) {
        double time = (NOW - actualShadowingMap->at(nodeId).first).dbl();
        double space = time * speed;
        double a = exp(-0.5 * (space / correlationDistance_));
        double old = actualShadowingMap->at(nodeId).second;
        att = a * old + sqrt(1 - pow(a, 2)) * normal(0.0, stdDev);
        (*actualShadowingMap)[nodeId] = std::make_pair(NOW, att);
    }
    else {
        att = actualShadowingMap->at(nodeId).second;
    }

    return att;
}

void NtnChannelModel::computeLosProbability(double d, MacNodeId nodeId)
{
    (void)d;

    if (!dynamicLos_) {
        losMap_[nodeId] = fixedLos_;
        return;
    }

    inet::Coord groundEcef = ecefFromWgs84(lastTerrestrialEndpointWgs84_);
    inet::Coord ntnEcef = lastSatelliteEndpointEcefCoord_;
    double elevationAngle = computeElevationFromEcefEndpoints(lastTerrestrialEndpointWgs84_, groundEcef, ntnEcef);
    int roundedAngle = static_cast<int>(std::round(elevationAngle / 10.0)) * 10;
    roundedAngle = std::max(10, std::min(90, roundedAngle));
    int angleIndex = roundedAngle / 10 - 1;

    double p = 0.0;
    switch (scenario_) {
        case INDOOR_HOTSPOT:
        case URBAN_MICROCELL:
            p = kDenseUrbanLosProb[angleIndex];
            break;
        case URBAN_MACROCELL:
            p = kUrbanLosProb[angleIndex];
            break;
        case SUBURBAN_MACROCELL:
        case RURAL_MACROCELL:
            p = kSuburbanRuralLosProb[angleIndex];
            break;
        default:
            throw cRuntimeError("NtnChannelModel::computeLosProbability - unsupported scenario value %d", scenario_);
    }

    losMap_[nodeId] = uniform(0.0, 1.0) <= p;
}

std::vector<double> NtnChannelModel::getSINR(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    (void)frame;

    GeographicReferenceSystem *referenceSystem = GeographicReferenceSystemAccess().get();
    ASSERT(referenceSystem != nullptr);

    MacNodeId transmitterId = lteInfo->getRadioTransmitterId();
    MacNodeId receiverId = lteInfo->getRadioReceiverId();
    Coord transmitterCoord = lteInfo->getRadioTransmitterCoord();
    Coord receiverCoord = phy_->getRadioPosition();
    RanNodeType transmitterType = getNodeTypeById(transmitterId);
    RanNodeType receiverType = getNodeTypeById(receiverId);
    Coord transmitterEcef = lteInfo->getRadioTransmitterEcefCoord();
    inet::GeoCoord receiverWgs84 = referenceSystem->wgs84FromOmnet(receiverCoord);

    if (transmitterType != SATELLITE_NODE && receiverType != SATELLITE_NODE)
        throw cRuntimeError("NtnChannelModel::getSINR - one hop endpoint must be the satellite");

    MacNodeId terrestrialEndpointId;
    Coord terrestrialEndpointCoord;
    Coord satelliteEndpointCoord;
    inet::GeoCoord terrestrialEndpointWgs84 = inet::GeoCoord::NIL;
    Coord satelliteEndpointEcefCoord;
    if (transmitterType == SATELLITE_NODE) {
        terrestrialEndpointId = receiverId;
        terrestrialEndpointCoord = receiverCoord;
        satelliteEndpointCoord = transmitterCoord;
        terrestrialEndpointWgs84 = receiverWgs84;
        satelliteEndpointEcefCoord = transmitterEcef;
    }
    else {
        terrestrialEndpointId = transmitterId;
        terrestrialEndpointCoord = transmitterCoord;
        satelliteEndpointCoord = receiverCoord;
        terrestrialEndpointWgs84 = referenceSystem->wgs84FromOmnet(transmitterCoord);
        satelliteEndpointEcefCoord = ecefFromWgs84(receiverWgs84);
    }

    bool cqiDl = (lteInfo->getDirection() == DL && lteInfo->getFrameType() == FEEDBACKPKT);
    lastTerrestrialEndpointCoord_ = terrestrialEndpointCoord;
    lastSatelliteEndpointCoord_ = satelliteEndpointCoord;
    lastTerrestrialEndpointWgs84_ = terrestrialEndpointWgs84;
    lastSatelliteEndpointEcefCoord_ = satelliteEndpointEcefCoord;

    // TODO if satellite is moving, you need to compute speed differently
    double speed = computeSpeed(terrestrialEndpointId, terrestrialEndpointCoord);


    // TODO fix noise figure and gains
    double noiseFigure = receiverType == UE ? ueNoiseFigure_ : bsNoiseFigure_;
    double antennaGainTx = transmitterType == UE ? antennaGainUe_ : antennaGainEnB_;
    double antennaGainRx = receiverType == UE ? antennaGainUe_ : antennaGainEnB_;

    double attenuation = getAttenuation(terrestrialEndpointId, static_cast<Direction>(lteInfo->getDirection()), transmitterCoord, cqiDl);
    double recvPower = lteInfo->getTxPower() - attenuation;
    recvPower += antennaGainTx;
    recvPower += antennaGainRx;
    recvPower -= cableLoss_;

    std::vector<double> snrVector(numBands_, 0.0);
    std::vector<double> fadingAttenuation = computeFrequencySelectiveFading(terrestrialEndpointId, speed);
    for (unsigned int i = 0; i < numBands_; i++) {
        snrVector[i] = recvPower + fadingAttenuation[i];
    }

    RbMap rbmap = lteInfo->getGrantedBlocks();
    double totN = dBmToLinear(thermalNoise_ + noiseFigure);
    for (unsigned int i = 0; i < numBands_; i++) {
        if (lteInfo->getFrameType() == DATAPKT && rbmap[MACRO][i] == 0)
            continue;
        double den = linearToDBm(totN);
        snrVector[i] -= den;
    }

    updatePositionHistory(terrestrialEndpointId, terrestrialEndpointCoord);
    return snrVector;
}

std::vector<double> NtnChannelModel::getRSRP(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    (void)frame;

    GeographicReferenceSystem *referenceSystem = GeographicReferenceSystemAccess().get();
    ASSERT(referenceSystem != nullptr);

    MacNodeId transmitterId = lteInfo->getRadioTransmitterId();
    MacNodeId receiverId = lteInfo->getRadioReceiverId();
    Coord transmitterCoord = lteInfo->getRadioTransmitterCoord();
    Coord receiverCoord = phy_->getRadioPosition();
    RanNodeType transmitterType = getNodeTypeById(transmitterId);
    RanNodeType receiverType = getNodeTypeById(receiverId);
    Coord transmitterEcef = lteInfo->getRadioTransmitterEcefCoord();
    inet::GeoCoord receiverWgs84 = referenceSystem->wgs84FromOmnet(receiverCoord);

    if (transmitterType != SATELLITE_NODE && receiverType != SATELLITE_NODE)
        throw cRuntimeError("NtnChannelModel::getRSRP - one hop endpoint must be the satellite");

    MacNodeId terrestrialEndpointId = transmitterType == SATELLITE_NODE ? receiverId : transmitterId;
    bool cqiDl = (lteInfo->getDirection() == DL && lteInfo->getFrameType() == FEEDBACKPKT);
    Coord terrestrialEndpointCoord = transmitterType == SATELLITE_NODE ? receiverCoord : transmitterCoord;
    Coord satelliteEndpointCoord = transmitterType == SATELLITE_NODE ? transmitterCoord : receiverCoord;
    inet::GeoCoord terrestrialEndpointWgs84 = transmitterType == SATELLITE_NODE ? receiverWgs84 : referenceSystem->wgs84FromOmnet(transmitterCoord);
    Coord satelliteEndpointEcefCoord = transmitterType == SATELLITE_NODE ? transmitterEcef : ecefFromWgs84(receiverWgs84);
    double speed = computeSpeed(terrestrialEndpointId, terrestrialEndpointCoord);
    lastTerrestrialEndpointCoord_ = terrestrialEndpointCoord;
    lastSatelliteEndpointCoord_ = satelliteEndpointCoord;
    lastTerrestrialEndpointWgs84_ = terrestrialEndpointWgs84;
    lastSatelliteEndpointEcefCoord_ = satelliteEndpointEcefCoord;

    double recvPower = lteInfo->getTxPower();

    double antennaGainTx = transmitterType == UE ? antennaGainUe_ : antennaGainEnB_;
    double antennaGainRx = receiverType == UE ? antennaGainUe_ : antennaGainEnB_;
    double attenuation = getAttenuation(terrestrialEndpointId, static_cast<Direction>(lteInfo->getDirection()), transmitterCoord, cqiDl);
    recvPower -= attenuation;
    recvPower += antennaGainTx;
    recvPower += antennaGainRx;
    recvPower -= cableLoss_;

    std::vector<double> rsrpVector(numBands_, 0.0);
    std::vector<double> fadingAttenuation = computeFrequencySelectiveFading(terrestrialEndpointId, speed);
    for (unsigned int i = 0; i < numBands_; i++) {
        rsrpVector[i] = recvPower + fadingAttenuation[i];
    }

    return rsrpVector;
}

} // namespace simu5g
