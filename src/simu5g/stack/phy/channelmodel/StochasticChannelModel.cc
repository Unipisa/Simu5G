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

#include "simu5g/stack/phy/channelmodel/StochasticChannelModel.h"

#include <fstream>
#include "simu5g/common/cellInfo/CellInfo.h"
#include "simu5g/stack/phy/packet/AirFrame_m.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/stack/mac/amc/UserTxParams.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/nodes/ExtCell.h"
#include "simu5g/background/cell/BackgroundScheduler.h"
#include "simu5g/stack/phy/PhyUe.h"
#include "simu5g/stack/d2d/mac/ID2dMacEnb.h"
#include "simu5g/stack/phy/channelmodel/Tr36814PathLossModel.h"
#include "simu5g/stack/phy/channelmodel/Tr36873PathLossModel.h"
#include "simu5g/stack/phy/channelmodel/Tr38901PathLossModel.h"

namespace simu5g {

using namespace inet;
using namespace omnetpp;
Define_Module(StochasticChannelModel);

simsignal_t StochasticChannelModel::rcvdSinrDlSignal_ = registerSignal("rcvdSinrDl");
simsignal_t StochasticChannelModel::rcvdSinrUlSignal_ = registerSignal("rcvdSinrUl");

simsignal_t StochasticChannelModel::measuredSinrDlSignal_ = registerSignal("measuredSinrDl");
simsignal_t StochasticChannelModel::measuredSinrUlSignal_ = registerSignal("measuredSinrUl");

StochasticChannelModel::~StochasticChannelModel()
{
    delete pathLoss_;
    delete extCellPathLoss_;
}

PathLossModel *StochasticChannelModel::createPathLossModel()
{
    std::string pathLossType = par("pathLossType").stringValue();
    if (pathLossType == "Tr36814")
        return new Tr36814PathLossModel();
    else if (pathLossType == "Tr36873")
        return new Tr36873PathLossModel();
    else if (pathLossType == "Tr38901") {
        auto *model = new Tr38901PathLossModel();
        if (inside_building_)
            useBuildingPenetrationHighLossModel_ = par("useBuildingPenetrationHighLossModel").boolValue();
        model->setUseBuildingPenetrationHighLossModel(useBuildingPenetrationHighLossModel_);
        return model;
    }
    else
        throw cRuntimeError("Unrecognized value in 'pathLossType' parameter: \"%s\"", pathLossType.c_str());
}

void StochasticChannelModel::initialize(int stage)
{
    ChannelModelBase::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        scenario_ = aToDeploymentScenario(par("scenario").stringValue());
        hNodeB_ = par("nodebHeight");
        shadowing_ = par("shadowing");
        hBuilding_ = par("buildingHeight");
        inside_building_ = par("insideBuilding");
        if (inside_building_)
            inside_distance_ = uniform(0.0, 25.0);
        tolerateMaxDistViolation_ = par("tolerateMaxDistViolation");
        hUe_ = par("ueHeight");

        wStreet_ = par("streetWidth");

        correlationDistance_ = par("correlationDistance");
        harqReduction_ = par("harqReduction");

        antennaGainUe_ = par("antennaGainUe");
        antennaGainEnB_ = par("antennGainEnB");
        antennaGainMicro_ = par("antennGainMicro");
        thermalNoise_ = par("thermalNoise");
        cableLoss_ = par("cableLoss");
        ueNoiseFigure_ = par("ueNoiseFigure");
        bsNoiseFigure_ = par("bsNoiseFigure");
        dynamicLos_ = par("dynamicLos");
        fixedLos_ = par("fixedLos");

        fading_ = par("fading");
        std::string fType = par("fadingType");
        if (fType == "JAKES")
            fadingType_ = JAKES;
        else if (fType == "RAYLEIGH")
            fadingType_ = RAYLEIGH;
        else
            throw cRuntimeError("Unrecognized value in 'fadingType' parameter: \"%s\"", fType.c_str());

        fadingPaths_ = par("numFadingPaths");
        enableBackgroundCellInterference_ = par("bgCellInterference");
        enableExtCellInterference_ = par("extCellInterference");
        enableDownlinkInterference_ = par("downlinkInterference");
        enableUplinkInterference_ = par("uplinkInterference");
        delayRMS_ = par("delayRms");

        enable_extCell_los_ = par("enableExtCellLos");

        collectSinrStatistics_ = par("collectSinrStatistics");

        //clear jakes fading map structure
        jakesFadingMap_.clear();

        WATCH(lastCorrelationPoint_);
        WATCH(losMap_);
        WATCH(lastComputedSF_);
        WATCH(jakesFadingMap_);
        WATCH(jakesFadingMapBgUe_);
    }
    else if (stage == INITSTAGE_SIMU5G_POSTLOCAL) {
        // carrierFrequencyHz_/GHz_/log10CarrierFrequencyGHz_ have just been set
        // by ChannelModelBase::initialize() above, in this same stage
        pathLoss_ = createPathLossModel();
        pathLoss_->initialize(this, scenario_, hNodeB_, hUe_, hBuilding_, wStreet_,
                inside_building_, inside_distance_,
                carrierFrequencyHz_, carrierFrequencyGHz_, log10CarrierFrequencyGHz_,
                tolerateMaxDistViolation_);
        extCellPathLoss_ = new Tr36814PathLossModel();
        extCellPathLoss_->initialize(this, scenario_, hNodeB_, hUe_, hBuilding_, wStreet_,
                inside_building_, inside_distance_,
                carrierFrequencyHz_, carrierFrequencyGHz_, log10CarrierFrequencyGHz_,
                tolerateMaxDistViolation_);
    }
}

RadioLink StochasticChannelModel::cellularLink(MacNodeId ueId, Direction dir, Coord coord, bool cqiDl)
{
    // The local module is one endpoint and 'coord' the other; 'dir' says which
    // of the two is the UE. The UE is the node whose channel state we track.
    RadioLink link;
    link.dir = dir;
    // A cellular link: the degenerate key {ueId, ueId} reproduces the historical
    // node-keyed behavior exactly, since a UE has one such link per instance.
    link.stateKey = LinkKey(ueId);
    link.stateNodeId = ueId;
    link.useUeSideMaps = cqiDl;

    if (dir == DL) { // the local module is the UE, 'coord' is the BS
        link.txIsBaseStation = true;
        link.txCoord = coord;
        link.rxCoord = phy_->getCoord();
        link.stateCoord = phy_->getCoord();
        link.rxId = ueId;
    }
    else { // the local module is the BS, 'coord' is the UE
        link.txIsBaseStation = false;
        link.txCoord = coord;
        link.rxCoord = phy_->getCoord();
        link.stateCoord = coord;
        link.txId = ueId;
    }
    return link;
}

RadioLink StochasticChannelModel::linkFor(UserControlInfo *lteInfo)
{
    RadioLink link;
    link.dir = lteInfo->getDirection();

    // The object associated with the packet: the eNodeB if the direction is DL,
    // the UE if it is UL.
    Coord coord = lteInfo->getCoord();

    MacNodeId ueId, eNbId;
    Coord ueCoord, enbCoord;

    /*
     * If the direction is DL and this is not a feedback packet, this function has been
     * called by isReceptionSuccessful() in the UE: downlink error computation.
     */
    if (link.dir == DL && (lteInfo->getFrameType() != FEEDBACKPKT)) {
        ueId = lteInfo->getDestId();
        eNbId = lteInfo->getSourceId();
        ueCoord = phy_->getCoord();
        enbCoord = coord;
        link.useUeSideMaps = false;
    }
    /*
     * If the direction is UL, or the packet is a feedback packet, this function is called
     * by the feedback computation module located in the eNodeB, which computes the feedback
     * received from the UE. Hence the UE macNodeId comes from the sourceId of the lteInfo.
     */
    else { // UL/DL CQI & UL error computation
        ueId = lteInfo->getSourceId();
        eNbId = lteInfo->getDestId();
        ueCoord = coord;
        enbCoord = phy_->getCoord();
        // for a DL CQI we need the maps stored on the UE side
        link.useUeSideMaps = (link.dir == DL);
    }

    if (link.dir == DL) {
        link.noiseFigure = ueNoiseFigure_;    // dB
        link.txAntennaGain = antennaGainEnB_; // dB
        link.rxAntennaGain = antennaGainUe_;  // dB
        link.txIsBaseStation = true;
        link.txId = eNbId;
        link.rxId = ueId;
        link.txCoord = enbCoord;
        link.rxCoord = ueCoord;
    }
    else { // if( dir == UL )
        // TODO check if antennaGainEnB should be added in UL direction too
        link.txAntennaGain = antennaGainUe_;
        link.rxAntennaGain = antennaGainEnB_;
        link.noiseFigure = bsNoiseFigure_;
        link.txIsBaseStation = false;
        link.txId = ueId;
        link.rxId = eNbId;
        link.txCoord = ueCoord;
        link.rxCoord = enbCoord;
    }

    // The UE owns the channel state, and it is always the UE's position that feeds
    // the speed and correlation-distance computation -- which is why the old code's
    // "pass UL for a FEEDBACKPKT" special case is not needed here: it only existed
    // to make getAttenuation() pick 'coord' rather than phy_->getCoord().
    link.stateKey = LinkKey(ueId);
    link.stateNodeId = ueId;
    link.stateCoord = ueCoord;

    // the cell this link belongs to, for the interference computation
    link.cellId = eNbId;

    return link;
}

