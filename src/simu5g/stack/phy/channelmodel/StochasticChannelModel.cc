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
#include "simu5g/stack/phy/radio/BlerCurveErrorModel.h"
#include "simu5g/stack/phy/radio/CellularReceiver.h"

namespace simu5g {

using namespace inet;
using namespace omnetpp;
Define_Module(StochasticChannelModel);

StochasticChannelModel::~StochasticChannelModel()
{
    delete pathLoss_;
    delete extCellPathLoss_;
    // radioMedium_ is unset if the medium has already been deleted
    if (channelState_ != nullptr && radioMedium_)
        radioMedium_->removeChannelState(this);
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

ChannelState& StochasticChannelModel::channelState()
{
    if (channelState_ == nullptr)
        channelState_ = &radioMedium_->getChannelState(this);
    return *channelState_;
}

const ChannelState& StochasticChannelModel::channelState() const
{
    // finding the medium's record on first use does not change the state
    return const_cast<StochasticChannelModel *>(this)->channelState();
}

StochasticChannelModel::LosMap& StochasticChannelModel::losMap()
{
    if (losMap_ == nullptr)
        losMap_ = &radioMedium_->getLosMap(carrierFrequency_);
    return *losMap_;
}

StochasticChannelModel::ShadowFadingMap& StochasticChannelModel::shadowingMap()
{
    if (shadowingMap_ == nullptr)
        shadowingMap_ = &radioMedium_->getShadowingMap(carrierFrequency_);
    return *shadowingMap_;
}

StochasticChannelModel::JakesFadingMap& StochasticChannelModel::jakesFadingMap()
{
    if (jakesFadingMap_ == nullptr)
        jakesFadingMap_ = &radioMedium_->getJakesFadingMap(carrierFrequency_);
    return *jakesFadingMap_;
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
        if (inside_building_) {
            // The distance from the building wall is the UE's, not a carrier's:
            // the first channel model of the radio's carriers draws it, and the
            // others take it (they initialize after it, in vector order).
            auto *first = isVector() && getIndex() > 0 ? dynamic_cast<StochasticChannelModel *>(getParentModule()->getSubmodule(getName(), 0)) : nullptr;
            if (first != nullptr && first->inside_building_)
                inside_distance_ = first->inside_distance_;
            else
                inside_distance_ = uniform(0.0, 25.0);
        }
        tolerateMaxDistViolation_ = par("tolerateMaxDistViolation");
        hUe_ = par("ueHeight");

        wStreet_ = par("streetWidth");

        correlationDistance_ = par("correlationDistance");

        antennaGainUe_ = par("antennaGainUe");
        antennaGainEnB_ = par("antennGainEnB");
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

RadioLink StochasticChannelModel::cellularLink(MacNodeId ueId, Direction dir, Coord coord)
{
    // The local module is one endpoint and 'coord' the other; 'dir' says which
    // of the two is the UE. The UE is the node whose position history we track.
    RadioLink link;
    link.dir = dir;
    link.stateNodeId = ueId;

    if (dir == DL) { // the local module is the UE, 'coord' is the BS
        link.txIsBaseStation = true;
        link.txCoord = coord;
        link.rxCoord = phy_->getCoord();
        link.stateCoord = phy_->getCoord();
        link.rxId = ueId;
        link.rxRadio = phy_;
    }
    else { // the local module is the BS, 'coord' is the UE
        link.txIsBaseStation = false;
        link.txCoord = coord;
        link.rxCoord = phy_->getCoord();
        link.stateCoord = coord;
        link.txId = ueId;
        link.rxId = phy_->getMacNodeId();
        link.txRadio = radioMedium_->findRadio(ueId);
        link.rxRadio = phy_;
    }
    // The link is the UE and the local radio. For DL the local radio is the UE
    // itself, and the base station at 'coord' is not named.
    link.linkKey = LinkKey(link.txId, link.rxId);
    return link;
}

const Coord& StochasticChannelModel::receptionPosition(const AirFrame *frame) const
{
    const Coord& arrivalPosition = frame->getArrivalPosition();
    return arrivalPosition.isUnspecified() ? phy_->getCoord() : arrivalPosition;
}

RadioLink StochasticChannelModel::linkFor(UserControlInfo *lteInfo, const Coord& localPosition)
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
        ueCoord = localPosition;
        enbCoord = coord;
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
        enbCoord = localPosition;
    }

    if (link.dir == DL) {
        link.txIsBaseStation = true;
        link.txId = eNbId;
        link.rxId = ueId;
        link.txCoord = enbCoord;
        link.rxCoord = ueCoord;
    }
    else { // if( dir == UL )
        link.txIsBaseStation = false;
        link.txId = ueId;
        link.rxId = eNbId;
        link.txCoord = ueCoord;
        link.rxCoord = enbCoord;
    }

    // The two radios: the local one is this model's own, the other the frame's
    // source -- the base station of a DL frame or a beacon, the UE of a UL frame
    // or a CQI report. The local end is named by the local radio, not by the
    // frame's destination: a beacon and a cell-selection probe carry none.
    MacNodeId localId = phy_->getMacNodeId();
    if (link.dir == DL && lteInfo->getFrameType() != FEEDBACKPKT) { // decoding at the UE
        link.rxId = localId;
        link.txRadio = radioMedium_->findRadio(eNbId);
        link.rxRadio = phy_;
    }
    else if (link.dir == DL) { // a DL CQI, at the base station
        link.txId = localId;
        link.txRadio = phy_;
        link.rxRadio = radioMedium_->findRadio(ueId);
    }
    else { // UL data or a UL CQI, at the base station
        link.rxId = localId;
        link.txRadio = radioMedium_->findRadio(ueId);
        link.rxRadio = phy_;
    }
    link.linkKey = LinkKey(link.txId, link.rxId);

    // It is always the UE's position that feeds the speed computation -- which is
    // why the old code's "pass UL for a FEEDBACKPKT" special case is not needed
    // here: it only existed to make getAttenuation() pick 'coord' rather than
    // phy_->getCoord().
    link.stateNodeId = ueId;
    link.stateCoord = ueCoord;

    // the cell this link belongs to, for the interference computation
    link.cellId = eNbId;

    return link;
}

double StochasticChannelModel::getAttenuation(const RadioLink& link)
{
    // a building's penetration loss is the UE end's: the transmitter of an
    // uplink, otherwise the receiver (the UE, or a D2D link's receiver); a
    // background UE is outdoors
    IRadioEndpoint *ueEnd = link.dir == UL ? link.txRadio : link.rxRadio;
    pathLoss_->setIndoor(ueEnd != nullptr && ueEnd->isInsideBuilding(), ueEnd != nullptr ? ueEnd->getInsideDistance() : 0.0);

    // COMPUTE 3D and 2D DISTANCE between the two endpoints
    double threeDimDistance = link.txCoord.distance(link.rxCoord);
    double twoDimDistance = getTwoDimDistance(link.txCoord, link.rxCoord);

    // The link's LOS state is decided on its first evaluation, and decided again
    // when either of its ends is farther than the correlation distance from
    // where it was then: the UE could have changed its visibility from the
    // other end
    auto it = losMap().find(link.linkKey);
    if (it == losMap().end()
        || link.displacementSince(it->second.positionA, it->second.positionB) > correlationDistance_)
    {
        computeLosProbability(threeDimDistance, twoDimDistance, link);
    }

    //compute attenuation based on selected scenario and based on LOS or NLOS
    bool los = losMap()[link.linkKey].los;
    double attenuation = computePathLoss(threeDimDistance, twoDimDistance, los);

    //    Applying shadowing only if it is enabled by configuration
    //    log-normal shadowing (not available for background UEs)
    if (num(link.stateNodeId) < BGUE_MIN_ID && shadowing_)
        attenuation += computeShadowing(threeDimDistance, twoDimDistance, los, link);

    // update the tracked node's current position
    updatePositionHistory(link.stateNodeId, link.stateCoord);

    EV << "StochasticChannelModel::getAttenuation - computed attenuation at distance " << threeDimDistance << " for eNB is " << attenuation << endl;

    return attenuation;
}

double StochasticChannelModel::computeShadowing(double d3D, double d2D, bool los, const RadioLink& link)
{
    ASSERT(link.linkKey == LinkKey(link.txId, link.rxId));

    // one realization per link, whichever end and whichever direction asks
    ShadowFadingMap *actualShadowingMap = &shadowingMap();
    const LinkKey& key = link.linkKey;

    // where the link's two radios are, in the order of its key
    const Coord& positionA = link.positionA();
    const Coord& positionB = link.positionB();

    double mean = 0;

    // Get std deviation according to LOS/NLOS and selected scenario
    double stdDev = pathLoss_->getShadowingStdDev(d3D, d2D, los);
    double att;

    auto it = actualShadowingMap->find(key);
    // if shadowing for this link has never been computed
    if (it == actualShadowingMap->end()) {
        //Get the log-normal shadowing with std deviation stdDev
        att = normal(mean, stdDev);

        //store the shadowing attenuation for this link, with when and where it was drawn
        (*actualShadowingMap)[key] = ShadowingSample{NOW, positionA, positionB, att};
    }
    else {
        // how far the link has moved since the sample was drawn: the farther of
        // its two ends (a base station stays put, so for a cellular link it is
        // how far the UE has moved)
        double space = link.displacementSince(it->second.positionA, it->second.positionB);

        // if either end has moved more than the correlation distance
        if (space > correlationDistance_) {
            //Compute shadowing with an EAW (Exponential Average Window) (step 1)
            double a = exp(-0.5 * (space / correlationDistance_));

            //Get last shadowing attenuation computed
            double old = it->second.value;

            //Compute shadowing with an EAW (Exponential Average Window) (step 2)
            att = a * old + sqrt(1 - pow(a, 2)) * normal(mean, stdDev);

            // Store the new computed shadowing
            it->second = ShadowingSample{NOW, positionA, positionB, att};
        }
        // otherwise the shadowing attenuation remains the same
        else
            att = it->second.value;
    }

    return att;
}

void StochasticChannelModel::updatePositionHistory(const MacNodeId nodeId,
        const Coord coord)
{
    auto& positionHistory = channelState().positionHistory;

    if (positionHistory.find(nodeId) != positionHistory.end()) {
        // position already updated for this TTI.
        if (positionHistory[nodeId].back().first == NOW)
            return;
    }

    // FIXME: possible memory leak
    positionHistory[nodeId].push(Position(NOW, coord));

    if (positionHistory[nodeId].size() > 2) // if we have more than a past and a current element
        // drop the oldest one
        positionHistory[nodeId].pop();
}

double StochasticChannelModel::computeSpeed(const MacNodeId nodeId,
        const Coord coord)
{
    double speed = 0.0;
    auto& positionHistory = channelState().positionHistory;

    if (positionHistory.find(nodeId) == positionHistory.end()) {
        // no entries
        return speed;
    }
    else {
        //compute distance traveled from last update by UE (eNodeB position is fixed)

        if (positionHistory[nodeId].size() == 1) {
            //  the only element refers to the present, return 0
            return speed;
        }

        double movement = positionHistory[nodeId].front().second.distance(coord);

        if (movement <= 0.0)
            return speed;
        else {
            double time = (NOW.dbl()) - (positionHistory[nodeId].front().first.dbl());
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
    else if (relx >= 0 && rely < 0) // quadrant IV, and the negative y axis
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
    double elevation = acos(twoDimDistance / threeDimDistance) * 180.0 / M_PI;
    // angle from the zenith: above the horizon if the point is higher than the centre
    return (point.z > center.z) ? 90 - elevation : 90 + elevation;
}

double StochasticChannelModel::computeAngularAttenuation(double hAngle, double vAngle) {
    return pathLoss_->computeAngularAttenuation(hAngle, vAngle);
}

std::vector<double> StochasticChannelModel::getSINR(AirFrame *frame, UserControlInfo *lteInfo)
{
    RadioLink link = linkFor(lteInfo, receptionPosition(frame));

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
    if (link.rxRadio == nullptr)
        throw cRuntimeError("StochasticChannelModel::getSINR(): the receiver of the link to node %d is not a radio on the medium", (int)num(link.rxId));
    double totN = dBmToLinear(thermalNoise_ + link.rxRadio->getNoiseFigure());

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
    // types must not be reported as one of them. The UE's receiver records it in
    // both directions (for UL we are on the BS).
    if (collectSinrStatistics_ && (lteInfo->getFrameType() == FEEDBACKPKT) && usedRBs > 0
        && (link.dir == DL || link.dir == UL))
        radioMedium_->getRadio(ueId)->getReceiver()->emitMeasuredSinr(link.dir, lteInfo->getCarrierFrequency(), sumSnr / usedRBs);

    // if sender is an eNodeB
    if (link.dir == DL)
        // store the position of user
        updatePositionHistory(link.stateNodeId, phy_->getCoord());
    // sender is a UE
    else
        updatePositionHistory(link.stateNodeId, lteInfo->getCoord());
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
        computeBackgroundCellInterference(link.linkKey, enbCoord, ueCoord, (lteInfo->getFrameType() == FEEDBACKPKT), lteInfo->getCarrierFrequency(), rbmap, dir, &bgCellInterference); // dBm
    }

    //============ EXTCELL INTERFERENCE COMPUTATION =================
    // TODO this might be obsolete as it is replaced by background cell interference
    // vector containing the sum of external cell interference for each band
    std::vector<double> extCellInterference; // Linear value (mW)
    // prepare data structure
    extCellInterference.resize(numBands_, 0);
    if (enableExtCellInterference_ && dir == DL) {
        computeExtCellInterference(eNbId, link.linkKey, ueCoord, (lteInfo->getFrameType() == FEEDBACKPKT), lteInfo->getCarrierFrequency(), &extCellInterference); // dBm
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
    return getRSRP(linkFor(lteInfo, receptionPosition(frame)), lteInfo->getTxPower());
}

std::vector<double> StochasticChannelModel::getRSRP(const RadioLink& link, double txPower)
{
    double recvPower = txPower; // dBm

    EV << "StochasticChannelModel::getRSRP - txId=" << link.txId
       << " - rxId=" << link.rxId
       << " - DIR=" << dirToA(link.dir)
       << " - txPwr " << txPower
       << " - txCoord[" << link.txCoord << "] - rxCoord[" << link.rxCoord << "]" << endl;

    // the link budget of the two radios: their antenna gains, the receiver's cable loss
    IRadioEndpoint *tx = link.txRadio;
    IRadioEndpoint *rx = link.rxRadio;
    if (tx == nullptr || rx == nullptr)
        throw cRuntimeError("StochasticChannelModel::getRSRP(): the link from node %d to node %d has an end that is not a radio on the medium",
                (int)num(link.txId), (int)num(link.rxId));
    double txAntennaGain = tx->getAntennaGain();
    double rxAntennaGain = rx->getAntennaGain();
    double cableLoss = rx->getCableLoss();

    // =============== PATH LOSS + SHADOWING + FADING =================
    EV << "\t using parameters - antennaGainTx=" << txAntennaGain << " - antennaGainRx=" << rxAntennaGain
       << " - txPwr=" << txPower << " - for link=" << link.linkKey << endl;

    // Speed must be read BEFORE getAttenuation(), which appends to the position
    // history: computeSpeed() derives from that history, so evaluating it
    // afterwards would yield a different value and hence different fading.
    // Load-bearing ordering.
    double speed = computeSpeed(link.stateNodeId, link.stateCoord);

    // attenuation for the desired signal
    double attenuation = getAttenuation(link); // dB

    // compute attenuation (PATHLOSS + SHADOWING)
    recvPower -= attenuation; // (dBm-dB)=dBm

    // add antenna gain
    recvPower += txAntennaGain; // (dBm+dB)=dBm
    recvPower += rxAntennaGain; // (dBm+dB)=dBm

    // sub cable loss
    recvPower -= cableLoss; // (dBm-dB)=dBm

    // =============== ANGULAR ATTENUATION =================
    // Only a base station has a sectorial antenna; a UE-to-UE link never gets here.
    if (link.txIsBaseStation) {
        if (tx->getTxDirection() == ANISOTROPIC) {
            // get tx angle
            double txAngle = tx->getTxAngle();

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
                fadingAttenuation = jakesFading(jakesFadingMap(), link.linkKey, speed, i);
        }
        // add fading contribution to the received power
        double finalRecvPower = recvPower + fadingAttenuation; // (dBm+dB)=dBm

        EV << " StochasticChannelModel::getRSRP link " << link.linkKey
           << " band " << i << " recvPower " << recvPower
           << " direction " << dirToA(link.dir) << " antenna gain tx "
           << txAntennaGain << " antenna gain rx " << rxAntennaGain
           << " cable loss   " << cableLoss
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
    }
    else { // if( dir == UL )
        // TODO check if antennaGainEnB should be added in UL direction too
        antennaGainTx = antennaGainUe_;
        antennaGainRx = antennaGainEnB_;
        noiseFigure = bsNoiseFigure_;
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
    RadioLink link = cellularLink(bgUeId, UL, ueCoord);
    double attenuation = getAttenuation(link);

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
        IRadioEndpoint *bsEndpoint = radioMedium_->findRadio(eNbId);

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
                fadingAttenuation = jakesFading(jakesFadingMap(), link.linkKey, speed, i);
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
        computeBackgroundCellInterference(link.linkKey, enbCoord, ueCoord, isCqi, lteInfo->getCarrierFrequency(), rbmap, dir, &bgCellInterference); // dBm
    }

    //============ EXTCELL INTERFERENCE COMPUTATION =================
    // TODO this might be obsolete as it is replaced by background cell interference
    //vector containing the sum of ext-cell interference for each band
    std::vector<double> extCellInterference; // Linear value (mW)
    // prepare data structure
    extCellInterference.resize(numBands_, 0);
    if (enableExtCellInterference_ && dir == DL) {
        computeExtCellInterference(eNbId, link.linkKey, ueCoord, isCqi, lteInfo->getCarrierFrequency(), &extCellInterference); // dBm
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

double StochasticChannelModel::rayleighFading(MacNodeId id,
        unsigned int band)
{
    // get rayleigh variable from trace file
    const int channelndex = 0;
    double temp1 = binder_->phyPisaData.getChannel(channelndex + band);
    return linearToDb(temp1);
}

double StochasticChannelModel::jakesFading(JakesFadingMap& jakesMap, const LinkKey& key, double speed,
        unsigned int band)
{
    JakesFadingMap *actualJakesMap = &jakesMap;

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
    if (cqi > 15)
        throw cRuntimeError("A packet has been transmitted with a cqi greater than 15 cqi:%d txmode:%d dir:%d cw:%d rtx:%d",
                cqi, lteInfo->getTxMode(), lteInfo->getDirection(), cw, lteInfo->getTxNumber());

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

    // the receiving node's receiver decides, against its error model
    CellularReceiver *receiver = phy_->getReceiver();
    std::optional<double> packetErrorRate = receiver->getErrorModel()->computePacketErrorRate(cqi, snrV, rbmap, transmissionAttempt);
    if (!packetErrorRate)
        return false; // lost for certain, no decision to draw

    // emit SINR statistic: the mean over the allocated bands
    if (collectSinrStatistics_) {
        double sumSnr = 0.0;
        int usedRBs = 0;
        for (const auto& [remoteUnit, rbList] : rbmap) {
            for (const auto& [band, allocation] : rbList) {
                if (allocation == 0)
                    continue;
                sumSnr += snrV[band];
                usedRBs++;
            }
        }
        if (usedRBs > 0)
            emitRcvdSinr(dir, id, lteInfo->getCarrierFrequency(), sumSnr / usedRBs);
    }

    return receiver->decide(*packetErrorRate);
}

double StochasticChannelModel::antennaGainOf(MacNodeId nodeId, double gainIfNoRadio) const
{
    IRadioEndpoint *radio = radioMedium_->findRadio(nodeId);
    return radio != nullptr ? radio->getAntennaGain() : gainIfNoRadio;
}

double StochasticChannelModel::cableLossOf(MacNodeId nodeId) const
{
    IRadioEndpoint *radio = radioMedium_->findRadio(nodeId);
    return radio != nullptr ? radio->getCableLoss() : cableLoss_;
}

void StochasticChannelModel::emitRcvdSinr(Direction dir, MacNodeId ueId, GHz carrierFrequency, double sinr)
{
    if (dir == DL) { // we are on the UE
        phy_->getReceiver()->emitRcvdSinr(DL, carrierFrequency, sinr);
        return;
    }

    // we are on the BS: the statistic is the sending UE's
    radioMedium_->getRadio(ueId)->getReceiver()->emitRcvdSinr(UL, carrierFrequency, sinr);
}

void StochasticChannelModel::computeLosProbability(double d3D, double d2D,
        const RadioLink& link)
{
    ChannelState::LosSample& sample = losMap()[link.linkKey];
    sample.positionA = link.positionA();
    sample.positionB = link.positionB();
    if (!dynamicLos_) {
        sample.los = fixedLos_;
        return;
    }
    double p = pathLoss_->computeLosProbability(d3D, d2D);
    sample.los = (uniform(0.0, 1.0) <= p);
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

bool StochasticChannelModel::computeExtCellInterference(MacNodeId eNbId, const LinkKey& link, Coord coord, bool isCqi, GHz carrierFrequency,
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
        att = computeExtCellPathLoss(dist, link);

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
            else {      // error computation: the cell's transmission during the slot just completed
                occ = radioMedium_->isBandOccupied(carrierFrequency, extCell->getNodeId(), DL, i, NOW - slotDuration(carrierFrequency), NOW);
                // where the cell's ticks are the carrier's slots, that is its band status of the previous TTI
                ASSERT(slotDuration(carrierFrequency) != TTI || occ == (extCell->getPrevBandStatus(i) != 0));
            }

            // if the ext cell is active, add interference
            if (occ) {
                (*interference)[i] += recvPwr;
            }
        }
    }

    return true;
}

bool StochasticChannelModel::computeBackgroundCellInterference(const LinkKey& link, inet::Coord bsCoord, inet::Coord ueCoord, bool isCqi, GHz carrierFrequency, const RbMap& rbmap, Direction dir,
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
            att = computeExtCellPathLoss(dist, link);

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
                else if (!rbmap.empty() && rbmap.at(MACRO).at(i) != 0) {     // error computation: the cell's transmission during the slot just completed (only if the band has been used by the UE)
                    occ = radioMedium_->isBandOccupied(carrierFrequency, bgScheduler->getNodeId(), DL, i, NOW - slotDuration(carrierFrequency), NOW);
                    // where the cell's slots are the carrier's, that is its band status of the previous slot
                    ASSERT(slotDuration(carrierFrequency) != bgScheduler->getTtiPeriod() || occ == (bgScheduler->getPrevBandStatus(i, DL) != 0));
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

            // error computation: the cell's UEs' transmissions during the slot just completed, by band
            std::vector<const CellularTransmission *> ueTransmissions(numBands, nullptr);
            if (!isCqi) {
                for (auto transmission : radioMedium_->getUplinkTransmissionsDuring(carrierFrequency, NOW - slotDuration(carrierFrequency), NOW)) {
                    if (transmission->cellId != bgScheduler->getNodeId())
                        continue;
                    for (unsigned int i : sharedBands(*transmission, numBands, RbMap()))
                        ueTransmissions[i] = transmission;
                }
            }

            // add interference in those bands where a UE in the background cell is active
            for (unsigned int i = 0; i < numBands; i++) {
                int occ = 0;

                if (isCqi) { // check slot occupation for this TTI
                    occ = bgScheduler->getBandStatus(i, UL);
                    if (occ)
                        bgUe = bgScheduler->getBandInterferingUe(i);
                }
                else if (rbmap.at(MACRO).at(i) != 0) {     // error computation (only if the band has been used by the UE)
                    occ = ueTransmissions[i] != nullptr;
                    if (occ)
                        bgUe = ueTransmissions[i]->backgroundUe;
                    // where the cell's slots are the carrier's, that is its allocation of the previous slot
                    ASSERT(slotDuration(carrierFrequency) != bgScheduler->getTtiPeriod()
                            || (occ == (bgScheduler->getPrevBandStatus(i, UL) != 0) && (!occ || bgUe == bgScheduler->getPrevBandInterferingUe(i))));
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
                    att = computeExtCellPathLoss(dist, link);

                    recvPwrDBm = txPwr - att - angularAtt - cableLoss_ + antennaGainEnB_ + antennaGainBgUe;
                    recvPwr = dBmToLinear(recvPwrDBm);

                    (*interference)[i] += recvPwr;
                }
            }
        }
    }

    return true;
}

double StochasticChannelModel::computeExtCellPathLoss(double dist, const LinkKey& key)
{

    //compute attenuation based on selected scenario and based on LOS or NLOS
    bool los = losMap()[key].los;

    if (!enable_extCell_los_)
        los = false;

    // always the TR 36.814 formulas, whatever study the model itself uses
    double attenuation = extCellPathLoss_->computePathLoss(dist, dist, los);

    return attenuation;
}

bool StochasticChannelModel::computeDownlinkInterference(MacNodeId eNbId, MacNodeId ueId, Coord coord, bool isCqi, GHz carrierFrequency, const RbMap& rbmap,
        std::vector<double> *interference)
{
    EV << "**** Downlink Interference ****" << endl;

    // the power of an interfering cell's transmission at the UE before the path loss: its transmit
    // power less its antenna's attenuation towards the UE, with the gains and losses of both ends
    auto powerTowardsUe = [&] (MacNodeId id, const CellularTransmission& transmission) {
        const Coord& cellPosition = transmission.startPosition;
        //=============== ANGULAR ATTENUATION =================
        double angularAtt = 0;
        if (transmission.txDirection == ANISOTROPIC) {
            //get tx angle
            double txAngle = transmission.txAngle;

            // compute the angle between uePosition and reference axis, considering the eNB as center
            double ueAngle = computeAngle(cellPosition, coord);

            // compute the reception angle between ue and eNB
            double recvAngle = fabs(txAngle - ueAngle);
            if (recvAngle > 180)
                recvAngle = 360 - recvAngle;

            double verticalAngle = computeVerticalAngle(cellPosition, coord);

            // compute attenuation due to sectorial tx
            angularAtt = computeAngularAttenuation(recvAngle, verticalAngle);

            EV << "EnbId [" << id << "] - angular attenuation [" << angularAtt << "]" << endl;
        }
        // else, antenna is omni-directional
        //=============== END ANGULAR ATTENUATION =================

        return transmission.txPower - angularAtt - cableLossOf(ueId) + antennaGainOf(id, antennaGainEnB_) + antennaGainOf(ueId, antennaGainUe_);
    };

    // a reception: the other cells' DL data transmissions of the slot just completed, on its bands;
    // a CQI: those of the slot completed last, on every band
    static const RbMap everyBand;
    simtime_t slotEnd = isCqi ? lastCompletedSlotEnd(carrierFrequency) : NOW;
    simtime_t slotStart = slotEnd - slotDuration(carrierFrequency);
    const RbMap& receptionBands = isCqi ? everyBand : rbmap;
    // in creation order
    for (auto transmission : radioMedium_->getDataTransmissionsDuring(carrierFrequency, slotStart, slotEnd)) {
        MacNodeId id = transmission->sourceId;
        if (transmission->direction != DL || id == eNbId)
            continue;
        // external and background cells interfere through computeExtCellInterference() and computeBackgroundCellInterference()
        if (transmission->phantomCell != nullptr)
            continue;

        // the cell's radio, which also sends its background UEs' allocations
        IRadioEndpoint *cell = radioMedium_->getRadio(id);
        ASSERT(transmission->txPower == cell->getTxPwr() && transmission->txDirection == cell->getTxDirection() && transmission->txAngle == cell->getTxAngle());
        StochasticChannelModel *interfChanModel = dynamic_cast<StochasticChannelModel *>(cell->getChannelModel(carrierFrequency));
        if (interfChanModel == nullptr)
            continue;

        // a transmission on none of the reception's bands does not interfere with it: its link is not evaluated
        unsigned int numBands = std::min(numBands_, interfChanModel->getNumBands());
        auto bands = sharedBands(*transmission, numBands, receptionBands);
        if (bands.empty())
            continue;

        double txPwr = powerTowardsUe(id, *transmission);

        // compute attenuation using data structures within the cell
        double att = interfChanModel->getAttenuation(ueId, UL, coord);
        EV << "EnbId [" << id << "] - attenuation [" << att << "]" << endl;

        for (unsigned int i : bands) {
            (*interference)[i] += dBmToLinear(txPwr - att); //(dBm-dB)=dBm

            EV << "\t band " << i << " occupied/pwr[" << txPwr << "]-int[" << (*interference)[i] << "]" << endl;
        }
    }

    // for a cell with one carrier, the bands of a reception its transmissions occupied are what its scheduler allocated
    if (!isCqi)
        for (auto enbInfo : binder_->getEnbList())
            ASSERT(dlOccupancyMatchesScheduler(enbInfo->id, eNbId, carrierFrequency, slotStart, rbmap));

    return true;
}

bool StochasticChannelModel::dlOccupancyMatchesScheduler(MacNodeId id, MacNodeId eNbId, GHz carrierFrequency, simtime_t slotStart, const RbMap& rbmap)
{
    if (id == eNbId)
        return true;
    auto interfChanModel = dynamic_cast<StochasticChannelModel *>(radioMedium_->getRadio(id)->getChannelModel(carrierFrequency));
    auto mac = check_and_cast<LteMacEnb *>(binder_->getMacByNodeId(id));
    if (interfChanModel == nullptr || mac->getCellInfo()->getCarriers().size() != 1)
        return true;
    unsigned int numBands = std::min(numBands_, interfChanModel->getNumBands());
    for (unsigned int i = 0; i < numBands; i++) {
        if (!rbmap.empty() && rbmap.at(MACRO).at(i) == 0)
            continue;
        if (radioMedium_->isBandOccupied(carrierFrequency, id, DL, i, slotStart, NOW) != (mac->getDlPrevBandStatus(i) != 0))
            return false;
    }
    return true;
}

StochasticChannelModel::InterfererInfo StochasticChannelModel::describeInterferer(const CellularTransmission& transmission)
{
    InterfererInfo info;
    info.nodeId = transmission.sourceId;
    info.cellId = transmission.cellId;
    info.dir = transmission.direction;
    info.txPwr = transmission.txPower;
    info.coord = transmission.startPosition;
    return info;
}

simtime_t StochasticChannelModel::slotDuration(GHz carrierFrequency)
{
    return binder_->getSlotDurationFromNumerologyIndex(binder_->getNumerologyIndexFromCarrierFreq(carrierFrequency));
}

simtime_t StochasticChannelModel::lastCompletedSlotEnd(GHz carrierFrequency)
{
    return SimTime::fromRaw(NOW.raw() - NOW.raw() % slotDuration(carrierFrequency).raw());
}

std::vector<unsigned int> StochasticChannelModel::sharedBands(const CellularTransmission& transmission, unsigned int numBands, const RbMap& rbmap)
{
    std::vector<unsigned int> bands;
    // TODO fix for multi-antenna case
    auto antennaIt = transmission.grantedBlocks.find(MACRO);
    if (antennaIt == transmission.grantedBlocks.end())
        return bands;
    for (const auto& [band, allocation] : antennaIt->second) {
        if (band >= numBands)
            break;
        if (allocation == 0 || (!rbmap.empty() && rbmap.at(MACRO).at(band) == 0))
            continue;
        bands.push_back(band);
    }
    return bands;
}

bool StochasticChannelModel::computeUplinkInterference(MacNodeId eNbId, MacNodeId senderId, bool isCqi, GHz carrierFrequency, const RbMap& rbmap, std::vector<double> *interference)
{
    EV << "**** Uplink Interference for cellId[" << eNbId << "] node[" << senderId << "] ****" << endl;

    // whether an interfering UE counts at the base station
    auto interferes = [&] (const InterfererInfo& interferer) {
        // no self-interference
        if (interferer.nodeId == senderId)
            return false;

        // no interference from UL/D2D connections of the same cell (no D2D-UL reuse allowed)
        return interferer.cellId != eNbId;
    };

    // the attenuation from an interfering UE to the base station
    auto attenuationFrom = [&] (const InterfererInfo& interferer) {
        return getAttenuation(interferer.nodeId, UL, interferer.coord);
    };

    // what an interfering UE adds on the band, at the given attenuation
    auto addInterference = [&] (const InterfererInfo& interferer, unsigned int band, double att) {
        const MacNodeId ueId = interferer.nodeId;

        EV << NOW << " StochasticChannelModel::computeUplinkInterference - Interference from UE: " << ueId << "(dir " << dirToA(interferer.dir) << ") on band[" << band << "]" << endl;

        // get rx power from this UE
        double rxPwr = interferer.txPwr - cableLossOf(eNbId) + antennaGainOf(ueId, antennaGainUe_) + antennaGainOf(eNbId, antennaGainEnB_);
        (*interference)[band] += dBmToLinear(rxPwr - att);//(dBm-dB)=dBm

        EV << "\t band " << band << "/pwr[" << rxPwr - att << "]-int[" << (*interference)[band] << "]" << endl;
    };

    // a reception: the uplink transmissions of the slot just completed, on its bands -- the Binder's
    // map of the previous TTI lists the same ones; a CQI: those of the slot completed last, on every band
    static const RbMap everyBand;
    simtime_t slotEnd = isCqi ? lastCompletedSlotEnd(carrierFrequency) : NOW;
    simtime_t slotStart = slotEnd - slotDuration(carrierFrequency);
    ASSERT(isCqi || radioMedium_->matchesUplinkTransmissionMap(carrierFrequency, slotStart, slotEnd, binder_->getUlTransmissionMap(carrierFrequency, PREV_TTI)));
    const RbMap& receptionBands = isCqi ? everyBand : rbmap;
    // in creation order
    for (auto transmission : radioMedium_->getUplinkTransmissionsDuring(carrierFrequency, slotStart, slotEnd)) {
        // background cells' UEs interfere through computeBackgroundCellInterference()
        if (transmission->phantomCell != nullptr)
            continue;
        const InterfererInfo interferer = describeInterferer(*transmission);
        if (!interferes(interferer))
            continue;

        // a transmission on none of the reception's bands does not interfere with it: its link is not evaluated
        auto bands = sharedBands(*transmission, numBands_, receptionBands);
        if (bands.empty())
            continue;

        double att = attenuationFrom(interferer);
        for (unsigned int i : bands)
            addInterference(interferer, i, att);
    }

    // Debug Output
    EV << NOW << " StochasticChannelModel::computeUplinkInterference - Final Band Interference Status: " << endl;
    for (unsigned int i = 0; i < numBands_; i++)
        EV << "\t band " << i << " int[" << (*interference)[i] << "]" << endl;

    return true;
}

} //namespace
