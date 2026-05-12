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
#include "simu5g/mobility/georeference/GeographicReferenceSystem.h"
#include "simu5g/stack/phy/packet/NtnAirFrame.h"
#include "inet/common/ModuleRefByPar.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <limits>

namespace simu5g {

Define_Module(NtnChannelModel);

namespace {

double clampCosine(double value)
{
    return std::max(-1.0, std::min(1.0, value));
}

double clampAntennaAngle(double angle)
{
    return std::max(0.0, std::min(90.0, angle));
}

void computeGroundEnuBasis(const inet::GeoCoord& wgs84, inet::Coord& east, inet::Coord& north, inet::Coord& up)
{
    double latitudeRad = math::deg2rad(wgs84.latitude.get());
    double longitudeRad = math::deg2rad(wgs84.longitude.get());

    east = inet::Coord(-std::sin(longitudeRad), std::cos(longitudeRad), 0.0);
    north = inet::Coord(-std::sin(latitudeRad) * std::cos(longitudeRad),
            -std::sin(latitudeRad) * std::sin(longitudeRad),
            std::cos(latitudeRad));
    up = inet::Coord(std::cos(latitudeRad) * std::cos(longitudeRad),
            std::cos(latitudeRad) * std::sin(longitudeRad),
            std::sin(latitudeRad));
}

} // namespace

void NtnChannelModel::initialize(int stage)
{
    LteRealisticChannelModel::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        polarizationMismatchLoss_ = par("polarizationMismatchLoss");
        // TODO Apply this once per link when NTN Tx/Rx antenna-model references expose
        // their polarizations and both endpoints are configured with different non-NONE values.
        if (inside_building_)
            useBuildingPenetrationHighLossModel_ = par("useBuildingPenetrationHighLossModel").boolValue();
    }
    else if (stage == INITSTAGE_SIMU5G_PHYSICAL_LAYER) {
        antennaModel_.reference(this, "antennaModelModule", true);
    }
}

double NtnChannelModel::computeAntennaOffAxisAngle(const IAntennaModel *antenna, RanNodeType endpointType, const inet::Coord& endpointEcef,
        const inet::GeoCoord& endpointWgs84, const inet::Coord& peerEcef) const
{
    if (antenna == nullptr)
        throw cRuntimeError("NtnChannelModel::computeAntennaOffAxisAngle - missing antenna model");

    if (antenna->getPointingMode() == AntennaPointingMode::TRACK_PEER)
        return 0.0;

    if (endpointType == SATELLITE_NODE)
        return computeSatelliteAntennaOffAxisAngle(antenna, endpointEcef, peerEcef);
    else
        return computeGroundAntennaOffAxisAngle(antenna, endpointEcef, endpointWgs84, peerEcef);
}

double NtnChannelModel::computeGroundAntennaOffAxisAngle(const IAntennaModel *antenna, const inet::Coord& endpointEcef,
        const inet::GeoCoord& endpointWgs84, const inet::Coord& peerEcef) const
{
    // Convert the configured ground azimuth/elevation into an ECEF boresight
    // vector using the local ENU frame, then compare it with the ECEF LOS vector.
    inet::Coord east;
    inet::Coord north;
    inet::Coord up;
    computeGroundEnuBasis(endpointWgs84, east, north, up);

    inet::Coord los = peerEcef - endpointEcef;
    double losLength = los.length();
    if (losLength <= 1e-9)
        throw cRuntimeError("NtnChannelModel::computeGroundAntennaOffAxisAngle - cannot normalize zero-length LOS vector");
    los /= losLength;

    double azimuthRad = math::deg2rad(antenna->getBoresightAzimuth());
    double elevationRad = math::deg2rad(antenna->getBoresightElevation());
    inet::Coord boresight = east * (std::cos(elevationRad) * std::sin(azimuthRad)) +
            north * (std::cos(elevationRad) * std::cos(azimuthRad)) +
            up * std::sin(elevationRad);

    double boresightLength = boresight.length();
    if (boresightLength <= 1e-9)
        throw cRuntimeError("NtnChannelModel::computeGroundAntennaOffAxisAngle - cannot normalize zero-length boresight vector");
    inet::Coord boresightUnit = boresight / boresightLength;

    double boresightToLosCosine = boresightUnit * los;
    double angle = math::rad2deg(std::acos(clampCosine(boresightToLosCosine)));
    return clampAntennaAngle(angle);
}