double StochasticChannelModel::getAttenuation(const RadioLink& link)
{
    // COMPUTE 3D and 2D DISTANCE between the two endpoints
    double threeDimDistance = link.txCoord.distance(link.rxCoord);
    double twoDimDistance = getTwoDimDistance(link.txCoord, link.rxCoord);

    double speed = computeSpeed(link.stateNodeId, link.stateCoord);
    double correlationDist = computeCorrelationDistance(link.stateKey, link.stateCoord);

    // If Euclidean distance since last LOS probability computation is greater than
    // correlation distance the UE could have changed its state and
    // its visibility from eNodeB, hence it is correct to recompute the LOS probability
    if (correlationDist > correlationDistance_
        || losMap_.find(link.stateKey) == losMap_.end())
    {
        computeLosProbability(threeDimDistance, twoDimDistance, link.stateKey);
    }

    //compute attenuation based on selected scenario and based on LOS or NLOS
    bool los = losMap_[link.stateKey];
    double attenuation = computePathLoss(threeDimDistance, twoDimDistance, los);

    //    Applying shadowing only if it is enabled by configuration
    //    log-normal shadowing (not available for background UEs)
    if (num(link.stateNodeId) < BGUE_MIN_ID && shadowing_)
        attenuation += computeShadowing(threeDimDistance, twoDimDistance, link.stateKey, link.stateNodeId, speed, link.useUeSideMaps);

    // update the tracked node's current position
    updatePositionHistory(link.stateNodeId, link.stateCoord);
    updateCorrelationDistance(link.stateKey, link.stateCoord);

    EV << "StochasticChannelModel::getAttenuation - computed attenuation at distance " << threeDimDistance << " for eNB is " << attenuation << endl;

    return attenuation;
}

double StochasticChannelModel::computeShadowing(double d3D, double d2D, const LinkKey& key, MacNodeId ownerId, double speed, bool cqiDl)
{
    ShadowFadingMap *actualShadowingMap;

    if (cqiDl) // if we are computing a DL CQI we need the Shadowing Map stored on the UE side
        actualShadowingMap = obtainShadowingMap(ownerId);
    else
        actualShadowingMap = &lastComputedSF_;

    double mean = 0;

    // Get std deviation according to LOS/NLOS and selected scenario
    double stdDev = pathLoss_->getShadowingStdDev(d3D, d2D, losMap_[key]);
    double time = 0;
    double space = 0;
    double att;

    // if direction is DOWNLINK it means that this module is located in the UE stack than
    // the Move object associated with the UE is myMove_ variable
    // if direction is UPLINK it means that this module is located in the UE stack than
    // the Move object associated with the UE is move variable

    // if shadowing for current user has never been computed
    if (actualShadowingMap->find(key) == actualShadowingMap->end()) {
        //Get the log-normal shadowing with std deviation stdDev
        att = normal(mean, stdDev);

        //store the shadowing attenuation for this user and the temporal mark
        std::pair<simtime_t, double> tmp(NOW, att);
        (*actualShadowingMap)[key] = tmp;

        //If the shadowing attenuation has been computed at least one time for this user
        // and the distance traveled by the UE is greater than correlation distance
    }
    else if ((NOW - actualShadowingMap->at(key).first).dbl() * speed
             > correlationDistance_)
    {

        //get the temporal mark of the last computed shadowing attenuation
        time = (NOW - actualShadowingMap->at(key).first).dbl();

        //compute the traveled distance
        space = time * speed;

        //Compute shadowing with an EAW (Exponential Average Window) (step 1)
        double a = exp(-0.5 * (space / correlationDistance_));

        //Get last shadowing attenuation computed
        double old = actualShadowingMap->at(key).second;

        //Compute shadowing with an EAW (Exponential Average Window) (step 2)
        att = a * old + sqrt(1 - pow(a, 2)) * normal(mean, stdDev);

        // Store the new computed shadowing
        std::pair<simtime_t, double> tmp(NOW, att);
        (*actualShadowingMap)[key] = tmp;

        // if the distance traveled by the UE is smaller than correlation distance shadowing attenuation remains the same
    }
    else {
        att = actualShadowingMap->at(key).second;
    }

    return att;
}

void StochasticChannelModel::updatePositionHistory(const MacNodeId nodeId,
        const Coord coord)
{
    if (positionHistory_.find(nodeId) != positionHistory_.end()) {
        // position already updated for this TTI.
        if (positionHistory_[nodeId].back().first == NOW)
            return;
    }

    // FIXME: possible memory leak
    positionHistory_[nodeId].push(Position(NOW, coord));

    if (positionHistory_[nodeId].size() > 2) // if we have more than a past and a current element
        // drop the oldest one
        positionHistory_[nodeId].pop();
}

void StochasticChannelModel::updateCorrelationDistance(const LinkKey& nodeId, const inet::Coord coord) {

    if (lastCorrelationPoint_.find(nodeId) == lastCorrelationPoint_.end()) {
        // no lastCorrelationPoint set current point.
        lastCorrelationPoint_[nodeId] = Position(NOW, coord);
    }
    else if ((lastCorrelationPoint_[nodeId].first != NOW) &&
             lastCorrelationPoint_[nodeId].second.distance(coord) > correlationDistance_)
    {
        // check simtime_t first
        lastCorrelationPoint_[nodeId] = Position(NOW, coord);
    }
}

double StochasticChannelModel::computeCorrelationDistance(const LinkKey& nodeId, const inet::Coord coord) {
    double dist = 0.0;

    if (lastCorrelationPoint_.find(nodeId) == lastCorrelationPoint_.end()) {
        // no lastCorrelationPoint found. Add current position and return dist = 0.0
        lastCorrelationPoint_[nodeId] = Position(NOW, coord);
    }
    else {
        dist = lastCorrelationPoint_[nodeId].second.distance(coord);
    }
    return dist;
}

double StochasticChannelModel::computeSpeed(const MacNodeId nodeId,
        const Coord coord)
{
    double speed = 0.0;

    if (positionHistory_.find(nodeId) == positionHistory_.end()) {
        // no entries
        return speed;
    }
    else {
        //compute distance traveled from last update by UE (eNodeB position is fixed)

        if (positionHistory_[nodeId].size() == 1) {
            //  the only element refers to the present, return 0
            return speed;
        }

        double movement = positionHistory_[nodeId].front().second.distance(coord);

        if (movement <= 0.0)
            return speed;
        else {
            double time = (NOW.dbl()) - (positionHistory_[nodeId].front().first.dbl());
            if (time <= 0.0) // time not updated since last speed call
                throw cRuntimeError("Multiple entries detected in position history referring to the same time");
            // compute speed
            speed = (movement) / (time);
        }
    }
    return speed;
}

double StochasticChannelModel::computeAngle(Coord center, Coord point) {
    double relx, rely, arcoSen, angle, dist;

    // compute distance between points
    dist = point.distance(center);

    // compute distance along the axis
    relx = point.x - center.x;
    rely = point.y - center.y;

    // compute the arc sine
    arcoSen = asin(rely / dist) * 180.0 / M_PI;

    // adjust the angle depending on the quadrants
    if (relx < 0 && rely > 0) // quadrant II
        angle = 180.0 - arcoSen;
    else if (relx < 0 && rely <= 0) // quadrant III
        angle = 180.0 - arcoSen;
    else if (relx > 0 && rely < 0) // quadrant IV
        angle = 360.0 + arcoSen;
    else
        // quadrant I
        angle = arcoSen;

    return angle;
}

