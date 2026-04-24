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
        lastUeCoord_ = phy_->getRadioPosition();
        lastNtnCoord_ = coord;
    }
    else {
        lastUeCoord_ = coord;
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

    double elevationAngle = computeVerticalAngle(lastUeCoord_, lastNtnCoord_);
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

} // namespace simu5g
