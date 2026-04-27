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
#include <cmath>

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

Define_Module(NtnChannelModel);

double NtnChannelModel::getAttenuation(MacNodeId nodeId, Direction dir, inet::Coord coord, bool cqiDl)
{
    (void)dir;
    (void)coord;

    double speed = computeSpeed(nodeId, lastTerrestrialEndpointCoord_);
    double correlationDist = computeCorrelationDistance(nodeId, lastTerrestrialEndpointCoord_);
    inet::Coord groundEcef = ecefFromWgs84(lastTerrestrialEndpointWgs84_);
    inet::Coord ntnEcef = lastSatelliteEndpointEcefCoord_;
    double distance = groundEcef.distance(ntnEcef);

    if (correlationDist > correlationDistance_ || losMap_.find(nodeId) == losMap_.end())
        computeLosProbability(distance, nodeId);

    bool los = losMap_[nodeId];
    double dbp = 0;
    double attenuation = computePathLoss(distance, dbp, los);

    if (num(nodeId) < BGUE_MIN_ID && shadowing_)
        attenuation += computeShadowing(distance, nodeId, speed, cqiDl);

    updatePositionHistory(nodeId, lastTerrestrialEndpointCoord_);
    updateCorrelationDistance(nodeId, lastTerrestrialEndpointCoord_);

    EV << "NtnChannelModel::getAttenuation - computed attenuation at distance " << distance
       << " is " << attenuation << endl;

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

    inet::Coord terrestrialEndpointEcef = ecefFromWgs84(lastTerrestrialEndpointWgs84_);
    double elevationAngle = computeElevationFromEcefEndpoints(lastTerrestrialEndpointWgs84_, terrestrialEndpointEcef, lastSatelliteEndpointEcefCoord_);
    int roundedAngle = static_cast<int>(std::round(elevationAngle / 10.0)) * 10;
    roundedAngle = std::max(10, std::min(90, roundedAngle));
    int angleIndex = roundedAngle / 10 - 1;

    double clutterLoss = carrierFrequency_.get() < 13.0 ? kSbandClutterLoss[angleIndex] : kKabandClutterLoss[angleIndex];
    return pathLoss + clutterLoss;
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
    for (unsigned int i = 0; i < numBands_; i++) {
        double fadingAttenuation = 0;
        if (fading_) {
            if (fadingType_ == RAYLEIGH)
                fadingAttenuation = rayleighFading(terrestrialEndpointId, i);
            else if (fadingType_ == JAKES)
                fadingAttenuation = jakesFading(terrestrialEndpointId, speed, i, false);
        }
        snrVector[i] = recvPower + fadingAttenuation;
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
    for (unsigned int i = 0; i < numBands_; i++) {
        double fadingAttenuation = 0;
        if (fading_) {
            if (fadingType_ == RAYLEIGH)
                fadingAttenuation = rayleighFading(terrestrialEndpointId, i);
            else if (fadingType_ == JAKES)
                fadingAttenuation = jakesFading(terrestrialEndpointId, speed, i, false);
        }
        rsrpVector[i] = recvPower + fadingAttenuation;
    }

    return rsrpVector;
}

} // namespace simu5g