double StochasticChannelModel::computeVerticalAngle(Coord center, Coord point)
{
    double threeDimDistance = center.distance(point);
    double twoDimDistance = getTwoDimDistance(center, point);
    double arccos = acos(twoDimDistance / threeDimDistance) * 180.0 / M_PI;
    return 90 + arccos;
}

double StochasticChannelModel::computeAngularAttenuation(double hAngle, double vAngle) {
    return pathLoss_->computeAngularAttenuation(hAngle, vAngle);
}

std::vector<double> StochasticChannelModel::getSINR(AirFrame *frame, UserControlInfo *lteInfo)
{
    RadioLink link = linkFor(lteInfo);

    EV << "------------ GET SINR ----------------" << endl;

    // The desired signal: path loss, shadowing and fading. getSINR() below adds
    // noise and interference on top of it.
    return getSINR(link, lteInfo, getRSRP(link, lteInfo->getTxPower()));
}

std::vector<double> StochasticChannelModel::getSINR(const RadioLink& link, UserControlInfo *lteInfo, std::vector<double> snrVector)
{
    // Get the Resource Blocks used to transmit this packet
    RbMap rbmap = lteInfo->getGrantedBlocks();

    /*
     * The SINR will be calculated as follows
     *
     *              Pwr
     * SINR = ---------
     *           N  +  I
     *
     * Ndb = thermalNoise_ + noiseFigure (measured in decibels)
     */

    // compute and linearize total noise
    double totN = dBmToLinear(thermalNoise_ + link.noiseFigure);

    // per-band interference-plus-noise denominator, in dBm
    std::vector<double> den(numBands_, 0.0);
    computeInterferencePlusNoise(link, lteInfo, rbmap, totN, den);

    double sumSnr = 0.0;
    int usedRBs = 0;
    for (unsigned int i = 0; i < numBands_; i++) {
        // if we are decoding a data transmission and this RB has not been used, skip it
        // TODO fix for multi-antenna case
        if (lteInfo->getFrameType() == DATAPKT && rbmap[MACRO][i] == 0)
            continue;

        // compute final SINR. Subtraction in dB is equivalent to linear division
        snrVector[i] -= den[i];

        sumSnr += snrVector[i];
        ++usedRBs;
    }

    MacNodeId ueId = link.txIsBaseStation ? link.rxId : link.txId;

    // emit SINR statistic. Only DL and UL have a measured-SINR signal; other link
    // types must not be reported as one of them.
    if (collectSinrStatistics_ && (lteInfo->getFrameType() == FEEDBACKPKT) && usedRBs > 0
        && (link.dir == DL || link.dir == UL))
    {
        // we are on the BS, so we need to retrieve the channel model of the sender
        // XXX I know, there might be a faster way...
        ChannelModelBase *ueChannelModel = binder_->getPhy(ueId)->getChannelModel(lteInfo->getCarrierFrequency());

        if (link.dir == DL) // we are on the UE
            ueChannelModel->emit(measuredSinrDlSignal_, sumSnr / usedRBs);
        else
            ueChannelModel->emit(measuredSinrUlSignal_, sumSnr / usedRBs);
    }

    // if sender is an eNodeB
    if (link.dir == DL)
        // store the position of user
        updatePositionHistory(ueId, phy_->getCoord());
    // sender is a UE
    else
        updatePositionHistory(ueId, lteInfo->getCoord());
    return snrVector;
}

void StochasticChannelModel::computeInterferencePlusNoise(const RadioLink& link, UserControlInfo *lteInfo,
        RbMap& rbmap, double totN, std::vector<double>& den)
{
    // The interference model is cellular-topology-aware (it asks "which cell?"),
    // so recover the UE/BS roles from the link. The propagation math does not need them.
    Direction dir = link.dir;
    MacNodeId ueId = link.txIsBaseStation ? link.rxId : link.txId;
    MacNodeId eNbId = link.cellId;
    Coord ueCoord = link.txIsBaseStation ? link.rxCoord : link.txCoord;
    Coord enbCoord = link.txIsBaseStation ? link.txCoord : link.rxCoord;

    //============ MULTI CELL INTERFERENCE COMPUTATION =================
    // vector containing the sum of multi-cell interference for each band
    std::vector<double> multiCellInterference; // Linear value (mW)
    // prepare data structure
    multiCellInterference.resize(numBands_, 0);
    if (enableDownlinkInterference_ && dir == DL && lteInfo->getFrameType() != BEACONPKT) {
        computeDownlinkInterference(eNbId, ueId, ueCoord, (lteInfo->getFrameType() == FEEDBACKPKT), lteInfo->getCarrierFrequency(), rbmap, &multiCellInterference);
    }
    else if (enableUplinkInterference_ && dir == UL) {
        computeUplinkInterference(eNbId, ueId, (lteInfo->getFrameType() == FEEDBACKPKT), lteInfo->getCarrierFrequency(), rbmap, &multiCellInterference);
    }

    //============ BACKGROUND CELLS INTERFERENCE COMPUTATION =================
    // vector containing the sum of background cell interference for each band
    std::vector<double> bgCellInterference; // Linear value (mW)
    // prepare data structure
    bgCellInterference.resize(numBands_, 0);
    if (enableBackgroundCellInterference_) {
        computeBackgroundCellInterference(ueId, enbCoord, ueCoord, (lteInfo->getFrameType() == FEEDBACKPKT), lteInfo->getCarrierFrequency(), rbmap, dir, &bgCellInterference); // dBm
    }

    //============ EXTCELL INTERFERENCE COMPUTATION =================
    // TODO this might be obsolete as it is replaced by background cell interference
    // vector containing the sum of external cell interference for each band
    std::vector<double> extCellInterference; // Linear value (mW)
    // prepare data structure
    extCellInterference.resize(numBands_, 0);
    if (enableExtCellInterference_ && dir == DL) {
        computeExtCellInterference(eNbId, ueId, ueCoord, (lteInfo->getFrameType() == FEEDBACKPKT), lteInfo->getCarrierFrequency(), &extCellInterference); // dBm
    }

    EV << "StochasticChannelModel::getSINR - distance from my eNb=" << enbCoord.distance(ueCoord) << " - DIR=" << ((dir == DL) ? "DL" : "UL") << endl;

    for (unsigned int i = 0; i < numBands_; i++) {
        // the caller skips these bands too; leave their denominator untouched
        if (lteInfo->getFrameType() == DATAPKT && rbmap[MACRO][i] == 0)
            continue;

        //                  (      mW              +          mW            +  mW  +        mW            )
        den[i] = linearToDBm(bgCellInterference[i] + extCellInterference[i] + totN + multiCellInterference[i]);

        EV << "\t bgCell[" << bgCellInterference[i] << "] - ext[" << extCellInterference[i] << "] - multi[" << multiCellInterference[i]
           << "] - den[" << den[i] << "]\n";
    }
}

std::vector<double> StochasticChannelModel::getRSRP(AirFrame *frame, UserControlInfo *lteInfo)
{
    return getRSRP(linkFor(lteInfo), lteInfo->getTxPower());
}