double NtnChannelModel::computeSatelliteAntennaOffAxisAngle(const IAntennaModel *antenna, const inet::Coord& endpointEcef, const inet::Coord& peerEcef) const
{
    // Build a satellite local frame from the ECEF radial direction: nadir is
    // toward Earth center, while east/north span the plane orthogonal to nadir.
    // The configured off-nadir/azimuth angles define the boresight in that frame.
    inet::Coord radialUp = endpointEcef;
    double radialUpLength = radialUp.length();
    if (radialUpLength <= 1e-9)
        throw cRuntimeError("NtnChannelModel::computeSatelliteAntennaOffAxisAngle - cannot normalize zero-length radial vector");
    radialUp /= radialUpLength;

    inet::Coord nadir = -radialUp;
    inet::Coord east = inet::Coord::Z_AXIS % radialUp;
    if (east.length() <= 1e-9)
        east = inet::Coord::Y_AXIS % radialUp;

    double eastLength = east.length();
    if (eastLength <= 1e-9)
        throw cRuntimeError("NtnChannelModel::computeSatelliteAntennaOffAxisAngle - cannot normalize zero-length east vector");
    east /= eastLength;

    inet::Coord north = radialUp % east;
    double northLength = north.length();
    if (northLength <= 1e-9)
        throw cRuntimeError("NtnChannelModel::computeSatelliteAntennaOffAxisAngle - cannot normalize zero-length north vector");
    north /= northLength;

    double offNadirRad = math::deg2rad(antenna->getOffNadirAngle());
    double azimuthRad = math::deg2rad(antenna->getBoresightAzimuth());
    inet::Coord azimuthDirection = north * std::cos(azimuthRad) + east * std::sin(azimuthRad);
    inet::Coord boresight = nadir * std::cos(offNadirRad) + azimuthDirection * std::sin(offNadirRad);
    inet::Coord los = peerEcef - endpointEcef;
    double losLength = los.length();
    if (losLength <= 1e-9)
        throw cRuntimeError("NtnChannelModel::computeSatelliteAntennaOffAxisAngle - cannot normalize zero-length LOS vector");
    los /= losLength;

    double boresightLength = boresight.length();
    if (boresightLength <= 1e-9)
        throw cRuntimeError("NtnChannelModel::computeSatelliteAntennaOffAxisAngle - cannot normalize zero-length boresight vector");
    inet::Coord boresightUnit = boresight / boresightLength;

    double boresightToLosCosine = boresightUnit * los;
    double angle = math::rad2deg(std::acos(clampCosine(boresightToLosCosine)));
    return clampAntennaAngle(angle);
}

