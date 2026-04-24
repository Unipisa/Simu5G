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

namespace simu5g {

static constexpr std::array<double, 9> kDenseUrbanLosProb = {0.282, 0.331, 0.398, 0.468, 0.537, 0.612, 0.738, 0.820, 0.981};
static constexpr std::array<double, 9> kUrbanLosProb = {0.246, 0.386, 0.493, 0.613, 0.726, 0.805, 0.919, 0.968, 0.992};
static constexpr std::array<double, 9> kSuburbanRuralLosProb = {0.782, 0.869, 0.919, 0.929, 0.935, 0.940, 0.949, 0.952, 0.998};

Define_Module(NtnChannelModel);

double NtnChannelModel::getAttenuation(MacNodeId nodeId, Direction dir, inet::Coord coord, bool cqiDl)
{
    double speed = .0;
    double correlationDist = .0;

    double distance = phy_->getRadioPosition().distance(coord);
    if (dir == DL) {
        lastGroundCoord_ = phy_->getRadioPosition();
        lastNtnCoord_ = coord;
    }
    else {
        lastGroundCoord_ = coord;
        lastNtnCoord_ = phy_->getRadioPosition();
    }

    if (dir == DL) {
        speed = computeSpeed(nodeId, phy_->getRadioPosition());
        correlationDist = computeCorrelationDistance(nodeId, phy_->getRadioPosition());
    }
    else {
        speed = computeSpeed(nodeId, coord);
        correlationDist = computeCorrelationDistance(nodeId, coord);
    }

    if (correlationDist > correlationDistance_ || losMap_.find(nodeId) == losMap_.end())
        computeLosProbability(distance, nodeId);

    bool los = losMap_[nodeId];
    double dbp = 0;
    double attenuation = computePathLoss(distance, dbp, los);

    if (num(nodeId) < BGUE_MIN_ID && shadowing_)
        attenuation += computeShadowing(distance, nodeId, speed, cqiDl);

    if (dir == DL) {
        updatePositionHistory(nodeId, phy_->getRadioPosition());
        updateCorrelationDistance(nodeId, phy_->getRadioPosition());
    }
    else {
        updatePositionHistory(nodeId, coord);
        updateCorrelationDistance(nodeId, coord);
    }

    EV << "NtnChannelModel::getAttenuation - computed attenuation at distance " << distance
       << " is " << attenuation << endl;

    return attenuation;
}

void NtnChannelModel::computeLosProbability(double d, MacNodeId nodeId)
{
    (void)d;

    if (!dynamicLos_) {
        losMap_[nodeId] = fixedLos_;
        return;
    }

    double elevationAngle = computeVerticalAngle(lastGroundCoord_, lastNtnCoord_);
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

    MacNodeId transmitterId = lteInfo->getRadioTransmitterId() != NODEID_NONE ? lteInfo->getRadioTransmitterId() : lteInfo->getSourceId();
    MacNodeId receiverId = lteInfo->getRadioReceiverId() != NODEID_NONE ? lteInfo->getRadioReceiverId() : lteInfo->getDestId();
    Coord transmitterCoord = transmitterId != NODEID_NONE ? lteInfo->getRadioTransmitterCoord() : lteInfo->getCoord();
    Coord receiverCoord = phy_->getRadioPosition();
    RanNodeType transmitterType = transmitterId != NODEID_NONE ? getNodeTypeById(transmitterId) : UNKNOWN_NODE_TYPE;
    RanNodeType receiverType = receiverId != NODEID_NONE ? getNodeTypeById(receiverId) : UNKNOWN_NODE_TYPE;
    MacNodeId stateNodeId = transmitterType == SATELLITE_NODE ? receiverId : transmitterId;
    bool cqiDl = (lteInfo->getDirection() == DL && lteInfo->getFrameType() == FEEDBACKPKT);
    Coord stateCoord = stateNodeId == receiverId ? receiverCoord : transmitterCoord;
    double speed = computeSpeed(stateNodeId, stateCoord);

    double recvPower = lteInfo->getTxPower();
    RbMap rbmap = lteInfo->getGrantedBlocks();

    double noiseFigure = receiverType == UE ? ueNoiseFigure_ : bsNoiseFigure_;
    double antennaGainTx = transmitterType == UE ? antennaGainUe_ : antennaGainEnB_;
    double antennaGainRx = receiverType == UE ? antennaGainUe_ : antennaGainEnB_;

    double attenuation = getAttenuation(stateNodeId, static_cast<Direction>(lteInfo->getDirection()), transmitterCoord, cqiDl);
    recvPower -= attenuation;
    recvPower += antennaGainTx;
    recvPower += antennaGainRx;
    recvPower -= cableLoss_;

    std::vector<double> snrVector(numBands_, 0.0);
    for (unsigned int i = 0; i < numBands_; i++) {
        double fadingAttenuation = 0;
        if (fading_) {
            if (fadingType_ == RAYLEIGH)
                fadingAttenuation = rayleighFading(stateNodeId, i);
            else if (fadingType_ == JAKES)
                fadingAttenuation = jakesFading(stateNodeId, speed, i, false);
        }
        snrVector[i] = recvPower + fadingAttenuation;
    }

    double totN = dBmToLinear(thermalNoise_ + noiseFigure);
    for (unsigned int i = 0; i < numBands_; i++) {
        if (lteInfo->getFrameType() == DATAPKT && rbmap[MACRO][i] == 0)
            continue;
        double den = linearToDBm(totN);
        snrVector[i] -= den;
    }

    updatePositionHistory(stateNodeId, stateCoord);
    return snrVector;
}

std::vector<double> NtnChannelModel::getRSRP(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    (void)frame;

    MacNodeId transmitterId = lteInfo->getRadioTransmitterId() != NODEID_NONE ? lteInfo->getRadioTransmitterId() : lteInfo->getSourceId();
    MacNodeId receiverId = lteInfo->getRadioReceiverId() != NODEID_NONE ? lteInfo->getRadioReceiverId() : lteInfo->getDestId();
    Coord transmitterCoord = transmitterId != NODEID_NONE ? lteInfo->getRadioTransmitterCoord() : lteInfo->getCoord();
    Coord receiverCoord = phy_->getRadioPosition();
    RanNodeType transmitterType = transmitterId != NODEID_NONE ? getNodeTypeById(transmitterId) : UNKNOWN_NODE_TYPE;
    RanNodeType receiverType = receiverId != NODEID_NONE ? getNodeTypeById(receiverId) : UNKNOWN_NODE_TYPE;
    MacNodeId stateNodeId = transmitterType == SATELLITE_NODE ? receiverId : transmitterId;
    bool cqiDl = (lteInfo->getDirection() == DL && lteInfo->getFrameType() == FEEDBACKPKT);
    Coord stateCoord = stateNodeId == receiverId ? receiverCoord : transmitterCoord;
    double speed = computeSpeed(stateNodeId, stateCoord);

    double recvPower = lteInfo->getTxPower();

    double antennaGainTx = transmitterType == UE ? antennaGainUe_ : antennaGainEnB_;
    double antennaGainRx = receiverType == UE ? antennaGainUe_ : antennaGainEnB_;
    double attenuation = getAttenuation(stateNodeId, static_cast<Direction>(lteInfo->getDirection()), transmitterCoord, cqiDl);
    recvPower -= attenuation;
    recvPower += antennaGainTx;
    recvPower += antennaGainRx;
    recvPower -= cableLoss_;

    std::vector<double> rsrpVector(numBands_, 0.0);
    for (unsigned int i = 0; i < numBands_; i++) {
        double fadingAttenuation = 0;
        if (fading_) {
            if (fadingType_ == RAYLEIGH)
                fadingAttenuation = rayleighFading(stateNodeId, i);
            else if (fadingType_ == JAKES)
                fadingAttenuation = jakesFading(stateNodeId, speed, i, false);
        }
        rsrpVector[i] = recvPower + fadingAttenuation;
    }

    return rsrpVector;
}

} // namespace simu5g