std::vector<double> StochasticChannelModel::getRSRP(const RadioLink& link, double txPower)
{
    double recvPower = txPower; // dBm

    EV << "StochasticChannelModel::getRSRP - txId=" << link.txId
       << " - rxId=" << link.rxId
       << " - DIR=" << dirToA(link.dir)
       << " - txPwr " << txPower
       << " - txCoord[" << link.txCoord << "] - rxCoord[" << link.rxCoord << "]" << endl;

    // =============== PATH LOSS + SHADOWING + FADING =================
    EV << "\t using parameters - noiseFigure=" << link.noiseFigure
       << " - antennaGainTx=" << link.txAntennaGain << " - antennaGainRx=" << link.rxAntennaGain
       << " - txPwr=" << txPower << " - for nodeId=" << link.stateKey << endl;

    // Speed must be read BEFORE getAttenuation(), which appends to positionHistory_:
    // computeSpeed() derives from that history, so evaluating it afterwards would
    // yield a different value and hence different fading. Load-bearing ordering.
    double speed = computeSpeed(link.stateNodeId, link.stateCoord);

    // attenuation for the desired signal
    double attenuation = getAttenuation(link); // dB

    // compute attenuation (PATHLOSS + SHADOWING)
    recvPower -= attenuation; // (dBm-dB)=dBm

    // add antenna gain
    recvPower += link.txAntennaGain; // (dBm+dB)=dBm
    recvPower += link.rxAntennaGain; // (dBm+dB)=dBm

    // sub cable loss
    recvPower -= cableLoss_; // (dBm-dB)=dBm

    // =============== ANGULAR ATTENUATION =================
    // Only a base station has a sectorial antenna; a UE-to-UE link never gets here.
    if (link.txIsBaseStation) {
        IRadioEndpoint *bsEndpoint = binder_->findPhy(link.txId);

        if (bsEndpoint && bsEndpoint->getTxDirection() == ANISOTROPIC) {
            // get tx angle
            double txAngle = bsEndpoint->getTxAngle();

            // compute the angle between the receiver position and the reference axis,
            // considering the transmitting BS as center
            double ueAngle = computeAngle(link.txCoord, link.rxCoord);

            // compute the reception angle
            double recvAngle = fabs(txAngle - ueAngle);

            if (recvAngle > 180)
                recvAngle = 360 - recvAngle;

            double verticalAngle = computeVerticalAngle(link.txCoord, link.rxCoord);

            // compute attenuation due to sectorial tx
            double angularAtt = computeAngularAttenuation(recvAngle, verticalAngle);

            recvPower -= angularAtt;
        }
        // else, antenna is omni-directional
    }
    // =============== END ANGULAR ATTENUATION =================

    std::vector<double> rsrpVector;
    rsrpVector.resize(numBands_, 0.0);

    // compute and add interference due to fading
    // Apply fading for each band
    // if the phy layer is localized we can assume that for each logical band we have different fading attenuation
    // if the phy layer is distributed the number of logical bands should be set to 1
    double fadingAttenuation = 0;

    // for each logical band
    // FIXME compute fading only for used RBs
    for (unsigned int i = 0; i < numBands_; i++) {
        fadingAttenuation = 0;
        // if fading is enabled
        if (fading_) {
            // Applying fading
            if (fadingType_ == RAYLEIGH)
                fadingAttenuation = rayleighFading(link.stateNodeId, i);

            else if (fadingType_ == JAKES)
                fadingAttenuation = jakesFading(link.stateKey, link.stateNodeId, speed, i, link.useUeSideMaps);
        }
        // add fading contribution to the received power
        double finalRecvPower = recvPower + fadingAttenuation; // (dBm+dB)=dBm

        EV << " StochasticChannelModel::getRSRP node " << link.stateKey
           << " band " << i << " recvPower " << recvPower
           << " direction " << dirToA(link.dir) << " antenna gain tx "
           << link.txAntennaGain << " antenna gain rx " << link.rxAntennaGain
           << " noise figure " << link.noiseFigure
           << " cable loss   " << cableLoss_
           << " attenuation (pathloss + shadowing) " << attenuation
           << " speed " << speed << " thermal noise " << thermalNoise_
           << " fading attenuation " << fadingAttenuation << endl;

        rsrpVector[i] = finalRecvPower;
    }
    // ============ END PATH LOSS + SHADOWING + FADING ===============

    return rsrpVector;
}

std::vector<double> StochasticChannelModel::getSINR_bgUe(AirFrame *frame, UserControlInfo *lteInfo)
{
    //get tx power
    double recvPower = lteInfo->getTxPower(); // dBm

    // get MacId and Direction
    MacNodeId bgUeId = lteInfo->getSourceId();
    MacNodeId eNbId = lteInfo->getDestId();
    Direction dir = lteInfo->getDirection();

    // position of e/gNb and UE
    Coord ueCoord = lteInfo->getCoord();
    Coord enbCoord = phy_->getCoord();

    double antennaGainTx = 0.0;
    double antennaGainRx = 0.0;
    double noiseFigure = 0.0;
    double speed = 0.0;

    // true if we are computing a CQI for the DL direction
    bool cqiDl = false;

    EV << "------------ GET SINR for background UE ----------------" << endl;
    //===================== PARAMETERS SETUP ============================
    /*
     * This function is called on the e/gNodeB side and is similar
     * to what is called when computing feedback
     */
    if (dir == DL) {
        //set noise figure
        noiseFigure = ueNoiseFigure_; //dB
        //set antenna gain figure
        antennaGainTx = antennaGainEnB_; //dB
        antennaGainRx = antennaGainUe_;  //dB
        // use the jakes map on the UE side
        cqiDl = true;
    }
    else { // if( dir == UL )
        // TODO check if antennaGainEnB should be added in UL direction too
        antennaGainTx = antennaGainUe_;
        antennaGainRx = antennaGainEnB_;
        noiseFigure = bsNoiseFigure_;
        // use the jakes map on the eNb side
        cqiDl = false;
    }
    speed = computeSpeed(bgUeId, ueCoord);

    CellInfo *eNbCell = binder_->getCellInfoByNodeId(eNbId);
    const char *eNbTypeString = eNbCell ? (eNbCell->getEnbType() == MACRO_ENB ? "MACRO" : "MICRO") : "NULL";

    EV << "StochasticChannelModel::getSINR_bgUe - DIR=" << ((dir == DL) ? "DL" : "UL")
       << " " << eNbTypeString << " - txPwr " << lteInfo->getTxPower()
       << " - ueCoord[" << ueCoord << "] - enbCoord[" << enbCoord << "] - enbId[" << eNbId << "]" <<
        endl;

    //=================== END PARAMETERS SETUP =======================

    //=============== PATH LOSS =================
    // Note that shadowing and fading effects are not applied here and left FFW

    // UL because we are computing a feedback
    double attenuation = getAttenuation(bgUeId, UL, ueCoord, cqiDl);

    //compute recvPower
    recvPower -= attenuation; // (dBm-dB)=dBm

    //add antenna gain
    recvPower += antennaGainTx; // (dBm+dB)=dBm
    recvPower += antennaGainRx; // (dBm+dB)=dBm
    //sub cable loss
    recvPower -= cableLoss_; // (dBm-dB)=dBm

    // ANGULAR ATTENUATION
    if (dir == DL) {
        //get tx angle
        IRadioEndpoint *bsEndpoint = binder_->findPhy(eNbId);

        if (bsEndpoint && bsEndpoint->getTxDirection() == ANISOTROPIC) {
            // get tx angle
            double txAngle = bsEndpoint->getTxAngle();

            // compute the angle between uePosition and reference axis, considering the eNb as center
            double ueAngle = computeAngle(enbCoord, ueCoord);

            // compute the reception angle between ue and eNb
            double recvAngle = fabs(txAngle - ueAngle);

            if (recvAngle > 180)
                recvAngle = 360 - recvAngle;

            double verticalAngle = computeVerticalAngle(enbCoord, ueCoord);

            // compute attenuation due to sectorial tx
            double angularAtt = computeAngularAttenuation(recvAngle, verticalAngle);

            recvPower -= angularAtt;
        }
        // else, antenna is omni-directional
    }

    std::vector<double> snrVector;
    snrVector.resize(numBands_, recvPower);

    // for each logical band
    double fadingAttenuation = 0;
    for (unsigned int i = 0; i < numBands_; i++) {
        //if fading is enabled
        if (fading_) {
            //Applying fading
            if (fadingType_ == RAYLEIGH)
                fadingAttenuation = rayleighFading(bgUeId, i);

            else if (fadingType_ == JAKES)
                fadingAttenuation = jakesFading(LinkKey(bgUeId), bgUeId, speed, i, cqiDl, true);
        }
        // add fading contribution to the received power
        double finalRecvPower = recvPower + fadingAttenuation; // (dBm+dB)=dBm

        snrVector[i] = finalRecvPower;
    }

    //============ END PATH LOSS + SHADOWING + FADING ===============

    /*
     * The SINR will be calculated as follows
     *
     *           Pwr
     * SINR = ---------
     *         N  +  I
     *
     * Ndb = thermalNoise_ + noiseFigure (measured in decibel)
     * I = extCellInterference + multiCellInterference
     */

    // TODO Interference computation still needs to be implemented

    //============ MULTI CELL INTERFERENCE COMPUTATION =================
    // for background UEs, we only compute CQI
    bool isCqi = true;
    RbMap rbmap;
    //vector containing the sum of multicell interference for each band
    std::vector<double> multiCellInterference; // Linear value (mW)
    // prepare data structure
    multiCellInterference.resize(numBands_, 0);
    if (enableDownlinkInterference_ && dir == DL) {
        computeDownlinkInterference(eNbId, bgUeId, ueCoord, isCqi, lteInfo->getCarrierFrequency(), rbmap, &multiCellInterference);
    }
    else if (enableUplinkInterference_ && dir == UL) {
        computeUplinkInterference(eNbId, bgUeId, isCqi, lteInfo->getCarrierFrequency(), rbmap, &multiCellInterference);
    }

    //============ BACKGROUND CELLS INTERFERENCE COMPUTATION =================
    //vector containing the sum of bg-cell interference for each band
    std::vector<double> bgCellInterference; // Linear value (mW)
    // prepare data structure
    bgCellInterference.resize(numBands_, 0);
    if (enableBackgroundCellInterference_) {
        computeBackgroundCellInterference(bgUeId, enbCoord, ueCoord, isCqi, lteInfo->getCarrierFrequency(), rbmap, dir, &bgCellInterference); // dBm
    }

    //============ EXTCELL INTERFERENCE COMPUTATION =================
    // TODO this might be obsolete as it is replaced by background cell interference
    //vector containing the sum of ext-cell interference for each band
    std::vector<double> extCellInterference; // Linear value (mW)
    // prepare data structure
    extCellInterference.resize(numBands_, 0);
    if (enableExtCellInterference_ && dir == DL) {
        computeExtCellInterference(eNbId, bgUeId, ueCoord, isCqi, lteInfo->getCarrierFrequency(), &extCellInterference); // dBm
    }

    //===================== SINR COMPUTATION ========================
    // compute and linearize total noise
    double totN = dBmToLinear(thermalNoise_ + noiseFigure);

    // add interference for each band
    for (unsigned int i = 0; i < numBands_; i++) {
        // denominator expressed in dBm as (N+extCell+multiCell)
        //               (      mW              +          mW            +  mW  +        mW            )
        double den = linearToDBm(bgCellInterference[i] + extCellInterference[i] + totN + multiCellInterference[i]);

        EV << "\t bgCell[" << bgCellInterference[i] << "] - ext[" << extCellInterference[i] << "] - multi[" << multiCellInterference[i] << "] - recvPwr["
           << dBmToLinear(snrVector[i]) << "] - sinr[" << snrVector[i] - den << "]\n";

        // compute final SINR
        snrVector[i] -= den;
    }

    return snrVector;
}