std::vector<double> NtnChannelModel::computeReceptionSinr(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    auto *ntnFrame = dynamic_cast<NtnAirFrame *>(frame);
    if (ntnFrame == nullptr || !ntnFrame->hasRelayHopSinr())
        return LteRealisticChannelModel::computeReceptionSinr(frame, lteInfo);

    // compute SINR on the last hop
    std::vector<double> localHopSinr = getSINR(frame, lteInfo);
    // obtain the relay hop SINR
    std::vector<double> relayHopSinr = ntnFrame->getRelayHopSinrVector();
    if (relayHopSinr.size() != localHopSinr.size()) {
        throw cRuntimeError("NtnChannelModel::computeReceptionSinr - relay-hop SINR vector size %lu differs from local-hop SINR vector size %lu",
                static_cast<unsigned long>(relayHopSinr.size()), static_cast<unsigned long>(localHopSinr.size()));
    }

    // use harmonic formula to combine service and feeder link SINRs (see Maral-Bousequets, chap 5)
    std::vector<double> combinedSinr(localHopSinr.size(), 0.0);
    for (size_t i = 0; i < combinedSinr.size(); i++) {
        double relayLinear = dBToLinear(relayHopSinr[i]);
        double localLinear = dBToLinear(localHopSinr[i]);
        double combinedSinrLinear = 1.0 / (1.0 / relayLinear + 1.0 / localLinear);
        combinedSinr[i] = linearToDb(combinedSinrLinear);
        EV_DEBUG << "NtnChannelModel::computeReceptionSinr - band[" << i << "] 1st-hop SINR[" << relayHopSinr[i] << "dB] 2nd-hop SINR[" << localHopSinr[i]
                    << "dB] combined[" << combinedSinr[i] << "]" << endl;
    }

    return combinedSinr;
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
    double basePL = computePathLoss(distance, dbp, los);
    // add O2I penetration loss
    double o2iLoss = computeBuildingPenetrationLoss(nodeId);
    // add atmospheric loss
    double atmLoss = computeAtmosphericLoss();
    // add scintillation loss
    double scintLoss = computeScintillationLoss();
    // add shadowing
    double shadowing = 0.0;
    if (num(nodeId) < BGUE_MIN_ID && shadowing_)
        shadowing = computeShadowing(distance, nodeId, speed, cqiDl);

    // compute attenuation
    double attenuation = basePL + o2iLoss + atmLoss + scintLoss + shadowing;
    updatePositionHistory(nodeId, lastTerrestrialEndpointCoord_);
    updateCorrelationDistance(nodeId, lastTerrestrialEndpointCoord_);

    EV_DEBUG << "NtnChannelModel::getAttenuation - path loss[" << basePL << "dB], o2iLoss[" << o2iLoss << "dB] "
                << "atmLoss[" << atmLoss << "dB], scintLoss[" << scintLoss << "dB], shadowing[" << shadowing << "dB]" << endl;
    EV_INFO << "NtnChannelModel::getAttenuation - computed attenuation at distance " << distance << " is " << attenuation << endl;

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

    EV_DEBUG << "NtnChannelModel::computePathLoss - path loss[" << pathLoss << "dB] clutter loss[" << clutterLoss << "dB] "
            << "total[" << pathLoss + clutterLoss << "dB]" << endl;

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

    double inverseNormalCdf = inverseStandardNormalCdf(probabilityIt->second);
    double a = inverseNormalCdf * sigma1 + mu1;
    double b = inverseNormalCdf * sigma2 + mu2;
    constexpr double c = -3.0;

    double penetrationLoss = 10.0 * std::log10(std::pow(10.0, 0.1 * a) + std::pow(10.0, 0.1 * b) + std::pow(10.0, 0.1 * c));
    EV_DEBUG << " NtnChannelModel::computeBuildingPenetrationLoss - O2I penetration loss[" << penetrationLoss << "dB]" << endl;
    return penetrationLoss;
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

    double atmLoss = kAtmosphericAbsorption[roundedFrequencyGHz] / sinElevation;
    EV_DEBUG << " NtnChannelModel::computeBuildingPenetrationLoss - atmospheric absorption loss[" << atmLoss << "dB]" << endl;
    return atmLoss;
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

    EV_DEBUG << " NtnChannelModel::computeBuildingPenetrationLoss - scintillation loss[" << kTroposphericScintillationLoss[angleIndex] << "dB]" << endl;
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

    const IAntennaModel* txAntenna = lteInfo->getRadioTransmitterAntenna();
    MacNodeId transmitterId = lteInfo->getRadioTransmitterId();
    MacNodeId receiverId = lteInfo->getRadioReceiverId();
    Coord transmitterCoord = lteInfo->getRadioTransmitterCoord();
    Coord receiverCoord = phy_->getRadioPosition();
    RanNodeType transmitterType = getNodeTypeById(transmitterId);
    RanNodeType receiverType = getNodeTypeById(receiverId);
    inet::GeoCoord transmitterWgs84 = referenceSystem->wgs84FromOmnet(transmitterCoord);
    inet::GeoCoord receiverWgs84 = referenceSystem->wgs84FromOmnet(receiverCoord);
    Coord transmitterEcef = transmitterType == SATELLITE_NODE ? lteInfo->getRadioTransmitterEcefCoord() : ecefFromWgs84(transmitterWgs84);
    Coord receiverEcef = ecefFromWgs84(receiverWgs84);

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
        terrestrialEndpointWgs84 = transmitterWgs84;
        satelliteEndpointEcefCoord = receiverEcef;
    }

    bool cqiDl = (lteInfo->getDirection() == DL && lteInfo->getFrameType() == FEEDBACKPKT);
    lastTerrestrialEndpointCoord_ = terrestrialEndpointCoord;
    lastSatelliteEndpointCoord_ = satelliteEndpointCoord;
    lastTerrestrialEndpointWgs84_ = terrestrialEndpointWgs84;
    lastSatelliteEndpointEcefCoord_ = satelliteEndpointEcefCoord;

    Coord terrestrialEndpointEcef = ecefFromWgs84(terrestrialEndpointWgs84);
    double elevation = computeElevationFromEcefEndpoints(terrestrialEndpointWgs84, terrestrialEndpointEcef, satelliteEndpointEcefCoord);
    if (elevation < 0.0) {
        EV_DEBUG << "NtnChannelModel::getSINR - satellite below local horizon, elevation[" << elevation
                 << " deg], returning no-signal SINR" << endl;
        updatePositionHistory(terrestrialEndpointId, terrestrialEndpointCoord);
        return std::vector<double>(numBands_, -INFINITY);
    }

    // TODO if satellite is moving, you need to compute speed differently
    double speed = computeSpeed(terrestrialEndpointId, terrestrialEndpointCoord);

    double frequency = Hz(lteInfo->getCarrierFrequency()).get();

    double txAngle = computeAntennaOffAxisAngle(txAntenna, transmitterType, transmitterEcef, transmitterWgs84, receiverEcef);
    double antennaGainTx = txAntenna->computeTxGain(txAngle, frequency);
    double antennaLossTx = txAntenna->getTxFeederLoss();

    double rxAngle = computeAntennaOffAxisAngle(antennaModel_, receiverType, receiverEcef, receiverWgs84, transmitterEcef);
    double antennaGainRx = antennaModel_->computeRxGain(rxAngle, frequency);
    double antennaLossRx = antennaModel_->getRxFeederLoss() + antennaModel_->getRxLumpedLoss();

    double attenuation = getAttenuation(terrestrialEndpointId, static_cast<Direction>(lteInfo->getDirection()), transmitterCoord, cqiDl);
    double recvPower = lteInfo->getTxPower() - attenuation + antennaGainTx - antennaLossTx + antennaGainRx - antennaLossRx;

    EV_DEBUG << " NtnChannelModel::getSINR - recvPower[" << recvPower
       << "dBm] antenna gain tx["  << antennaGainTx << "dB] antenna gain rx[" << antennaGainRx
       << "dB] tx antenna loss[" << antennaLossTx << "dB] rx antenna loss [" << antennaLossRx
       << "dB] attenuation " << attenuation << endl;

    std::vector<double> snrVector(numBands_, 0.0);
    std::vector<double> fadingAttenuation = computeFrequencySelectiveFading(terrestrialEndpointId, speed);
    for (unsigned int i = 0; i < numBands_; i++) {
        snrVector[i] = recvPower + fadingAttenuation[i];
        EV_DEBUG << " NtnChannelModel::getSINR - band[" << i << "] fast fading[" << fadingAttenuation[i] << "dB] rx power[" << snrVector[i] << "dBm]" << endl;
    }

    // compute noise over one RB
    constexpr double rbBandwidth = 180000.0;
    double noiseFigure = antennaModel_->getNoiseFigure();
    double thermalNoise = antennaModel_->getThermalNoise(rbBandwidth);

    RbMap rbmap = lteInfo->getGrantedBlocks();
    double totN = dBmToLinear(thermalNoise + noiseFigure);
    for (unsigned int i = 0; i < numBands_; i++) {
        if (lteInfo->getFrameType() == DATAPKT && rbmap[MACRO][i] == 0)
            continue;
        double den = linearToDBm(totN);
        snrVector[i] -= den;

        EV_INFO << " NtnChannelModel::getSINR - band[" << i << "]" << " noise figure[" << noiseFigure << "dB]" << " thermal noise[" << thermalNoise << "dB]"
                << " snr[" << snrVector[i] << "dB]" << endl;
    }

    updatePositionHistory(terrestrialEndpointId, terrestrialEndpointCoord);
    return snrVector;
}