double StochasticChannelModel::getReceivedPower_bgUe(double txPower, inet::Coord txPos, inet::Coord rxPos, Direction dir, bool losStatus, MacNodeId bsId)
{
    double antennaGainTx = 0.0;
    double antennaGainRx = 0.0;

    EV << NOW << " StochasticChannelModel::getReceivedPower_bgUe" << endl;

    //===================== PARAMETERS SETUP ============================
    if (dir == DL) {
        antennaGainTx = antennaGainEnB_; //dB
        antennaGainRx = antennaGainUe_;  //dB
    }
    else { // if( dir == UL )
        antennaGainTx = antennaGainUe_;
        antennaGainRx = antennaGainEnB_;
    }

    EV << "StochasticChannelModel::getReceivedPower_bgUe - DIR=" << ((dir == DL) ? "DL" : "UL")
       << " - txPwr " << txPower << " - txPos[" << txPos << "] - rxPos[" << rxPos << "] " << endl;
    //=================== END PARAMETERS SETUP =======================

    //=============== PATH LOSS =================
    // Note that shadowing and fading effects are not applied here and left FFW

    //compute attenuation based on selected scenario and based on LOS or NLOS
    double sqrDistance = txPos.distance(rxPos);
    double dbp = 0;
    double attenuation = computePathLoss(sqrDistance, dbp, losStatus);

    //compute recvPower
    double recvPower = txPower - attenuation; // (dBm-dB)=dBm

    //add antenna gain
    recvPower += antennaGainTx; // (dBm+dB)=dBm
    recvPower += antennaGainRx; // (dBm+dB)=dBm
    //sub cable loss
    recvPower -= cableLoss_; // (dBm-dB)=dBm

    // ANGULAR ATTENUATION
    if (dir == DL) {
        //get tx angle
        IRadioEndpoint *bsEndpoint = binder_->findPhy(bsId);

        if (bsEndpoint && bsEndpoint->getTxDirection() == ANISOTROPIC) {
            // get tx angle
            double txAngle = bsEndpoint->getTxAngle();

            // compute the angle between uePosition and reference axis, considering the eNb as center
            double ueAngle = computeAngle(txPos, rxPos);

            // compute the reception angle between ue and eNb
            double recvAngle = fabs(txAngle - ueAngle);

            if (recvAngle > 180)
                recvAngle = 360 - recvAngle;

            double verticalAngle = computeVerticalAngle(txPos, rxPos);

            // compute attenuation due to sectorial tx
            double angularAtt = computeAngularAttenuation(recvAngle, verticalAngle);

            recvPower -= angularAtt;
        }
        // else, antenna is omni-directional
    }
    //============ END PATH LOSS + ANGULAR ATTENUATION ===============

    return recvPower;
}

double StochasticChannelModel::rayleighFading(MacNodeId id,
        unsigned int band)
{
    // get rayleigh variable from trace file
    const int channelndex = 0;
    double temp1 = binder_->phyPisaData.getChannel(channelndex + band);
    return linearToDb(temp1);
}

double StochasticChannelModel::jakesFading(const LinkKey& key, MacNodeId ownerId, double speed,
        unsigned int band, bool cqiDl, bool isBgUe)
{
    /**
     * NOTE: there are two different Jakes maps. One on the UE side and one on the eNB side, with different values.
     *
     * eNB side => used for CQI computation and for error-probability evaluation in UL
     * UE side  => used for error-probability evaluation in DL
     *
     * the one within eNB is referred to the UL direction
     * the one within UE is referred to the DL direction
     *
     * thus the actual map should be chosen carefully (i.e. just check the cqiDL flag)
     */
    JakesFadingMap *actualJakesMap;

    if (cqiDl) // if we are computing a DL CQI we need the Jakes Map stored on the UE side
        actualJakesMap = (!isBgUe) ? obtainUeJakesMap(ownerId) : &jakesFadingMapBgUe_;
    else
        actualJakesMap = &jakesFadingMap_;

    // if this is the first time that we compute fading for current user
    if (actualJakesMap->find(key) == actualJakesMap->end()) {
        // clear the map
        // FIXME: possible memory leak
        (*actualJakesMap)[key].clear();

        // for each band we are going to create a Jakes fading
        for (unsigned int j = 0; j < numBands_; j++) {
            // clear some structure
            JakesFadingData temp;
            temp.angleOfArrival.clear();
            temp.delaySpread.clear();

            // for each fading path
            for (int i = 0; i < fadingPaths_; i++) {
                // get angle of arrivals
                temp.angleOfArrival.push_back(cos(uniform(0, M_PI)));

                // get delay spread
                temp.delaySpread.push_back(exponential(delayRMS_));
            }
            // store the Jakes fading for this user
            (*actualJakesMap)[key].push_back(temp);
        }
    }
    // convert carrier frequency from GHz to Hz
    double f = carrierFrequencyHz_;

    // get transmission time start (TTI = 1ms)
    simtime_t t = simTime().dbl() - 0.001;

    double re_h = 0;
    double im_h = 0;

    const JakesFadingData& actualJakesData = actualJakesMap->at(key).at(band);

    // Compute Doppler shift.
    double doppler_shift = (speed * f) / SPEED_OF_LIGHT;

    for (int i = 0; i < fadingPaths_; i++) {
        // Phase shift due to Doppler => t-selectivity.
        double phi_d = actualJakesData.angleOfArrival[i] * doppler_shift;

        // Phase shift due to delay spread => f-selectivity.
        double phi_i = actualJakesData.delaySpread[i].dbl() * f;

        // Calculate resulting phase due to t-selective and f-selective fading.
        double phi = 2.00 * M_PI * (phi_d * t.dbl() - phi_i);

        // One ring model/Clarke's model plus f-selectivity according to Cavers:
        // Due to isotropic antenna gain pattern on all paths only a^2 can be received on all paths.
        // Since we are interested in attenuation a := 1, attenuation per path is then:
        double attenuation = (1.00 / sqrt(static_cast<double>(fadingPaths_)));

        // Convert to cartesian form and aggregate {Re, Im} over all fading paths.
        re_h = re_h + attenuation * cos(phi);
        im_h = im_h - attenuation * sin(phi);
    }

    // Output: |H_f|^2 = absolute channel impulse response due to fading.
    // Note that this may be >1 due to constructive interference.
    return linearToDb(re_h * re_h + im_h * im_h);
}