std::vector<double> NtnChannelModel::getRSRP(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    (void)frame;

    GeographicReferenceSystem *referenceSystem = GeographicReferenceSystemAccess().get();
    ASSERT(referenceSystem != nullptr);

    const IAntennaModel* txAntenna = lteInfo->getRadioTransmitterAntenna();
    MacNodeId transmitterId = lteInfo->getRadioTransmitterId();
    MacNodeId receiverId = lteInfo->getRadioReceiverId();
    Coord transmitterCoord = lteInfo->getRadioTransmitterCoord();
    Coord receiverCoord = phy_->getRadioPosition();
    RanNodeType transmitterType = getNodeTypeById(transmitterId);
    RanNodeType receiverType = getNodeTypeById(receiverId);
    inet::GeoCoord transmitterWgs84 = referenceSystem->wgs84FromOmnet(transmitterCoord);
    inet::GeoCoord receiverWgs84 = referenceSystem->wgs84FromOmnet(receiverCoord);
    Coord transmitterEcef = transmitterType == SATELLITE_NODE ? lteInfo->getRadioTransmitterEcefCoord() : ecefFromWgs84(transmitterWgs84);
    Coord receiverEcef = ecefFromWgs84(receiverWgs84);

    if (transmitterType != SATELLITE_NODE && receiverType != SATELLITE_NODE)
        throw cRuntimeError("NtnChannelModel::getRSRP - one hop endpoint must be the satellite");

    MacNodeId terrestrialEndpointId = transmitterType == SATELLITE_NODE ? receiverId : transmitterId;
    bool cqiDl = (lteInfo->getDirection() == DL && lteInfo->getFrameType() == FEEDBACKPKT);
    Coord terrestrialEndpointCoord = transmitterType == SATELLITE_NODE ? receiverCoord : transmitterCoord;
    Coord satelliteEndpointCoord = transmitterType == SATELLITE_NODE ? transmitterCoord : receiverCoord;
    inet::GeoCoord terrestrialEndpointWgs84 = transmitterType == SATELLITE_NODE ? receiverWgs84 : transmitterWgs84;
    Coord satelliteEndpointEcefCoord = transmitterType == SATELLITE_NODE ? transmitterEcef : receiverEcef;
    double speed = computeSpeed(terrestrialEndpointId, terrestrialEndpointCoord);
    lastTerrestrialEndpointCoord_ = terrestrialEndpointCoord;
    lastSatelliteEndpointCoord_ = satelliteEndpointCoord;
    lastTerrestrialEndpointWgs84_ = terrestrialEndpointWgs84;
    lastSatelliteEndpointEcefCoord_ = satelliteEndpointEcefCoord;

    Coord terrestrialEndpointEcef = ecefFromWgs84(terrestrialEndpointWgs84);
    double elevation = computeElevationFromEcefEndpoints(terrestrialEndpointWgs84, terrestrialEndpointEcef, satelliteEndpointEcefCoord);
    if (elevation < 0.0) {
        EV_DEBUG << "NtnChannelModel::getRSRP - satellite below local horizon, elevation[" << elevation
                 << " deg], returning no-signal RSRP" << endl;
        return std::vector<double>(numBands_, -INFINITY);
    }

    double frequency = Hz(lteInfo->getCarrierFrequency()).get();

    double txAngle = computeAntennaOffAxisAngle(txAntenna, transmitterType, transmitterEcef, transmitterWgs84, receiverEcef);
    double antennaGainTx = txAntenna->computeTxGain(txAngle, frequency);
    double antennaLossTx = txAntenna->getTxFeederLoss();

    double rxAngle = computeAntennaOffAxisAngle(antennaModel_, receiverType, receiverEcef, receiverWgs84, transmitterEcef);
    double antennaGainRx = antennaModel_->computeRxGain(rxAngle, frequency);
    double antennaLossRx = antennaModel_->getRxFeederLoss() + antennaModel_->getRxLumpedLoss();

    double attenuation = getAttenuation(terrestrialEndpointId, static_cast<Direction>(lteInfo->getDirection()), transmitterCoord, cqiDl);
    double recvPower = lteInfo->getTxPower() - attenuation + antennaGainTx - antennaLossTx + antennaGainRx - antennaLossRx;

    EV_DEBUG << " NtnChannelModel::getSINR - recvPower[" << recvPower
       << "dBm] antenna gain tx["  << antennaGainTx << "dB] antenna gain rx[" << antennaGainRx
       << "dB] tx antenna loss   " << antennaLossTx << "dB] rx antenna loss [" << antennaLossRx
       << "dB] attenuation " << attenuation << endl;

    std::vector<double> rsrpVector(numBands_, 0.0);
    std::vector<double> fadingAttenuation = computeFrequencySelectiveFading(terrestrialEndpointId, speed);
    for (unsigned int i = 0; i < numBands_; i++) {
        rsrpVector[i] = recvPower + fadingAttenuation[i];
        EV_DEBUG << " NtnChannelModel::getSINR - band[" << i << "] fast fading[" << fadingAttenuation[i] << "dB] rx power[" << rsrpVector[i] << "dBm]" << endl;
    }

    return rsrpVector;
}

} // namespace simu5g