bool StochasticChannelModel::isReceptionSuccessful(AirFrame *frame, UserControlInfo *lteInfo, const std::vector<double>& rsrpVector)
{
    EV << "StochasticChannelModel::error" << endl;

    // get codeword
    unsigned char cw = lteInfo->getCw();
    // get number of codewords
    int size = lteInfo->getUserTxParams()->readCqiVector().size();

    // if total number of codewords is equal to 1 the cw index should be only 0
    if (size == 1)
        cw = 0;

    // get cqi used to transmit this cw
    Cqi cqi = lteInfo->getUserTxParams()->readCqiVector()[cw];

    MacNodeId id;
    Direction dir = lteInfo->getDirection();

    // Get MacNodeId of UE
    if (dir == DL)
        id = lteInfo->getDestId();
    else
        id = lteInfo->getSourceId();

    // Get Number of transmission attempts (includes original + retransmissions)
    unsigned char transmissionAttempt = lteInfo->getTxNumber();

    // consistency check
    if (transmissionAttempt == 0)
        throw cRuntimeError("Transmissions counter should not be 0");

    // Take sinr
    // Take sinr (the D2D channel model overrides getReceptionSinr() to route
    // D2D/D2D_MULTI receptions through getSINR_D2D)
    std::vector<double> snrV = getReceptionSinr(frame, lteInfo, rsrpVector);

    // Get the resource Block id used to transmit this packet
    RbMap rbmap = lteInfo->getGrantedBlocks();

    double blockErrorRate = 0.0;
    double cumulativeSuccessProbability = 1.0;

    // for statistical purposes
    double sumSnr = 0.0;
    int usedRBs = 0;

    // for each Remote unit used to transmit the packet
    for (const auto &[remoteUnit, rbList] : rbmap) {
        // for each logical band used to transmit the packet
        for (const auto &[band, allocation] : rbList) {
            // this Rb is not allocated
            if (allocation == 0)
                continue;

            // Get the Bler
            if (cqi == 0)
                return false; // CQI 0 means channel below usable quality (e.g. after handover) — loss
            if (cqi > 15)
                throw cRuntimeError("A packet has been transmitted with a cqi greater than 15 cqi:%d txmode:%d dir:%d rb:%d cw:%d rtx:%d", cqi, lteInfo->getTxMode(), dir, band, cw, transmissionAttempt);

            // for statistical purposes
            sumSnr += snrV[band];
            usedRBs++;

            int snr = snrV[band];// XXX because band is a Band (=unsigned short)
            if (snr < binder_->phyPisaData.minSnr())
                return false;
            else if (snr > binder_->phyPisaData.maxSnr())
                blockErrorRate = 0.0;
            else
                blockErrorRate = binder_->phyPisaData.getBler(cqi, snr);

            EV << "\t bler computation: [cqi=" << cqi
               << "] - [snr=" << snr << "]" << endl;

            double blockSuccessRate = 1.0 - blockErrorRate;
            // compute the success probability according to the number of RB used
            double allocationSuccessProbability = pow(blockSuccessRate, (double)allocation);
            // compute the success probability according to the number of LB used
            cumulativeSuccessProbability *= allocationSuccessProbability;

            EV << " StochasticChannelModel::error direction " << dirToA(dir)
               << " node " << id << " remote unit " << dasToA(remoteUnit)
               << " Band " << band << " SNR " << snr << " CQI " << cqi
               << " BLER " << blockErrorRate << " success probability " << allocationSuccessProbability
               << " total success probability " << cumulativeSuccessProbability << endl;
        }
    }
    // Compute total error probability
    double packetErrorRate = 1.0 - cumulativeSuccessProbability;
    // Apply HARQ soft combining gain
    double effectiveErrorRateWithHarq = packetErrorRate * pow(harqReduction_, transmissionAttempt - 1);

    double randomSample = uniform(0.0, 1.0);

    EV << " StochasticChannelModel::error direction " << dirToA(dir)
       << " node " << id << " total ERROR probability  " << packetErrorRate
       << " per with H-ARQ error reduction " << effectiveErrorRateWithHarq
       << " - CQI[" << cqi << "]- random error extracted[" << randomSample << "]" << endl;

    // emit SINR statistic
    if (collectSinrStatistics_ && usedRBs > 0)
        emitRcvdSinr(dir, id, lteInfo->getCarrierFrequency(), sumSnr / usedRBs);

    bool receptionFailed = (randomSample <= effectiveErrorRateWithHarq);
    if (receptionFailed) {
        EV << "This is NOT your lucky day (" << randomSample << " < " << effectiveErrorRateWithHarq
           << ") -> do not receive." << endl;

        // Signal too weak, we can't receive it
        return false;
    }
    // Signal is strong enough, receive this Signal
    EV << "This is your lucky day (" << randomSample << " > " << effectiveErrorRateWithHarq
       << ") -> Receive AirFrame." << endl;

    return true;
}

void StochasticChannelModel::emitRcvdSinr(Direction dir, MacNodeId ueId, GHz carrierFrequency, double sinr)
{
    if (dir == DL) { // we are on the UE
        emit(rcvdSinrDlSignal_, sinr);
        return;
    }

    // we are on the BS, so we need to retrieve the channel model of the sender
    // XXX I know, there might be a faster way...
    ChannelModelBase *ueChannelModel = binder_->getPhy(ueId)->getChannelModel(carrierFrequency);
    ueChannelModel->emit(rcvdSinrUlSignal_, sinr);
}

void StochasticChannelModel::computeLosProbability(double d3D, double d2D,
        const LinkKey& nodeId)
{
    if (!dynamicLos_) {
        losMap_[nodeId] = fixedLos_;
        return;
    }
    double p = pathLoss_->computeLosProbability(d3D, d2D);
    losMap_[nodeId] = (uniform(0.0, 1.0) <= p);
}

double StochasticChannelModel::computePathLoss(double distance, double dbp, bool los)
{
    return pathLoss_->computePathLoss(distance, dbp, los);
}

double StochasticChannelModel::getTwoDimDistance(inet::Coord a, inet::Coord b)
{
    a.z = 0.0;
    b.z = 0.0;
    return a.distance(b);
}

bool StochasticChannelModel::computeExtCellInterference(MacNodeId eNbId, MacNodeId nodeId, Coord coord, bool isCqi, GHz carrierFrequency,
        std::vector<double> *interference)
{
    EV << "**** Ext Cell Interference **** " << endl;

    // get external cell list
    ExtCellList list = binder_->getExtCellList(carrierFrequency);

    double dist, // meters
           recvPwr, // watt
           recvPwrDBm, // dBm
           att, // dBm
           angularAtt; // dBm

    //compute distance for each cell
    for (auto& extCell : list) {
        // get external cell position
        Coord c = extCell->getPosition();
        // compute distance between UE and the ext cell
        dist = coord.distance(c);

        EV << "\t distance between UE[" << coord.x << "," << coord.y <<
            "] and extCell[" << c.x << "," << c.y << "] is -> "
           << dist << "\t";

        // compute attenuation according to some path loss model
        att = computeExtCellPathLoss(dist, LinkKey(nodeId));

        //=============== ANGULAR ATTENUATION =================
        if (extCell->getTxDirection() == OMNI) {
            angularAtt = 0;
        }
        else {
            // compute the angle between uePosition and reference axis, considering the eNb as center
            double ueAngle = computeAngle(c, coord);

            // compute the reception angle between ue and eNb
            double recvAngle = fabs(extCell->getTxAngle() - ueAngle);

            if (recvAngle > 180)
                recvAngle = 360 - recvAngle;

            double verticalAngle = computeVerticalAngle(c, coord);

            // compute attenuation due to sectorial tx
            angularAtt = computeAngularAttenuation(recvAngle, verticalAngle);
        }
        //=============== END ANGULAR ATTENUATION =================

        // TODO do we need to use (- cableLoss_ + antennaGainEnB_) in ext cells too?
        // compute and linearize received power
        recvPwrDBm = extCell->getTxPower() - att - angularAtt - cableLoss_ + antennaGainEnB_ + antennaGainUe_;
        recvPwr = dBmToLinear(recvPwrDBm);

        unsigned int numBands = std::min(numBands_, extCell->getNumBands());
        EV << " - shared bands [" << numBands << "]" << endl;

        // add interference in those bands where the ext cell is active
        for (unsigned int i = 0; i < numBands; i++) {
            int occ;
            if (isCqi) { // check slot occupation for this TTI
                occ = extCell->getBandStatus(i);
            }
            else {      // error computation. We need to check the slot occupation of the previous TTI
                occ = extCell->getPrevBandStatus(i);
            }

            // if the ext cell is active, add interference
            if (occ) {
                (*interference)[i] += recvPwr;
            }
        }
    }

    return true;
}

bool StochasticChannelModel::computeBackgroundCellInterference(MacNodeId nodeId, inet::Coord bsCoord, inet::Coord ueCoord, bool isCqi, GHz carrierFrequency, const RbMap& rbmap, Direction dir,
        std::vector<double> *interference)
{
    EV << "**** Background Cell Interference **** " << endl;

    // get bg schedulers list
    const auto& list = binder_->getBackgroundSchedulerList(carrierFrequency);

    Coord c;
    double dist, // meters
           txPwr, // dBm
           recvPwr, // watt
           recvPwrDBm, // dBm
           att, // dBm
           angularAtt; // dBm

    //compute distance for each cell
    for (auto& bgScheduler : list) {
        if (dir == DL) {
            // compute interference with respect to the background base station

            // get external cell position
            c = bgScheduler->getPosition();
            // compute distance between UE and the ext cell
            dist = ueCoord.distance(c);

            EV << "\t distance between UE[" << ueCoord.x << "," << ueCoord.y <<
                "] and backgroundCell[" << c.x << "," << c.y << "] is -> "
               << dist << "\t";

            // compute attenuation according to some path loss model
            att = computeExtCellPathLoss(dist, LinkKey(nodeId));

            txPwr = bgScheduler->getTxPower();

            //=============== ANGULAR ATTENUATION =================
            if (bgScheduler->getTxDirection() == OMNI) {
                angularAtt = 0;
            }
            else {
                // compute the angle between uePosition and reference axis, considering the eNB as center
                double ueAngle = computeAngle(c, ueCoord);

                // compute the reception angle between ue and eNB
                double recvAngle = fabs(bgScheduler->getTxAngle() - ueAngle);

                if (recvAngle > 180)
                    recvAngle = 360 - recvAngle;

                double verticalAngle = computeVerticalAngle(c, ueCoord);

                // compute attenuation due to sectorial tx
                angularAtt = computeAngularAttenuation(recvAngle, verticalAngle);
            }
            //=============== END ANGULAR ATTENUATION =================

            // TODO do we need to use (- cableLoss_ + antennaGainEnB_) in ext cells too?
            // compute and linearize received power
            recvPwrDBm = txPwr - att - angularAtt - cableLoss_ + antennaGainEnB_ + antennaGainUe_;
            recvPwr = dBmToLinear(recvPwrDBm);
            EV << " recvPwr[" << recvPwr << "]\t";

            unsigned int numBands = std::min(numBands_, bgScheduler->getNumBands());
            EV << " - shared bands [" << numBands << "]\t";
            EV << " - interfering bands[";

            // add interference in those bands where the ext cell is active
            for (unsigned int i = 0; i < numBands; i++) {
                int occ = 0;
                if (isCqi) { // check slot occupation for this TTI
                    occ = bgScheduler->getBandStatus(i, DL);
                }
                else if (!rbmap.empty() && rbmap.at(MACRO).at(i) != 0) {     // error computation. We need to check the slot occupation of the previous TTI (only if the band has been used by the UE)
                    occ = bgScheduler->getPrevBandStatus(i, DL);
                }

                // if the ext cell is active, add interference
                if (occ > 0) {
                    EV << i << ",";
                    (*interference)[i] += recvPwr;
                }
            }
            EV << "]" << endl;
        }
        else { // dir == UL
            // for each RB occupied in the background cell, compute interference with respect to the
            // background UE that is using that RB
            TrafficGeneratorBase *bgUe;

            double antennaGainBgUe = antennaGainUe_;  // TODO get this from the bgUe

            angularAtt = 0;  // we assume OMNI directional UEs

            unsigned int numBands = std::min(numBands_, bgScheduler->getNumBands());
            EV << " - shared bands [" << numBands << "]" << endl;

            // add interference in those bands where a UE in the background cell is active
            for (unsigned int i = 0; i < numBands; i++) {
                int occ = 0;

                if (isCqi) { // check slot occupation for this TTI
                    occ = bgScheduler->getBandStatus(i, UL);
                    if (occ)
                        bgUe = bgScheduler->getBandInterferingUe(i);
                }
                else if (rbmap.at(MACRO).at(i) != 0) {     // error computation. We need to check the slot occupation of the previous TTI (only if the band has been used by the UE)
                    occ = bgScheduler->getPrevBandStatus(i, UL);
                    if (occ)
                        bgUe = bgScheduler->getPrevBandInterferingUe(i);
                }

                // if the ext cell is active, add interference
                if (occ) {
                    txPwr = bgUe->getTxPwr();

                    c = bgUe->getCoord();
                    dist = bsCoord.distance(c);

                    EV << "\t distance between BgBS[" << bsCoord.x << "," << bsCoord.y <<
                        "] and backgroundUE[" << c.x << "," << c.y << "] is -> "
                       << dist << "\t";

                    // compute attenuation according to some path loss model
                    att = computeExtCellPathLoss(dist, LinkKey(nodeId));

                    recvPwrDBm = txPwr - att - angularAtt - cableLoss_ + antennaGainEnB_ + antennaGainBgUe;
                    recvPwr = dBmToLinear(recvPwrDBm);

                    (*interference)[i] += recvPwr;
                }
            }
        }
    }

    return true;
}

double StochasticChannelModel::computeExtCellPathLoss(double dist, const LinkKey& nodeId)
{

    //compute attenuation based on selected scenario and based on LOS or NLOS
    bool los = losMap_[nodeId];

    if (!enable_extCell_los_)
        los = false;

    // always the TR 36.814 formulas, whatever study the model itself uses
    double attenuation = extCellPathLoss_->computePathLoss(dist, dist, los);

    return attenuation;
}

StochasticChannelModel *StochasticChannelModel::obtainUeChannelModel(MacNodeId id)
{
    // obtain a reference to the UE's endpoint
    IRadioEndpoint *phy = nullptr;

    for (const auto& ueInfo : binder_->getUeList()) {
        if (ueInfo->id == id) {
            phy = ueInfo->phy;
            break;
        }
    }

    if (phy == nullptr)
        throw cRuntimeError("StochasticChannelModel::obtainUeChannelModel - UE %d is not known to the Binder", (int)num(id));

    StochasticChannelModel *model = dynamic_cast<StochasticChannelModel *>(phy->getChannelModel(carrierFrequency_));
    if (model == nullptr)
        throw cRuntimeError("StochasticChannelModel::obtainUeChannelModel - UE %d has no StochasticChannelModel on carrier %g GHz",
                (int)num(id), carrierFrequency_.get());
    return model;
}

StochasticChannelModel::JakesFadingMap *StochasticChannelModel::obtainUeJakesMap(MacNodeId id)
{
    return obtainUeChannelModel(id)->getJakesMap();
}

StochasticChannelModel::ShadowFadingMap *StochasticChannelModel::obtainShadowingMap(MacNodeId id)
{
    return obtainUeChannelModel(id)->getShadowingMap();
}

bool StochasticChannelModel::computeDownlinkInterference(MacNodeId eNbId, MacNodeId ueId, Coord coord, bool isCqi, GHz carrierFrequency, const RbMap& rbmap,
        std::vector<double> *interference)
{
    EV << "**** Downlink Interference ****" << endl;

    const auto& enbList = binder_->getEnbList();
    for (auto& enbInfo : enbList) {
        MacNodeId id = enbInfo->id;

        if (id == eNbId)
            continue;

        // initialize eNB data structures
        if (!enbInfo->init) {
            // obtain a reference to eNB phy and obtain tx power
            enbInfo->phy = binder_->getPhy(id);

            enbInfo->txPwr = enbInfo->phy->getTxPwr();//dBm

            // get tx direction
            enbInfo->txDirection = enbInfo->phy->getTxDirection();

            // get tx angle
            enbInfo->txAngle = enbInfo->phy->getTxAngle();

            //get reference to mac layer
            enbInfo->mac = check_and_cast<LteMacEnb *>(binder_->getMacByNodeId(id));

            enbInfo->init = true;
        }

        StochasticChannelModel *interfChanModel = dynamic_cast<StochasticChannelModel *>(enbInfo->phy->getChannelModel(carrierFrequency));

        // if the eNB does not use the selected carrier frequency, skip it
        if (interfChanModel == nullptr)
            continue;

        // compute attenuation using data structures within the cell
        double att = interfChanModel->getAttenuation(ueId, UL, coord, isCqi);
        EV << "EnbId [" << id << "] - attenuation [" << att << "]";

        //=============== ANGULAR ATTENUATION =================
        double angularAtt = 0;
        if (enbInfo->txDirection == ANISOTROPIC) {
            //get tx angle
            double txAngle = enbInfo->txAngle;

            // compute the angle between uePosition and reference axis, considering the eNB as center
            double ueAngle = computeAngle(enbInfo->phy->getCoord(), coord);

            // compute the reception angle between ue and eNB
            double recvAngle = fabs(txAngle - ueAngle);
            if (recvAngle > 180)
                recvAngle = 360 - recvAngle;

            double verticalAngle = computeVerticalAngle(enbInfo->phy->getCoord(), coord);

            // compute attenuation due to sectorial tx
            angularAtt = computeAngularAttenuation(recvAngle, verticalAngle);

            EV << "angular attenuation [" << angularAtt << "]";
        }
        // else, antenna is omni-directional
        //=============== END ANGULAR ATTENUATION =================

        double txPwr = enbInfo->txPwr - angularAtt - cableLoss_ + antennaGainEnB_ + antennaGainUe_;

        unsigned int numBands = std::min(numBands_, interfChanModel->getNumBands());
        EV << " - shared bands [" << numBands << "]" << endl;

        if (isCqi) {// check slot occupation for this TTI
            for (unsigned int i = 0; i < numBands; i++) {
                // compute the number of occupied slot (unnecessary)
                int temp = enbInfo->mac->getDlBandStatus(i);
                if (temp != 0)
                    (*interference)[i] += dBmToLinear(txPwr - att); //(dBm-dB)=dBm

                EV << "\t band " << i << " occupied " << temp << "/pwr[" << txPwr << "]-int[" << (*interference)[i] << "]" << endl;
            }
        }
        else { // error computation. We need to check the slot occupation of the previous TTI
            for (unsigned int i = 0; i < numBands; i++) {
                // if we are decoding a data transmission and this RB has not been used, skip it
                // TODO fix for multi-antenna case
                if (!rbmap.empty() && rbmap.at(MACRO).at(i) == 0)
                    continue;

                // compute the number of occupied slot (unnecessary)
                int temp = enbInfo->mac->getDlPrevBandStatus(i);
                if (temp != 0)
                    (*interference)[i] += dBmToLinear(txPwr - att); //(dBm-dB)=dBm

                EV << "\t band " << i << " occupied " << temp << "/pwr[" << txPwr << "]-int[" << (*interference)[i] << "]" << endl;
            }
        }
    }

    return true;
}

StochasticChannelModel::InterfererInfo StochasticChannelModel::describeInterferer(const UeAllocationInfo& allocation)
{
    InterfererInfo info;
    info.nodeId = allocation.nodeId;
    info.cellId = allocation.cellId;
    info.dir = allocation.dir;

    if (allocation.phy != nullptr) {
        PhyUe *uePhy = check_and_cast<PhyUe *>(allocation.phy);
        info.txPwr = uePhy->getTxPwr(info.dir);
        info.coord = uePhy->getCoord();
    }
    else { // this is a backgroundUe
        TrafficGeneratorBase *trafficGen = check_and_cast<TrafficGeneratorBase *>(allocation.trafficGen);
        info.txPwr = trafficGen->getTxPwr();
        info.coord = trafficGen->getCoord();
    }
    return info;
}

bool StochasticChannelModel::computeUplinkInterference(MacNodeId eNbId, MacNodeId senderId, bool isCqi, GHz carrierFrequency, const RbMap& rbmap, std::vector<double> *interference)
{
    EV << "**** Uplink Interference for cellId[" << eNbId << "] node[" << senderId << "] ****" << endl;

    const std::vector<std::vector<UeAllocationInfo>> *ulTransmissionMap;
    const std::vector<UeAllocationInfo> *allocatedUes;

    if (isCqi) {// check slot occupation for this TTI
        ulTransmissionMap = binder_->getUlTransmissionMap(carrierFrequency, CURR_TTI);
        if (ulTransmissionMap != nullptr && !ulTransmissionMap->empty()) {
            for (unsigned int i = 0; i < numBands_; i++) {
                // get the set of UEs transmitting on the same band
                allocatedUes = &(ulTransmissionMap->at(i));

                for (auto& ue_it : *allocatedUes) {
                    const InterfererInfo interferer = describeInterferer(ue_it);
                    const MacNodeId ueId = interferer.nodeId;
                    const MacCellId cellId = interferer.cellId;
                    const Direction dir = interferer.dir;
                    const double txPwr = interferer.txPwr;
                    const inet::Coord ueCoord = interferer.coord;

                    // no self-interference
                    if (ueId == senderId)
                        continue;

                    // no interference from UL/D2D connections of the same cell  (no D2D-UL reuse allowed)
                    if (cellId == eNbId)
                        continue;

                    EV << NOW << " StochasticChannelModel::computeUplinkInterference - Interference from UE: " << ueId << "(dir " << dirToA(dir) << ") on band[" << i << "]" << endl;

                    // get rx power and attenuation from this UE
                    double rxPwr = txPwr - cableLoss_ + antennaGainUe_ + antennaGainEnB_;
                    double att = getAttenuation(ueId, UL, ueCoord, false);
                    (*interference)[i] += dBmToLinear(rxPwr - att);//(dBm-dB)=dBm

                    EV << "\t band " << i << "/pwr[" << rxPwr - att << "]-int[" << (*interference)[i] << "]" << endl;
                }
            }
        }
    }
    else { // Error computation. We need to check the slot occupation of the previous TTI
        ulTransmissionMap = binder_->getUlTransmissionMap(carrierFrequency, PREV_TTI);
        if (ulTransmissionMap != nullptr && !ulTransmissionMap->empty()) {
            // For each band we have to check if the Band in the previous TTI was occupied by the interferingId
            for (unsigned int i = 0; i < numBands_; i++) {
                // if we are decoding a data transmission and this RB has not been used, skip it
                // TODO fix for multi-antenna case
                if (!rbmap.empty() && rbmap.at(MACRO).at(i) == 0)
                    continue;

                // get the set of UEs transmitting on the same band
                allocatedUes = &(ulTransmissionMap->at(i));

                for (auto& ue_it : *allocatedUes) {
                    const InterfererInfo interferer = describeInterferer(ue_it);
                    const MacNodeId ueId = interferer.nodeId;
                    const MacCellId cellId = interferer.cellId;
                    const Direction dir = interferer.dir;
                    const double txPwr = interferer.txPwr;
                    const inet::Coord ueCoord = interferer.coord;

                    // no self-interference
                    if (ueId == senderId)
                        continue;

                    // no interference from UL connections of the same cell (no D2D-UL reuse allowed)
                    if (cellId == eNbId)
                        continue;

                    EV << NOW << " StochasticChannelModel::computeUplinkInterference - Interference from UE: " << ueId << "(dir " << dirToA(dir) << ") on band[" << i << "]" << endl;

                    // get tx power and attenuation from this UE
                    double rxPwr = txPwr - cableLoss_ + antennaGainUe_ + antennaGainEnB_;
                    double att = getAttenuation(ueId, UL, ueCoord, false);
                    (*interference)[i] += dBmToLinear(rxPwr - att);//(dBm-dB)=dBm

                    EV << "\t band " << i << "/pwr[" << rxPwr - att << "]-int[" << (*interference)[i] << "]" << endl;
                }
            }
        }
    }

    // Debug Output
    EV << NOW << " StochasticChannelModel::computeUplinkInterference - Final Band Interference Status: " << endl;
    for (unsigned int i = 0; i < numBands_; i++)
        EV << "\t band " << i << " int[" << (*interference)[i] << "]" << endl;

    return true;
}

} //namespace
