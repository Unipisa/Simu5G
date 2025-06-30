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

#include "stack/phy/ChannelModel/LteRealisticChannelModel.h"

#include <fstream>
#include "common/cellInfo/CellInfo.h"
#include "stack/phy/packet/LteAirFrame.h"
#include "common/binder/Binder.h"
#include "stack/mac/amc/UserTxParams.h"
#include "common/LteCommon.h"
#include "nodes/ExtCell.h"
#include "nodes/backgroundCell/BackgroundScheduler.h"
#include "stack/phy/LtePhyUe.h"
#include "stack/mac/LteMacEnbD2D.h"

namespace simu5g {

// attenuation value to be returned if the maximum distance of a scenario has been violated
// and tolerating the maximum distance violation is enabled
#define ATT_MAXDISTVIOLATED    1000

using namespace inet;
using namespace omnetpp;
Define_Module(LteRealisticChannelModel);

simsignal_t LteRealisticChannelModel::rcvdSinrDlSignal_ = registerSignal("rcvdSinrDl");
simsignal_t LteRealisticChannelModel::rcvdSinrUlSignal_ = registerSignal("rcvdSinrUl");
simsignal_t LteRealisticChannelModel::rcvdSinrD2DSignal_ = registerSignal("rcvdSinrD2D");

simsignal_t LteRealisticChannelModel::measuredSinrDlSignal_ = registerSignal("measuredSinrDl");
simsignal_t LteRealisticChannelModel::measuredSinrUlSignal_ = registerSignal("measuredSinrUl");

void LteRealisticChannelModel::initialize(int stage)
{
    LteChannelModel::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        scenario_ = aToDeploymentScenario(par("scenario").stringValue());
        hNodeB_ = par("nodeb_height");
        shadowing_ = par("shadowing");
        hBuilding_ = par("building_height");
        inside_building_ = par("inside_building");
        if (inside_building_)
            inside_distance_ = uniform(0.0, 25.0);
        tolerateMaxDistViolation_ = par("tolerateMaxDistViolation");
        hUe_ = par("ue_height");

        wStreet_ = par("street_wide");

        correlationDistance_ = par("correlation_distance");
        harqReduction_ = par("harqReduction");

        lambdaMinTh_ = par("lambdaMinTh");
        lambdaMaxTh_ = par("lambdaMaxTh");
        lambdaRatioTh_ = par("lambdaRatioTh");

        antennaGainUe_ = par("antennaGainUe");
        antennaGainEnB_ = par("antennGainEnB");
        antennaGainMicro_ = par("antennGainMicro");
        thermalNoise_ = par("thermalNoise");
        cableLoss_ = par("cable_loss");
        ueNoiseFigure_ = par("ue_noise_figure");
        bsNoiseFigure_ = par("bs_noise_figure");
        useTorus_ = par("useTorus");
        dynamicLos_ = par("dynamic_los");
        fixedLos_ = par("fixed_los");

        fading_ = par("fading");
        std::string fType = par("fading_type");
        if (fType == "JAKES")
            fadingType_ = JAKES;
        else if (fType == "RAYLEIGH")
            fadingType_ = RAYLEIGH;
        else
            throw cRuntimeError("Unrecognized value in 'fading_type' parameter: \"%s\"", fType.c_str());

        fadingPaths_ = par("fading_paths");
        enableBackgroundCellInterference_ = par("bgCell_interference");
        enableExtCellInterference_ = par("extCell_interference");
        enableDownlinkInterference_ = par("downlink_interference");
        enableUplinkInterference_ = par("uplink_interference");
        enableD2DInterference_ = par("d2d_interference");
        delayRMS_ = par("delay_rms");

        enable_extCell_los_ = par("enable_extCell_los");

        collectSinrStatistics_ = par("collectSinrStatistics");

        //clear jakes fading map structure
        jakesFadingMap_.clear();
    }
}

double LteRealisticChannelModel::getAttenuation(MacNodeId nodeId, Direction dir,
        Coord coord, bool cqiDl)
{
    double speed = .0;
    double correlationDist = .0;

    //COMPUTE DISTANCE between UE and eNodeB
    double sqrDistance = phy_->getCoord().distance(coord);

    if (dir == DL) { // sender is UE
        speed = computeSpeed(nodeId, phy_->getCoord());
        correlationDist = computeCorrelationDistance(nodeId, phy_->getCoord());
    }
    else {
        speed = computeSpeed(nodeId, coord);
        correlationDist = computeCorrelationDistance(nodeId, coord);
    }

    // If Euclidean distance since last LOS probability computation is greater than
    // correlation distance the UE could have changed its state and
    // its visibility from eNodeB, hence it is correct to recompute the LOS probability
    if (correlationDist > correlationDistance_
        || losMap_.find(nodeId) == losMap_.end())
    {
        computeLosProbability(sqrDistance, nodeId);
    }

    //compute attenuation based on selected scenario and based on LOS or NLOS
    bool los = losMap_[nodeId];
    double dbp = 0;
    double attenuation = computePathLoss(sqrDistance, dbp, los);

    //    Applying shadowing only if it is enabled by configuration
    //    log-normal shadowing (not available for background UEs)
    if (nodeId < BGUE_MIN_ID && shadowing_)
        attenuation += computeShadowing(sqrDistance, nodeId, speed, cqiDl);

    // update current user position

    //if sender is an eNodeB
    if (dir == DL) {
        //store the position of user
        updatePositionHistory(nodeId, phy_->getCoord());
        updateCorrelationDistance(nodeId, phy_->getCoord());
    }
    else {
        //sender is an UE
        updatePositionHistory(nodeId, coord);
        updateCorrelationDistance(nodeId, coord);
    }

    EV << "LteRealisticChannelModel::getAttenuation - computed attenuation at distance " << sqrDistance << " for eNB is " << attenuation << endl;

    return attenuation;
}

double LteRealisticChannelModel::getAttenuation_D2D(MacNodeId nodeId, Direction dir, Coord coord, MacNodeId node2_Id, Coord coord_2, bool cqiDl)
{
    double speed = .0;
    double correlationDist = .0;

    //COMPUTE DISTANCE between UE1 and UE2
    double sqrDistance = coord.distance(coord_2);
    speed = computeSpeed(nodeId, coord);
    correlationDist = computeCorrelationDistance(nodeId, coord);

    // If Euclidean distance since last LOS probability computation is greater than
    // correlation distance the UE could have changed its state and
    // its visibility from eNodeB, hence it is correct to recompute the LOS probability
    if (correlationDist > correlationDistance_
        || losMap_.find(nodeId) == losMap_.end())
    {
        computeLosProbability(sqrDistance, nodeId);
    }

    //compute attenuation based on selected scenario and based on LOS or NLOS
    bool los = losMap_[nodeId];
    double dbp = 0;
    double attenuation = computePathLoss(sqrDistance, dbp, los);

    //    Applying shadowing only if it is enabled by configuration
    //    log-normal shadowing (not available for background UEs)
    if (nodeId < BGUE_MIN_ID && shadowing_)
        attenuation += computeShadowing(sqrDistance, nodeId, speed, cqiDl);

    // update current user position
    updatePositionHistory(nodeId, coord);

    EV << "LteRealisticChannelModel::getAttenuation - computed attenuation at distance " << sqrDistance << " for UE2 is " << attenuation << endl;

    return attenuation;
}

double LteRealisticChannelModel::computeShadowing(double sqrDistance, MacNodeId nodeId, double speed, bool cqiDl)
{
    ShadowFadingMap *actualShadowingMap;

    if (cqiDl) // if we are computing a DL CQI we need the Shadowing Map stored on the UE side
        actualShadowingMap = obtainShadowingMap(nodeId);
    else
        actualShadowingMap = &lastComputedSF_;

    if (actualShadowingMap == nullptr)
        throw cRuntimeError("LteRealisticChannelModel::computeShadowing - actualShadowingMap not found (nullptr)");

    double mean = 0;
    double dbp = 0.0;
    //Get std deviation according to LOS/NLOS and selected scenario

    double stdDev = getStdDev(sqrDistance < dbp, nodeId);
    double time = 0;
    double space = 0;
    double att;

    // if direction is DOWNLINK it means that this module is located in the UE stack than
    // the Move object associated with the UE is myMove_ variable
    // if direction is UPLINK it means that this module is located in the UE stack than
    // the Move object associated with the UE is move variable

    // if shadowing for current user has never been computed
    if (actualShadowingMap->find(nodeId) == actualShadowingMap->end()) {
        //Get the log-normal shadowing with std deviation stdDev
        att = normal(mean, stdDev);

        //store the shadowing attenuation for this user and the temporal mark
        std::pair<simtime_t, double> tmp(NOW, att);
        (*actualShadowingMap)[nodeId] = tmp;

        //If the shadowing attenuation has been computed at least one time for this user
        // and the distance traveled by the UE is greater than correlation distance
    }
    else if ((NOW - actualShadowingMap->at(nodeId).first).dbl() * speed
             > correlationDistance_)
    {

        //get the temporal mark of the last computed shadowing attenuation
        time = (NOW - actualShadowingMap->at(nodeId).first).dbl();

        //compute the traveled distance
        space = time * speed;

        //Compute shadowing with an EAW (Exponential Average Window) (step 1)
        double a = exp(-0.5 * (space / correlationDistance_));

        //Get last shadowing attenuation computed
        double old = actualShadowingMap->at(nodeId).second;

        //Compute shadowing with an EAW (Exponential Average Window) (step 2)
        att = a * old + sqrt(1 - pow(a, 2)) * normal(mean, stdDev);

        // Store the new computed shadowing
        std::pair<simtime_t, double> tmp(NOW, att);
        (*actualShadowingMap)[nodeId] = tmp;

        // if the distance traveled by the UE is smaller than correlation distance shadowing attenuation remains the same
    }
    else {
        att = actualShadowingMap->at(nodeId).second;
    }

    return att;
}

void LteRealisticChannelModel::updatePositionHistory(const MacNodeId nodeId,
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

void LteRealisticChannelModel::updateCorrelationDistance(const MacNodeId nodeId, const inet::Coord coord) {

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

double LteRealisticChannelModel::computeCorrelationDistance(const MacNodeId nodeId, const inet::Coord coord) {
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

double LteRealisticChannelModel::computeSpeed(const MacNodeId nodeId,
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

double LteRealisticChannelModel::computeAngle(Coord center, Coord point) {
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

        //          "] - relativePos[" << relx << "," << rely <<
    //          "] - siny[" << rely/dist << "] - senx[" << relx/dist <<
    //          "]" << endl;

    return angle;
}

double LteRealisticChannelModel::computeVerticalAngle(Coord center, Coord point)
{
    double threeDimDistance = center.distance(point);
    double twoDimDistance = getTwoDimDistance(center, point);
    double arccos = acos(twoDimDistance / threeDimDistance) * 180.0 / M_PI;
    return 90 + arccos;
}

double LteRealisticChannelModel::computeAngularAttenuation(double hAngle, double vAngle) {

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

std::vector<double> LteRealisticChannelModel::getSINR(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    // get tx power
    double recvPower = lteInfo->getTxPower(); // dBm

    // Get the Resource Blocks used to transmit this packet
    RbMap rbmap = lteInfo->getGrantedBlocks();

    // get move object associated with the packet
    // this object is referred to eNodeB if direction is DL or UE if direction is UL
    Coord coord = lteInfo->getCoord();

    // position of eNb and UE
    Coord ueCoord;
    Coord enbCoord;

    double antennaGainTx = 0.0;
    double antennaGainRx = 0.0;
    double noiseFigure = 0.0;
    double speed = 0.0;

    // true if we are computing a CQI for the DL direction
    bool cqiDl = false;

    MacNodeId ueId = NODEID_NONE;
    MacNodeId eNbId = NODEID_NONE;

    Direction dir = (Direction)lteInfo->getDirection();

    EV << "------------ GET SINR ----------------" << endl;
    //===================== PARAMETERS SETUP ============================
    /*
     * if direction is DL and this is not a feedback packet,
     * this function has been called by LteRealisticChannelModel::error() in the UE
     *
     *         Downlink error computation
     */
    if (dir == DL && (lteInfo->getFrameType() != FEEDBACKPKT)) {
        // set noise figure
        noiseFigure = ueNoiseFigure_; // dB
        // set antenna gain figure
        antennaGainTx = antennaGainEnB_; // dB
        antennaGainRx = antennaGainUe_;  // dB

        // get MacId for UE and eNb
        ueId = lteInfo->getDestId();
        eNbId = lteInfo->getSourceId();

        // get position of UE and eNb
        ueCoord = phy_->getCoord();
        enbCoord = lteInfo->getCoord();

        speed = computeSpeed(ueId, phy_->getCoord());
    }
    /*
     * If direction is UL OR
     * if the packet is a feedback packet
     * it means that this function is called by the feedback computation module
     *
     * located in the eNodeB that computes the feedback received by the UE
     * Hence the UE macNodeId can be taken from the sourceId of the lteInfo
     * and the speed of the UE is contained in the Move object associated with the lteInfo
     */
    else { // UL/DL CQI & UL error computation
        // get MacId for UE and eNb
        ueId = lteInfo->getSourceId();
        eNbId = lteInfo->getDestId();

        if (dir == DL) {
            // set noise figure
            noiseFigure = ueNoiseFigure_; // dB
            // set antenna gain figure
            antennaGainTx = antennaGainEnB_; // dB
            antennaGainRx = antennaGainUe_;  // dB

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
        speed = computeSpeed(ueId, coord);

        // get position of UE and eNb
        ueCoord = coord;
        enbCoord = phy_->getCoord();
    }

    CellInfo *eNbCell = getCellInfo(binder_, eNbId);
    const char *eNbTypeString = eNbCell ? (eNbCell->getEnbType() == MACRO_ENB ? "MACRO" : "MICRO") : "NULL";

    EV << "LteRealisticChannelModel::getSINR - srcId=" << lteInfo->getSourceId()
       << " - destId=" << lteInfo->getDestId()
       << " - DIR=" << ((dir == DL) ? "DL" : "UL")
       << " - frameType=" << ((lteInfo->getFrameType() == FEEDBACKPKT) ? "feedback" : "other")
       << endl
       << eNbTypeString << " - txPwr " << lteInfo->getTxPower()
       << " - ueCoord[" << ueCoord << "] - enbCoord[" << enbCoord << "] - ueId[" << ueId << "] - enbId[" << eNbId << "]" <<
        endl;
    //=================== END PARAMETERS SETUP =======================

    //=============== PATH LOSS + SHADOWING + FADING =================
    EV << "\t using parameters - noiseFigure=" << noiseFigure << " - antennaGainTx=" << antennaGainTx << " - antennaGainRx=" << antennaGainRx <<
        " - txPwr=" << lteInfo->getTxPower() << " - for ueId=" << ueId << endl;

    // attenuation for the desired signal
    double attenuation;
    if ((lteInfo->getFrameType() == FEEDBACKPKT))
        attenuation = getAttenuation(ueId, UL, coord, cqiDl); // dB
    else
        attenuation = getAttenuation(ueId, dir, coord, cqiDl); // dB

    // compute attenuation (PATHLOSS + SHADOWING)
    recvPower -= attenuation; // (dBm-dB)=dBm

    // add antenna gain
    recvPower += antennaGainTx; // (dBm+dB)=dBm
    recvPower += antennaGainRx; // (dBm+dB)=dBm

    // sub cable loss
    recvPower -= cableLoss_; // (dBm-dB)=dBm

    //=============== ANGULAR ATTENUATION =================
    if (dir == DL) {
        // get tx angle
        cModule *eNbModule = getSimulation()->getModule(binder_->getOmnetId(eNbId));
        LtePhyBase *ltePhy = eNbModule ?
            check_and_cast<LtePhyBase *>(eNbModule->getSubmodule("cellularNic")->getSubmodule("phy")) :
            nullptr;

        if (ltePhy && ltePhy->getTxDirection() == ANISOTROPIC) {
            // get tx angle
            double txAngle = ltePhy->getTxAngle();

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
    //=============== END ANGULAR ATTENUATION =================

    std::vector<double> snrVector;
    snrVector.resize(numBands_, 0.0);

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
                fadingAttenuation = rayleighFading(ueId, i);

            else if (fadingType_ == JAKES)
                fadingAttenuation = jakesFading(ueId, speed, i, cqiDl);
        }
        // add fading contribution to the received power
        double finalRecvPower = recvPower + fadingAttenuation; // (dBm+dB)=dBm

        // if tx mode is multi-user the tx power is divided by the number of paired users
        // in dB, divided by 2 means -3dB
        if (lteInfo->getTxMode() == MULTI_USER) {
            finalRecvPower -= 3;
        }

        EV << " LteRealisticChannelModel::getSINR node " << ueId
           << ((lteInfo->getFrameType() == FEEDBACKPKT) ?
            " FEEDBACK PACKET " : " NORMAL PACKET ")
           << " band " << i << " recvPower " << recvPower
           << " direction " << dirToA(dir) << " antenna gain tx "
           << antennaGainTx << " antenna gain rx " << antennaGainRx
           << " noise figure " << noiseFigure
           << " cable loss   " << cableLoss_
           << " attenuation (pathloss + shadowing) " << attenuation
           << " speed " << speed << " thermal noise " << thermalNoise_
           << " fading attenuation " << fadingAttenuation << endl;

        snrVector[i] = finalRecvPower;
    }
    //============ END PATH LOSS + SHADOWING + FADING ===============

    /*
     * The SINR will be calculated as follows
     *
     *              Pwr
     * SINR = ---------
     *           N  +  I
     *
     * Ndb = thermalNoise_ + noiseFigure (measured in decibels)
     * I = extCellInterference + multiCellInterference
     */

    //============ MULTI CELL INTERFERENCE COMPUTATION =================
    // vector containing the sum of multi-cell interference for each band
    std::vector<double> multiCellInterference; // Linear value (mW)
    // prepare data structure
    multiCellInterference.resize(numBands_, 0);
    if (enableDownlinkInterference_ && dir == DL && lteInfo->getFrameType() != HANDOVERPKT) {
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

    //===================== SINR COMPUTATION ========================
    // compute and linearize total noise
    double totN = dBmToLinear(thermalNoise_ + noiseFigure);

    // denominator expressed in dBm as (N+extCell+bgCell+multiCell)
    double den;
    EV << "LteRealisticChannelModel::getSINR - distance from my eNb=" << enbCoord.distance(ueCoord) << " - DIR=" << ((dir == DL) ? "DL" : "UL") << endl;

    double sumSnr = 0.0;
    int usedRBs = 0;
    // add interference for each band
    for (unsigned int i = 0; i < numBands_; i++) {
        // if we are decoding a data transmission and this RB has not been used, skip it
        // TODO fix for multi-antenna case
        if (lteInfo->getFrameType() == DATAPKT && rbmap[MACRO][i] == 0)
            continue;

        //               (      mW              +          mW            +  mW  +        mW            )
        den = linearToDBm(bgCellInterference[i] + extCellInterference[i] + totN + multiCellInterference[i]);

        EV << "\t bgCell[" << bgCellInterference[i] << "] - ext[" << extCellInterference[i] << "] - multi[" << multiCellInterference[i] << "] - recvPwr["
           << dBmToLinear(snrVector[i]) << "] - sinr[" << snrVector[i] - den << "]\n";

        // compute final SINR
        snrVector[i] -= den;

        sumSnr += snrVector[i];
        ++usedRBs;
    }

    // emit SINR statistic
    if (collectSinrStatistics_ && (lteInfo->getFrameType() == FEEDBACKPKT) && usedRBs > 0) {
        // we are on the BS, so we need to retrieve the channel model of the sender
        // XXX I know, there might be a faster way...
        LteChannelModel *ueChannelModel = check_and_cast<LtePhyUe *>(getPhyByMacNodeId(binder_, ueId))->getChannelModel(lteInfo->getCarrierFrequency());

        if (dir == DL) // we are on the UE
            ueChannelModel->emit(measuredSinrDlSignal_, sumSnr / usedRBs);
        else
            ueChannelModel->emit(measuredSinrUlSignal_, sumSnr / usedRBs);
    }

    // if sender is an eNodeB
    if (dir == DL)
        // store the position of user
        updatePositionHistory(ueId, phy_->getCoord());
    // sender is a UE
    else
        updatePositionHistory(ueId, coord);
    return snrVector;
}

std::vector<double> LteRealisticChannelModel::getRSRP(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    // get tx power
    double recvPower = lteInfo->getTxPower(); // dBm

    // Get the Resource Blocks used to transmit this packet
    RbMap rbmap = lteInfo->getGrantedBlocks();

    // get move object associated with the packet
    // this object is referred to eNodeB if the direction is DL or UE if the direction is UL
    Coord coord = lteInfo->getCoord();

    // position of eNb and UE
    Coord ueCoord;
    Coord enbCoord;

    double antennaGainTx = 0.0;
    double antennaGainRx = 0.0;
    double noiseFigure = 0.0;
    double speed = 0.0;

    // true if we are computing a CQI for the DL direction
    bool cqiDl = false;

    MacNodeId ueId = NODEID_NONE;
    MacNodeId eNbId = NODEID_NONE;

    Direction dir = (Direction)lteInfo->getDirection();

    EV << "------------ GET SINR ----------------" << endl;
    // ===================== PARAMETERS SETUP ============================
    /*
     * if direction is DL and this is not a feedback packet,
     * this function has been called by LteRealisticChannelModel::error() in the UE
     *
     *         Downlink error computation
     */
    if (dir == DL && (lteInfo->getFrameType() != FEEDBACKPKT)) {
        // set noise Figure
        noiseFigure = ueNoiseFigure_; // dB
        // set antenna gain Figure
        antennaGainTx = antennaGainEnB_; // dB
        antennaGainRx = antennaGainUe_;  // dB

        // get MacId for UE and eNB
        ueId = lteInfo->getDestId();
        eNbId = lteInfo->getSourceId();

        // get position of UE and eNB
        ueCoord = phy_->getCoord();
        enbCoord = lteInfo->getCoord();

        speed = computeSpeed(ueId, phy_->getCoord());
    }
    /*
     * If direction is UL OR
     * if the packet is a feedback packet
     * it means that this function is called by the feedback computation module
     *
     * located in the eNodeB that computes the feedback received by the UE
     * Hence the UE macNodeId can be taken from the sourceId of the lteInfo
     * and the speed of the UE is contained in the Move object associated with the lteInfo
     */
    else { // UL/DL CQI & UL error computation
        // get MacId for UE and eNB
        ueId = lteInfo->getSourceId();
        eNbId = lteInfo->getDestId();

        if (dir == DL) {
            // set noise Figure
            noiseFigure = ueNoiseFigure_; // dB
            // set antenna gain Figure
            antennaGainTx = antennaGainEnB_; // dB
            antennaGainRx = antennaGainUe_;  // dB

            // use the jakes map on the UE side
            cqiDl = true;
        }
        else { // if( dir == UL )
            // TODO check if antennaGainEnB should be added in UL direction too
            antennaGainTx = antennaGainUe_;
            antennaGainRx = antennaGainEnB_;
            noiseFigure = bsNoiseFigure_;

            // use the jakes map on the eNB side
            cqiDl = false;
        }
        speed = computeSpeed(ueId, coord);

        // get position of UE and eNB
        ueCoord = coord;
        enbCoord = phy_->getCoord();
    }

    CellInfo *eNbCell = getCellInfo(binder_, eNbId);
    const char *eNbTypeString = eNbCell ? (eNbCell->getEnbType() == MACRO_ENB ? "MACRO" : "MICRO") : "NULL";

    EV << "LteRealisticChannelModel::getRSRP - srcId=" << lteInfo->getSourceId()
       << " - destId=" << lteInfo->getDestId()
       << " - DIR=" << ((dir == DL) ? "DL" : "UL")
       << " - frameType=" << ((lteInfo->getFrameType() == FEEDBACKPKT) ? "feedback" : "other")
       << endl
       << eNbTypeString << " - txPwr " << lteInfo->getTxPower()
       << " - ueCoord[" << ueCoord << "] - enbCoord[" << enbCoord << "] - ueId[" << ueId << "] - enbId[" << eNbId << "]" <<
        endl;
    // =================== END PARAMETERS SETUP =======================

    // =============== PATH LOSS + SHADOWING + FADING =================
    EV << "\t using parameters - noiseFigure=" << noiseFigure << " - antennaGainTx=" << antennaGainTx << " - antennaGainRx=" << antennaGainRx <<
        " - txPwr=" << lteInfo->getTxPower() << " - for ueId=" << ueId << endl;

    // attenuation for the desired signal
    double attenuation;
    if ((lteInfo->getFrameType() == FEEDBACKPKT))
        attenuation = getAttenuation(ueId, UL, coord, cqiDl); // dB
    else
        attenuation = getAttenuation(ueId, dir, coord, cqiDl); // dB

    // compute attenuation (PATHLOSS + SHADOWING)
    recvPower -= attenuation; // (dBm-dB)=dBm

    // add antenna gain
    recvPower += antennaGainTx; // (dBm+dB)=dBm
    recvPower += antennaGainRx; // (dBm+dB)=dBm

    // sub cable loss
    recvPower -= cableLoss_; // (dBm-dB)=dBm

    // =============== ANGULAR ATTENUATION =================
    if (dir == DL) {
        // get tx angle
        cModule *eNbModule = getSimulation()->getModule(binder_->getOmnetId(eNbId));
        LtePhyBase *ltePhy = eNbModule ?
            check_and_cast<LtePhyBase *>(eNbModule->getSubmodule("cellularNic")->getSubmodule("phy")) :
            nullptr;

        if (ltePhy && ltePhy->getTxDirection() == ANISOTROPIC) {
            // get tx angle
            double txAngle = ltePhy->getTxAngle();

            // compute the angle between uePosition and reference axis, considering the eNB as center
            double ueAngle = computeAngle(enbCoord, ueCoord);

            // compute the reception angle between ue and eNB
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
                fadingAttenuation = rayleighFading(ueId, i);

            else if (fadingType_ == JAKES)
                fadingAttenuation = jakesFading(ueId, speed, i, cqiDl);
        }
        // add fading contribution to the received power
        double finalRecvPower = recvPower + fadingAttenuation; // (dBm+dB)=dBm

        // if txmode is multi-user the tx power is divided by the number of paired users
        // in dB, divided by 2 means -3dB
        if (lteInfo->getTxMode() == MULTI_USER) {
            finalRecvPower -= 3;
        }

        EV << " LteRealisticChannelModel::getRSRP node " << ueId
           << ((lteInfo->getFrameType() == FEEDBACKPKT) ?
            " FEEDBACK PACKET " : " NORMAL PACKET ")
           << " band " << i << " recvPower " << recvPower
           << " direction " << dirToA(dir) << " antenna gain tx "
           << antennaGainTx << " antenna gain rx " << antennaGainRx
           << " noise figure " << noiseFigure
           << " cable loss   " << cableLoss_
           << " attenuation (pathloss + shadowing) " << attenuation
           << " speed " << speed << " thermal noise " << thermalNoise_
           << " fading attenuation " << fadingAttenuation << endl;

        rsrpVector[i] = finalRecvPower;
    }
    // ============ END PATH LOSS + SHADOWING + FADING ===============

    return rsrpVector;
}

std::vector<double> LteRealisticChannelModel::getSINR_bgUe(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    //get tx power
    double recvPower = lteInfo->getTxPower(); // dBm

    // get MacId and Direction
    MacNodeId bgUeId = lteInfo->getSourceId();
    MacNodeId eNbId = lteInfo->getDestId();
    Direction dir = (Direction)lteInfo->getDirection();

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

    CellInfo *eNbCell = getCellInfo(binder_, eNbId);
    const char *eNbTypeString = eNbCell ? (eNbCell->getEnbType() == MACRO_ENB ? "MACRO" : "MICRO") : "NULL";

    EV << "LteRealisticChannelModel::getSINR_bgUe - DIR=" << ((dir == DL) ? "DL" : "UL")
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
        cModule *eNbModule = getSimulation()->getModule(binder_->getOmnetId(eNbId));
        LtePhyBase *ltePhy = eNbModule ?
            check_and_cast<LtePhyBase *>(eNbModule->getSubmodule("cellularNic")->getSubmodule("phy")) :
            nullptr;

        if (ltePhy && ltePhy->getTxDirection() == ANISOTROPIC) {
            // get tx angle
            double txAngle = ltePhy->getTxAngle();

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
                fadingAttenuation = jakesFading(bgUeId, speed, i, cqiDl, true);
        }
        // add fading contribution to the received power
        double finalRecvPower = recvPower + fadingAttenuation; // (dBm+dB)=dBm

        //if txmode is multi user the tx power is divided by the number of paired users
        // in dB divided by 2 means -3dB
        if (lteInfo->getTxMode() == MULTI_USER) {
            finalRecvPower -= 3;
        }

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

double LteRealisticChannelModel::getReceivedPower_bgUe(double txPower, inet::Coord txPos, inet::Coord rxPos, Direction dir, bool losStatus, MacNodeId bsId)
{
    double antennaGainTx = 0.0;
    double antennaGainRx = 0.0;

    EV << NOW << " LteRealisticChannelModel::getReceivedPower_bgUe" << endl;

    //===================== PARAMETERS SETUP ============================
    if (dir == DL) {
        antennaGainTx = antennaGainEnB_; //dB
        antennaGainRx = antennaGainUe_;  //dB
    }
    else { // if( dir == UL )
        antennaGainTx = antennaGainUe_;
        antennaGainRx = antennaGainEnB_;
    }

    EV << "LteRealisticChannelModel::getReceivedPower_bgUe - DIR=" << ((dir == DL) ? "DL" : "UL")
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
        cModule *bsModule = getSimulation()->getModule(binder_->getOmnetId(bsId));
        LtePhyBase *phy = bsModule ? check_and_cast<LtePhyBase *>(bsModule->getSubmodule("cellularNic")->getSubmodule("phy")) : nullptr;

        if (phy && phy->getTxDirection() == ANISOTROPIC) {
            // get tx angle
            double txAngle = phy->getTxAngle();

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

std::vector<double> LteRealisticChannelModel::getRSRP_D2D(LteAirFrame *frame, UserControlInfo *lteInfo_1, MacNodeId destId, Coord destCoord)
{
    // AttenuationVector::iterator it;
    // Get Tx power
    double recvPower = lteInfo_1->getD2dTxPower(); // dBm

    // Coordinate of the Sender of the Feedback packet
    Coord sourceCoord = lteInfo_1->getCoord();

    double antennaGainTx = 0.0;
    double antennaGainRx = 0.0;
    double noiseFigure = 0.0;
    double speed = 0.0;
    // Get MacId for UE and his peer
    MacNodeId sourceId = lteInfo_1->getSourceId();
    std::vector<double> rsrpVector;

    // True if we use the jakes map in the UE side (D2D is like DL for the receivers)
    bool cqiDl = false;
    // Get the direction
    Direction dir = (Direction)lteInfo_1->getDirection();
    dir = D2D; //todo[stsc]: dir is overridden? why?

    EV << "------------ GET RSRP D2D----------------" << endl;

    //===================== PARAMETERS SETUP ============================

    // D2D CQI or D2D error computation

    if (dir == UL || dir == DL) {
        //consistency check
        throw cRuntimeError("Direction should neither be UL nor DL");
    }
    else {
        antennaGainTx = antennaGainRx = antennaGainUe_;
        //In D2D case the noise figure is the ueNoiseFigure_
        noiseFigure = ueNoiseFigure_;
        // use the jakes map in the UE side
        cqiDl = true;
    }
    // Compute speed
    speed = computeSpeed(sourceId, sourceCoord);

    EV << "LteRealisticChannelModel::getRSRP_D2D - srcId=" << sourceId
       << " - destId=" << destId
       << " - DIR=" << dirToA(dir)
       << " - frameType=" << ((lteInfo_1->getFrameType() == FEEDBACKPKT) ? "feedback" : "other")
       << endl
       << " - txPwr " << recvPower
       << " - ue1_Coord[" << sourceCoord << "] - ue2_Coord[" << destCoord << "] - ue1_Id[" << sourceId << "] - ue2_Id[" << destId << "]" <<
        endl;
    //=================== END PARAMETERS SETUP =======================

    //=============== PATH LOSS + SHADOWING + FADING =================
    EV << "\t using parameters - noiseFigure=" << noiseFigure << " - antennaGainTx=" << antennaGainTx << " - antennaGainRx=" << antennaGainRx <<
        " - txPwr=" << recvPower << " - for ueId=" << sourceId << endl;

    // attenuation for the desired signal
    double attenuation = getAttenuation_D2D(sourceId, dir, sourceCoord, destId, destCoord, cqiDl); // dB

    //compute attenuation (PATHLOSS + SHADOWING)
    recvPower -= attenuation; // (dBm-dB)=dBm

    //add antenna gain
    recvPower += antennaGainTx; // (dBm+dB)=dBm
    recvPower += antennaGainRx; // (dBm+dB)=dBm

    //sub cable loss
    recvPower -= cableLoss_; // (dBm-dB)=dBm

    // compute and add interference due to fading
    // Apply fading for each band
    // if the phy layer is localized we can assume that for each logical band we have different fading attenuation
    // if the phy layer is distributed the number of logical band should be set to 1
    double fadingAttenuation = 0;
    //for each logical band
    for (unsigned int i = 0; i < numBands_; i++) {
        fadingAttenuation = 0;
        //if fading is enabled
        if (fading_) {
            //Applying fading
            if (fadingType_ == RAYLEIGH)
                fadingAttenuation = rayleighFading(sourceId, i);

            else if (fadingType_ == JAKES) {
                fadingAttenuation = jakesFading(sourceId, speed, i, cqiDl);
            }
        }
        // add fading contribution to the received power
        double finalRecvPower = recvPower + fadingAttenuation; // (dBm+dB)=dBm

        //if txmode is multi-user the tx power is divided by the number of paired users
        // in dB divided by 2 means -3dB
        if (lteInfo_1->getTxMode() == MULTI_USER) {
            finalRecvPower -= 3;
        }

        EV << " LteRealisticChannelModel::getRSRP_D2D node " << sourceId
           << ((lteInfo_1->getFrameType() == FEEDBACKPKT) ?
            " FEEDBACK PACKET " : " NORMAL PACKET ")
           << " band " << i << " recvPower " << recvPower
           << " direction " << dirToA(dir) << " antenna gain tx "
           << antennaGainTx << " antenna gain rx " << antennaGainRx
           << " noise figure " << noiseFigure
           << " cable loss   " << cableLoss_
           << " attenuation (pathloss + shadowing) " << attenuation
           << " speed " << speed << " thermal noise " << thermalNoise_
           << " fading attenuation " << fadingAttenuation << endl;

        // Store the calculated receive power
        rsrpVector.push_back(finalRecvPower);
    }
    //============ END PATH LOSS + SHADOWING + FADING ===============

    return rsrpVector;
}

std::vector<double> LteRealisticChannelModel::getSINR_D2D(LteAirFrame *frame, UserControlInfo *lteInfo, MacNodeId destId, Coord destCoord, MacNodeId enbId)
{
    // AttenuationVector::iterator it;
    // Get Tx power
    double recvPower = lteInfo->getD2dTxPower(); // dBm

    // Get allocated RBs
    RbMap rbmap = lteInfo->getGrantedBlocks();

    // Coordinate of the Sender of the Feedback packet
    Coord sourceCoord = lteInfo->getCoord();

    double antennaGainTx = 0.0;
    double antennaGainRx = 0.0;
    double noiseFigure = 0.0;
    double speed = 0.0;
    double extCellInterference = 0;
    // Get MacId for UE and his peer
    MacNodeId sourceId = lteInfo->getSourceId();
    std::vector<double> snrVector;
    snrVector.resize(numBands_, 0.0);

    // True if we use the jakes map in the UE side (D2D is like DL for the receivers)
    bool cqiDl = true;
    // Get the direction
    Direction dir = D2D;

    EV << "------------ GET SINR D2D ----------------" << endl;

    //===================== PARAMETERS SETUP ============================

    // antenna gain is antennaGainUe for both tx and rx
    antennaGainTx = antennaGainRx = antennaGainUe_;
    // In D2D case the noise figure is the ueNoiseFigure_
    noiseFigure = ueNoiseFigure_;

    // Compute speed
    speed = computeSpeed(sourceId, sourceCoord);

    EV << "LteRealisticChannelModel::getSINR_d2d - srcId=" << sourceId
       << " - destId=" << destId
       << " - DIR=" << dirToA(dir)
       << " - frameType=" << ((lteInfo->getFrameType() == FEEDBACKPKT) ? "feedback" : "other")
       << endl
       << " - txPwr " << recvPower
       << " - ue1_Coord[" << sourceCoord << "] - ue2_Coord[" << destCoord << "] - ue1_Id[" << sourceId << "] - ue2_Id[" << destId << "]" <<
        endl;
    //=================== END PARAMETERS SETUP =======================

    //=============== PATH LOSS + SHADOWING + FADING =================
    EV << "\t using parameters - noiseFigure=" << noiseFigure << " - antennaGainTx=" << antennaGainTx << " - antennaGainRx=" << antennaGainRx <<
        " - txPwr=" << recvPower << " - for ueId=" << sourceId << endl;

    // attenuation for the desired signal
    double attenuation = getAttenuation_D2D(sourceId, dir, sourceCoord, destId, destCoord, cqiDl); // dB

    //compute attenuation (PATHLOSS + SHADOWING)
    recvPower -= attenuation; // (dBm-dB)=dBm

    //add antenna gain
    recvPower += antennaGainTx; // (dBm+dB)=dBm
    recvPower += antennaGainRx; // (dBm+dB)=dBm

    //sub cable loss
    recvPower -= cableLoss_; // (dBm-dB)=dBm

    // compute and add interference due to fading
    // Apply fading for each band
    // if the phy layer is localized we can assume that for each logical band we have different fading attenuation
    // if the phy layer is distributed the number of logical band should be set to 1
    double fadingAttenuation = 0;
    //for each logical band
    for (unsigned int i = 0; i < numBands_; i++) {
        fadingAttenuation = 0;
        //if fading is enabled
        if (fading_) {
            //Applying fading
            if (fadingType_ == RAYLEIGH)
                fadingAttenuation = rayleighFading(sourceId, i);

            else if (fadingType_ == JAKES) {
                fadingAttenuation = jakesFading(sourceId, speed, i, cqiDl);
            }
        }
        // add fading contribution to the received power
        double finalRecvPower = recvPower + fadingAttenuation; // (dBm+dB)=dBm

        //if txmode is multi-user the tx power is divided by the number of paired users
        // in dB divided by 2 means -3dB
        if (lteInfo->getTxMode() == MULTI_USER) {
            finalRecvPower -= 3;
        }

        EV << " LteRealisticChannelModel::getSINR_d2d node " << sourceId
           << ((lteInfo->getFrameType() == FEEDBACKPKT) ?
            " FEEDBACK PACKET " : " NORMAL PACKET ")
           << " band " << i << " recvPower " << recvPower
           << " direction " << dirToA(dir) << " antenna gain tx "
           << antennaGainTx << " antenna gain rx " << antennaGainRx
           << " noise figure " << noiseFigure
           << " cable loss   " << cableLoss_
           << " attenuation (pathloss + shadowing) " << attenuation
           << " speed " << speed << " thermal noise " << thermalNoise_
           << " fading attenuation " << fadingAttenuation << endl;

        // Store the calculated receive power
        snrVector[i] = finalRecvPower;
    }
    //============ END PATH LOSS + SHADOWING + FADING ===============

    /*
     * The SINR will be calculated as follows
     *
     *              Pwr
     * SINR = ---------
     *           N  +  I
     *
     * N = thermalNoise_ + noiseFigure (measured in dBm)
     * I = extCellInterference + inCellInterference (measured in mW)
     */
    //============ D2D INTERFERENCE COMPUTATION =================
    /*
     * In calculating a D2D CQI the interference from other UEs discriminates between calculating a CQI
     * following direction D2D_Tx--->D2D_Rx or D2D_Tx<---D2D_Rx (This happens due to the different positions of the
     * interfering UEs relative to the position of the UE for whom we are calculating the CQI). We need that the CQI
     * for the D2D_Tx is the same as the D2D_Rx (This is a help for the simulator because when the eNodeB allocates
     * resources to a D2D_Tx it must refer to the quality channel of the D2D_Rx).
     * To do so here we must check if the ueId is the ID of the D2D_Tx: if it
     * is so we swap the ueId with the one of his Peer (D2D_Rx). We do the same for the coord.
     */
    //vector containing the sum of in-cell interference for each band
    std::vector<double> d2dInterference; // Linear value (mW)
    // prepare data structure
    d2dInterference.resize(numBands_, 0);
    if (enableD2DInterference_) {
        computeD2DInterference(enbId, sourceId, sourceCoord, destId, destCoord, (lteInfo->getFrameType() == FEEDBACKPKT), lteInfo->getCarrierFrequency(), rbmap, &d2dInterference, dir);
    }

    //===================== SINR COMPUTATION ========================
    if (enableD2DInterference_) {
        // compute and linearize total noise
        double totN = dBmToLinear(thermalNoise_ + noiseFigure);

        // denominator expressed in dBm as (N+extCell+inCell)
        double den;
        EV << "LteRealisticChannelModel::getSINR - distance from my Peer = " << destCoord.distance(sourceCoord) << " - DIR=" << dirToA(dir) << endl;

        // Add interference for each band
        for (unsigned int i = 0; i < numBands_; i++) {
            // if we are decoding a data transmission and this RB has not been used, skip it
            // TODO fix for multi-antenna case
            if (lteInfo->getFrameType() == DATAPKT && rbmap[MACRO][i] == 0)
                continue;

            //               (      mW            +  mW  +        mW            )
            den = linearToDBm(extCellInterference + totN + d2dInterference[i]);

            EV << "\t ext[" << extCellInterference << "] - in[" << d2dInterference[i] << "] - recvPwr["
               << dBmToLinear(snrVector[i]) << "] - sinr[" << snrVector[i] - den << "]\n";

            // compute final SINR. Subtraction in dB is equivalent to linear division
            snrVector[i] -= den;
        }
    }
    // compute snr with no D2D interference
    else {
        for (unsigned int i = 0; i < numBands_; i++) {
            // if we are decoding a data transmission and this RB has not been used, skip it
            // TODO fix for multi-antenna case
            if (lteInfo->getFrameType() == DATAPKT && rbmap[MACRO][i] == 0)
                continue;

            /*
               std::cout<<"SNR "<<i<<" "<<snrVector[i]<<endl;
               std::cout<<"noise figure "<<i<<" "<<noiseFigure<<endl;
               std::cout<<"Thermal noise "<<i<<" "<<thermalNoise_<<endl;
             */
            // compute final SINR
            snrVector[i] -= (noiseFigure + thermalNoise_);

            EV << "LteRealisticChannelModel::getSINR_d2d - distance from my Peer = " << destCoord.distance(sourceCoord) << " - DIR=" << dirToA(dir) << " - snr[" << snrVector[i] << "]\n";
        }
    }
    //sender is a UE
    updatePositionHistory(sourceId, sourceCoord);

    return snrVector;
}

std::vector<double> LteRealisticChannelModel::getSINR_D2D(LteAirFrame *frame, UserControlInfo *lteInfo_1, MacNodeId destId, Coord destCoord, MacNodeId enbId, const std::vector<double>& rsrpVector)
{
    std::vector<double> snrVector = rsrpVector;

    MacNodeId sourceId = lteInfo_1->getSourceId();
    Coord sourceCoord = lteInfo_1->getCoord();

    // Get allocated RBs
    RbMap rbmap = lteInfo_1->getGrantedBlocks();

    // Get the direction
    Direction dir = D2D;

    double noiseFigure = 0.0;
    double extCellInterference = 0.0;

    // In D2D case the noise figure is the ueNoiseFigure_
    noiseFigure = ueNoiseFigure_;

    EV << "------------ GET SINR D2D----------------" << endl;

    /*
     * The SINR will be calculated as follows
     *
     *              Pwr
     * SINR = ---------
     *           N  +  I
     *
     * N = thermalNoise_ + noiseFigure (measured in dBm)
     * I = extCellInterference + inCellInterference (measured in mW)
     */
    //============ IN CELL D2D INTERFERENCE COMPUTATION =================
    /*
     * In calculating a D2D CQI, the interference from other D2D UEs discriminates between calculating a CQI
     * in the direction D2D_Tx--->D2D_Rx or D2D_Tx<---D2D_Rx (This happens due to the different positions of the
     * interfering UEs relative to the position of the UE for whom we are calculating the CQI). We need that the CQI
     * for the D2D_Tx is the same as the D2D_Rx (This is a help for the simulator because when the eNodeB allocates
     * resources to a D2D_Tx it must refer to the quality channel of the D2D_Rx).
     * To do so, here we must check if the ueId is the ID of the D2D_Tx: if it
     * is so we swap the ueId with the one of his Peer (D2D_Rx). We do the same for the coord.
     */
    // vector containing the sum of inCell interference for each band
    std::vector<double> d2dInterference; // Linear value (mW)
    // prepare data structure
    d2dInterference.resize(numBands_, 0);
    if (enableD2DInterference_) {
        computeD2DInterference(enbId, sourceId, sourceCoord, destId, destCoord, (lteInfo_1->getFrameType() == FEEDBACKPKT), lteInfo_1->getCarrierFrequency(), rbmap, &d2dInterference, dir);
    }

    //===================== SINR COMPUTATION ========================
    if (enableD2DInterference_) {
        // compute and linearize total noise
        double totN = dBmToLinear(thermalNoise_ + noiseFigure);

        // denominator expressed in dBm as (N+extCell+inCell)
        double den;
        EV << "LteRealisticChannelModel::getSINR - distance from my Peer = " << destCoord.distance(sourceCoord) << " - DIR=" << dirToA(dir) << endl;

        // Add interference for each band
        for (unsigned int i = 0; i < numBands_; i++) {
            // if we are decoding a data transmission and this RB has not been used, skip it
            // TODO fix for multi-antenna case
            if (lteInfo_1->getFrameType() == DATAPKT && rbmap[MACRO][i] == 0)
                continue;

            //               (      mW            +  mW  +        mW            )
            den = linearToDBm(extCellInterference + totN + d2dInterference[i]);

            EV << "\t ext[" << extCellInterference << "] - in[" << d2dInterference[i] << "] - recvPwr["
               << dBmToLinear(snrVector[i]) << "] - sinr[" << snrVector[i] - den << "]\n";

            // compute final SINR. Subtraction in dB is equivalent to linear division
            snrVector[i] -= den;
        }
    }
    // compute snr with no D2D interference
    else {
        for (unsigned int i = 0; i < numBands_; i++) {
            // if we are decoding a data transmission and this RB has not been used, skip it
            // TODO fix for multi-antenna case
            if (lteInfo_1->getFrameType() == DATAPKT && rbmap[MACRO][i] == 0)
                continue;

            // compute final SINR
            snrVector[i] -= (noiseFigure + thermalNoise_);

            EV << "LteRealisticChannelModel::getSINR_D2D - distance from my Peer = " << destCoord.distance(sourceCoord) << " - DIR=" << dirToA(dir) << " - snr[" << snrVector[i] << "]\n";
        }
    }

    // sender is a UE
    updatePositionHistory(sourceId, sourceCoord);

    return snrVector;
}

std::vector<double> LteRealisticChannelModel::getSIR(LteAirFrame *frame,
        UserControlInfo *lteInfo)
{
    // AttenuationVector::iterator it;
    // get tx power
    double recvPower = lteInfo->getTxPower();

    Coord coord = lteInfo->getCoord();

    Direction dir = (Direction)lteInfo->getDirection();

    MacNodeId id = NODEID_NONE;
    double speed = 0.0;

    // if direction is DL
    if (dir == DL && (lteInfo->getFrameType() != FEEDBACKPKT)) {
        id = lteInfo->getDestId();
        speed = computeSpeed(id, phy_->getCoord());
    }
    /*
     * If direction is UL OR
     * if the packet is a feedback packet
     * it means that this function is called by the feedback computation module
     * located in the eNodeB that computes the feedback received by the UE
     * Hence, the UE macNodeId can be taken from the sourceId of the lteInfo
     * and the speed of the UE is contained in the Move object associated with the lteInfo
     */
    else {
        id = lteInfo->getSourceId();
        speed = computeSpeed(id, coord);
    }

    // Apply fading for each band
    // if the phy layer is localized we can assume that for each logical band we have different fading attenuation
    // if the phy layer is distributed, the number of logical bands should be set to 1
    std::vector<double> snrVector;

    double fadingAttenuation = 0;
    // for each logical band
    for (unsigned int i = 0; i < numBands_; i++) {
        fadingAttenuation = 0;
        // if fading is enabled
        if (fading_) {
            // Applying fading
            if (fadingType_ == RAYLEIGH) {
                fadingAttenuation = rayleighFading(id, i);
            }
            else if (fadingType_ == JAKES) {
                fadingAttenuation = jakesFading(id, speed, i, dir);
            }
        }
        // add fading contribution to the final SINR
        double finalSnr = recvPower + fadingAttenuation;

        // if txmode is multi-user, the tx power is divided by the number of paired users
        // using dB, to divide by 2 means -3dB
        if (lteInfo->getTxMode() == MULTI_USER)
            finalSnr -= 3;

        snrVector.push_back(finalSnr);
    }

    // if sender is an eNodeB
    if (dir == DL)
        // store the position of the user
        updatePositionHistory(id, phy_->getCoord());
    // sender is a UE
    else
        updatePositionHistory(id, coord);
    return snrVector;
}

double LteRealisticChannelModel::rayleighFading(MacNodeId id,
        unsigned int band)
{
    // get rayleigh variable from trace file
    double temp1 = binder_->phyPisaData.getChannel(
            getCellInfo(binder_, id)->getLambda(id)->channelIndex + band);
    return linearToDb(temp1);
}

double LteRealisticChannelModel::jakesFading(MacNodeId nodeId, double speed,
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
        actualJakesMap = (!isBgUe) ? obtainUeJakesMap(nodeId) : &jakesFadingMapBgUe_;
    else
        actualJakesMap = &jakesFadingMap_;

    // if this is the first time that we compute fading for current user
    if (actualJakesMap->find(nodeId) == actualJakesMap->end()) {
        // clear the map
        // FIXME: possible memory leak
        (*actualJakesMap)[nodeId].clear();

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
            (*actualJakesMap)[nodeId].push_back(temp);
        }
    }
    // convert carrier frequency from GHz to Hz
    double f = carrierFrequency_ * 1000000000;

    // get transmission time start (TTI = 1ms)
    simtime_t t = simTime().dbl() - 0.001;

    double re_h = 0;
    double im_h = 0;

    const JakesFadingData& actualJakesData = actualJakesMap->at(nodeId).at(band);

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

bool LteRealisticChannelModel::isError(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    EV << "LteRealisticChannelModel::error" << endl;

    // get codeword
    unsigned char cw = lteInfo->getCw();
    // get number of codewords
    int size = lteInfo->getUserTxParams()->readCqiVector().size();

    // get position associated with the packet
    // Coord coord = lteInfo->getCoord();

    // if total number of codewords is equal to 1 the cw index should be only 0
    if (size == 1)
        cw = 0;

    // get cqi used to transmit this cw
    Cqi cqi = lteInfo->getUserTxParams()->readCqiVector()[cw];

    MacNodeId id;
    Direction dir = (Direction)lteInfo->getDirection();

    // Get MacNodeId of UE
    if (dir == DL)
        id = lteInfo->getDestId();
    else
        id = lteInfo->getSourceId();

    // Get Number of RTX
    unsigned char nTx = lteInfo->getTxNumber();

    // consistency check
    if (nTx == 0)
        throw cRuntimeError("Transmissions counter should not be 0");

    // Get txmode
    TxMode txmode = (TxMode)lteInfo->getTxMode();

    // If rank is 1 and we used SMUX to transmit we have to corrupt this packet
    if (txmode == CL_SPATIAL_MULTIPLEXING
        || txmode == OL_SPATIAL_MULTIPLEXING)
    {
        // compare lambda min (smaller eigenvalues of channel matrix) with the threshold used to compute the rank
        if (binder_->phyPisaData.getLambda(num(id), 1) < lambdaMinTh_)
            return false;
    }

    // Take sinr
    std::vector<double> snrV;
    if (lteInfo->getDirection() == D2D || lteInfo->getDirection() == D2D_MULTI) {
        MacNodeId destId = lteInfo->getDestId();
        Coord destCoord = phy_->getCoord();
        MacNodeId enbId = binder_->getNextHop(lteInfo->getSourceId());
        snrV = getSINR_D2D(frame, lteInfo, destId, destCoord, enbId);
    }
    else {
        snrV = getSINR(frame, lteInfo);
    }

    // Get the resource Block id used to transmit this packet
    RbMap rbmap = lteInfo->getGrantedBlocks();

    // Get txmode
    unsigned int itxmode = txModeToIndex[txmode];

    double bler = 0;
    std::vector<double> totalbler;
    double finalSuccess = 1;

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

            // check the antenna used in Das
            if ((lteInfo->getTxMode() == CL_SPATIAL_MULTIPLEXING
                 || lteInfo->getTxMode() == OL_SPATIAL_MULTIPLEXING)
                && rbmap.size() > 1)
                // we consider only the snr associated with the LB used
                if (remoteUnit != lteInfo->getCw())
                    continue;

            // Get the Bler
            if (cqi == 0 || cqi > 15)
                throw cRuntimeError("A packet has been transmitted with a cqi equal to 0 or greater than 15 cqi:%d txmode:%d dir:%d rb:%d cw:%d rtx:%d", cqi, lteInfo->getTxMode(), dir, band, cw, nTx);

            // for statistical purposes
            sumSnr += snrV[band];
            usedRBs++;

            int snr = snrV[band];// XXX because band is a Band (=unsigned short)
            if (snr < binder_->phyPisaData.minSnr())
                return false;
            else if (snr > binder_->phyPisaData.maxSnr())
                bler = 0;
            else
                bler = binder_->phyPisaData.getBler(itxmode, cqi - 1, snr);

            EV << "\t bler computation: [itxMode=" << itxmode << "] - [cqi-1=" << cqi - 1
               << "] - [snr=" << snr << "]" << endl;

            double success = 1 - bler;
            // compute the success probability according to the number of RB used
            double successPacket = pow(success, (double)allocation);
            // compute the success probability according to the number of LB used
            finalSuccess *= successPacket;

            EV << " LteRealisticChannelModel::error direction " << dirToA(dir)
               << " node " << id << " remote unit " << dasToA(remoteUnit)
               << " Band " << band << " SNR " << snr << " CQI " << cqi
               << " BLER " << bler << " success probability " << successPacket
               << " total success probability " << finalSuccess << endl;
        }
    }
    // Compute total error probability
    double per = 1 - finalSuccess;
    // Harq Reduction
    double totalPer = per * pow(harqReduction_, nTx - 1);

    double er = uniform(0.0, 1.0);

    EV << " LteRealisticChannelModel::error direction " << dirToA(dir)
       << " node " << id << " total ERROR probability  " << per
       << " per with H-ARQ error reduction " << totalPer
       << " - CQI[" << cqi << "]- random error extracted[" << er << "]" << endl;

    // emit SINR statistic
    if (collectSinrStatistics_ && usedRBs > 0) {
        if (dir == DL) // we are on the UE
            emit(rcvdSinrDlSignal_, sumSnr / usedRBs);
        else {
            // we are on the BS, so we need to retrieve the channel model of the sender
            // XXX I know, there might be a faster way...
            LteChannelModel *ueChannelModel = check_and_cast<LtePhyUe *>(getPhyByMacNodeId(binder_, id))->getChannelModel(lteInfo->getCarrierFrequency());
            ueChannelModel->emit(rcvdSinrUlSignal_, sumSnr / usedRBs);
        }
    }

    if (er <= totalPer) {
        EV << "This is NOT your lucky day (" << er << " < " << totalPer
           << ") -> do not receive." << endl;

        // Signal too weak, we can't receive it
        return false;
    }
    // Signal is strong enough, receive this Signal
    EV << "This is your lucky day (" << er << " > " << totalPer
       << ") -> Receive AirFrame." << endl;

    return true;
}

bool LteRealisticChannelModel::isError_D2D(LteAirFrame *frame, UserControlInfo *lteInfo, const std::vector<double>& rsrpVector)
{
    EV << "LteRealisticChannelModel::error_D2D" << endl;

    // get codeword
    unsigned char cw = lteInfo->getCw();
    // get number of codewords
    int size = lteInfo->getUserTxParams()->readCqiVector().size();

    // get position associated with the packet
    // Coord coord = lteInfo->getCoord();

    // if total number of codewords is equal to 1 the cw index should be only 0
    if (size == 1)
        cw = 0;

    // Get CQI used to transmit this cw
    Cqi cqi = lteInfo->getUserTxParams()->readCqiVector()[cw];
    EV << "LteRealisticChannelModel:: CQI: " << cqi << endl;

    MacNodeId id;
    Direction dir = (Direction)lteInfo->getDirection();

    // Get MacNodeId of UE
    if (dir == DL)
        id = lteInfo->getDestId();
    else // UL or D2D
        id = lteInfo->getSourceId();

    EV << NOW << "LteRealisticChannelModel::FROM: " << id << endl;
    // Get Number of RTX
    unsigned char nTx = lteInfo->getTxNumber();

    // consistency check
    if (nTx == 0)
        throw cRuntimeError("Transmissions counter should not be 0");

    // Get txmode
    TxMode txmode = (TxMode)lteInfo->getTxMode();

    // If rank is 1 and we used SMUX to transmit we have to corrupt this packet
    if (txmode == CL_SPATIAL_MULTIPLEXING
        || txmode == OL_SPATIAL_MULTIPLEXING)
    {
        // compare lambda min (smaller eigenvalues of channel matrix) with the threshold used to compute the rank
        if (binder_->phyPisaData.getLambda(num(id), 1) < lambdaMinTh_)
            return false;
    }
    // SINR vector(one SINR value for each band)
    std::vector<double> snrV;
    if (lteInfo->getDirection() == D2D || lteInfo->getDirection() == D2D_MULTI) {
        MacNodeId peerUeMacNodeId = lteInfo->getDestId();
        Coord peerCoord = phy_->getCoord();
        MacNodeId enbId = MacNodeId(1); // TODO get an appropriate way to get EnbId

        if (lteInfo->getDirection() == D2D) {
            snrV = getSINR_D2D(frame, lteInfo, peerUeMacNodeId, peerCoord, enbId);
        }
        else { // D2D_MULTI
            snrV = getSINR_D2D(frame, lteInfo, peerUeMacNodeId, peerCoord, enbId, rsrpVector);
        }
    }
    // ROSSALI-------END------------------------------------------------
    else snrV = getSINR(frame, lteInfo);                                           // Take SINR

    // Get the resource Block id used to transmit this packet
    RbMap rbmap = lteInfo->getGrantedBlocks();

    // Get txmode
    unsigned int itxmode = txModeToIndex[txmode];

    double bler = 0;
    std::vector<double> totalbler;
    double finalSuccess = 1;

    // for statistical purposes
    double sumSnr = 0.0;
    int usedRBs = 0;

    // for each Remote unit used to transmit the packet
    for (const auto& [remoteUnitId, resourceBlocks] : rbmap) {
        // for each logical band used to transmit the packet
        for (const auto& [band, allocation] : resourceBlocks) {
            // this Rb is not allocated
            if (allocation == 0) continue;

            // check the antenna used in Das
            if ((lteInfo->getTxMode() == CL_SPATIAL_MULTIPLEXING
                 || lteInfo->getTxMode() == OL_SPATIAL_MULTIPLEXING)
                && rbmap.size() > 1)
                // we consider only the snr associated with the LB used
                if (remoteUnitId != lteInfo->getCw()) continue;

            // Get the Bler
            if (cqi == 0 || cqi > 15)
                throw cRuntimeError("A packet has been transmitted with a cqi equal to 0 or greater than 15 cqi:%d txmode:%d dir:%d rb:%d cw:%d rtx:%d", cqi, lteInfo->getTxMode(), dir, band, cw, nTx);

            // for statistical purposes
            sumSnr += snrV[band];
            usedRBs++;

            int snr = snrV[band];// XXX because band is a Band (=unsigned short)
            if (snr < 1)                           // XXX it was < 0
                return false;
            else if (snr > binder_->phyPisaData.maxSnr())
                bler = 0;
            else
                bler = binder_->phyPisaData.getBler(itxmode, cqi - 1, snr);

            EV << "\t bler computation: [itxMode=" << itxmode << "] - [cqi-1=" << cqi - 1
               << "] - [snr=" << snr << "]" << endl;

            double success = 1 - bler;
            // compute the success probability according to the number of RB used
            double successPacket = pow(success, (double)allocation);

            // compute the success probability according to the number of LB used
            finalSuccess *= successPacket;

            EV << " LteRealisticChannelModel::error direction " << dirToA(dir)
               << " node " << id << " remote unit " << dasToA(remoteUnitId)
               << " Band " << band << " SNR " << snr << " CQI " << cqi
               << " BLER " << bler << " success probability " << successPacket
               << " total success probability " << finalSuccess << endl;
        }
    }
    // Compute total error probability
    double per = 1 - finalSuccess;
    // Harq Reduction
    double totalPer = per * pow(harqReduction_, nTx - 1);

    double er = uniform(0.0, 1.0);

    EV << " LteRealisticChannelModel::error direction " << dirToA(dir)
       << " node " << id << " total ERROR probability  " << per
       << " per with H-ARQ error reduction " << totalPer
       << " - CQI[" << cqi << "]- random error extracted[" << er << "]" << endl;

    // emit SINR statistic
    if (collectSinrStatistics_ && usedRBs > 0)
        emit(rcvdSinrD2DSignal_, sumSnr / usedRBs);

    if (er <= totalPer) {
        EV << "This is NOT your lucky day (" << er << " < " << totalPer << ") -> do not receive." << endl;

        // Signal too weak, we can't receive it
        return false;
    }
    // Signal is strong enough, receive this Signal
    EV << "This is your lucky day (" << er << " > " << totalPer << ") -> Receive AirFrame." << endl;

    return true;
}

void LteRealisticChannelModel::computeLosProbability(double d,
        MacNodeId nodeId)
{
    double p = 0;
    if (!dynamicLos_) {
        losMap_[nodeId] = fixedLos_;
        return;
    }
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
            p = (((18 / d) > 1) ? 1 : 18 / d) * (1 - exp(-1 * d / 36))
                + exp(-1 * d / 36);
            break;
        case RURAL_MACROCELL:
            if (d <= 10)
                p = 1;
            else
                p = exp(-1 * (d - 10) / 200);
            break;
        case SUBURBAN_MACROCELL:
            if (d <= 10)
                p = 1;
            else
                p = exp(-1 * (d - 10) / 1000);
            break;
        default:
            throw cRuntimeError("Wrong path-loss scenario value %d", scenario_);
    }
    double random = uniform(0.0, 1.0);
    if (random <= p)
        losMap_[nodeId] = true;
    else
        losMap_[nodeId] = false;
}

double LteRealisticChannelModel::computePathLoss(double distance, double dbp, bool los)
{
    // compute attenuation based on selected scenario and based on LOS or NLOS
    double pathLoss = 0;
    switch (scenario_) {
        case INDOOR_HOTSPOT:
            pathLoss = computeIndoor(distance, los);
            break;
        case URBAN_MICROCELL:
            pathLoss = computeUrbanMicro(distance, los);
            break;
        case URBAN_MACROCELL:
            pathLoss = computeUrbanMacro(distance, los);
            break;
        case RURAL_MACROCELL:
            pathLoss = computeRuralMacro(distance, dbp, los);
            break;
        case SUBURBAN_MACROCELL:
            pathLoss = computeSubUrbanMacro(distance, dbp, los);
            break;
        default:
            throw cRuntimeError("Wrong value %d for path-loss scenario", scenario_);
    }
    return pathLoss;
}

double LteRealisticChannelModel::computeIndoor(double d, bool los)
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
    return a * log10(d) + b + 20 * log10(carrierFrequency_);
}

double LteRealisticChannelModel::computeUrbanMicro(double d, bool los)
{
    if (d < 10)
        d = 10;

    double dbp = 4 * (hNodeB_ - 1) * (hUe_ - 1)
        * ((carrierFrequency_ * 1000000000) / SPEED_OF_LIGHT);
    if (los) {
        // LOS situation
        if (d > 5000) {
            if (tolerateMaxDistViolation_)
                return ATT_MAXDISTVIOLATED;
            else
                throw cRuntimeError("Error: LOS urban microcell path loss model is valid for d < 5000 m");
        }
        if (d < dbp)
            return 22 * log10(d) + 28 + 20 * log10(carrierFrequency_);
        else
            return 40 * log10(d) + 7.8 - 18 * log10(hNodeB_ - 1)
                   - 18 * log10(hUe_ - 1) + 2 * log10(carrierFrequency_);
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
    return 36.7 * log10(d) + 22.7 + 26 * log10(carrierFrequency_);
}

double LteRealisticChannelModel::computeUrbanMacro(double d, bool los)
{
    if (d < 10)
        d = 10;

    double dbp = 4 * (hNodeB_ - 1) * (hUe_ - 1)
        * ((carrierFrequency_ * 1000000000) / SPEED_OF_LIGHT);
    if (los) {
        if (d > 5000) {
            if (tolerateMaxDistViolation_)
                return ATT_MAXDISTVIOLATED;
            else
                throw cRuntimeError("Error: LOS urban macrocell path loss model is valid for d < 5000 m");
        }
        if (d < dbp)
            return 22 * log10(d) + 28 + 20 * log10(carrierFrequency_);
        else
            return 40 * log10(d) + 7.8 - 18 * log10(hNodeB_ - 1)
                   - 18 * log10(hUe_ - 1) + 2 * log10(carrierFrequency_);
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
        + 20 * log10(carrierFrequency_)
        - (3.2 * (pow(log10(11.75 * hUe_), 2)) - 4.97);
    return att;
}

double LteRealisticChannelModel::computeSubUrbanMacro(double d, double& dbp, bool los)
{
    if (d < 10)
        d = 10;

    dbp = 4 * (hNodeB_ - 1) * (hUe_ - 1)
        * ((carrierFrequency_ * 1000000000) / SPEED_OF_LIGHT);
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
        double b = (b1 < 14.72) ? b1 : 14.72;
        if (d < dbp) {
            double first = 20 * log10((40 * M_PI * d * carrierFrequency_) / 3);
            double second = a * log10(d);
            double fourth = 0.002 * log10(hBuilding_) * d;
            return first + second - b + fourth;
        }
        else
            return 20 * log10((40 * M_PI * dbp * carrierFrequency_) / 3)
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
        + 20 * log10(carrierFrequency_)
        - (3.2 * (pow(log10(11.75 * hUe_), 2)) - 4.97);
    return att;
}

double LteRealisticChannelModel::computeRuralMacro(double d, double& dbp, bool los)
{
    if (d < 10)
        d = 10;

    dbp = 4 * (hNodeB_ - 1) * (hUe_ - 1)
        * ((carrierFrequency_ * 1000000000) / SPEED_OF_LIGHT);
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
        double b = (b1 < 14.72) ? b1 : 14.72;
        if (d < dbp)
            return 20 * log10((40 * M_PI * d * carrierFrequency_) / 3)
                   + a * log10(d) - b + 0.002 * log10(hBuilding_) * d;
        else
            return 20 * log10((40 * M_PI * dbp * carrierFrequency_) / 3)
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
        + 20 * log10(carrierFrequency_)
        - (3.2 * (pow(log10(11.75 * hUe_), 2)) - 4.97);
    return att;
}

double LteRealisticChannelModel::getTwoDimDistance(inet::Coord a, inet::Coord b)
{
    a.z = 0.0;
    b.z = 0.0;
    return a.distance(b);
}

double LteRealisticChannelModel::getStdDev(bool dist, MacNodeId nodeId)
{
    switch (scenario_) {
        case URBAN_MICROCELL:
        case INDOOR_HOTSPOT:
            if (losMap_[nodeId])
                return 3.;
            else
                return 4.;
        case URBAN_MACROCELL:
            if (losMap_[nodeId])
                return 4.;
            else
                return 6.;
        case RURAL_MACROCELL:
        case SUBURBAN_MACROCELL:
            if (losMap_[nodeId]) {
                if (dist)
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

bool LteRealisticChannelModel::computeExtCellInterference(MacNodeId eNbId, MacNodeId nodeId, Coord coord, bool isCqi, double carrierFrequency,
        std::vector<double> *interference)
{
    EV << "**** Ext Cell Interference **** " << endl;

    // get external cell list
    ExtCellList list = binder_->getExtCellList(carrierFrequency);

    Coord c;
    double dist, // meters
           recvPwr, // watt
           recvPwrDBm, // dBm
           att, // dBm
           angularAtt; // dBm

    //compute distance for each cell
    for (auto& extCell : list) {
        // get external cell position
        c = extCell->getPosition();
        // compute distance between UE and the ext cell
        dist = coord.distance(c);

        EV << "\t distance between UE[" << coord.x << "," << coord.y <<
            "] and extCell[" << c.x << "," << c.y << "] is -> "
           << dist << "\t";

        // compute attenuation according to some path loss model
        att = computeExtCellPathLoss(dist, nodeId);

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

bool LteRealisticChannelModel::computeBackgroundCellInterference(MacNodeId nodeId, inet::Coord bsCoord, inet::Coord ueCoord, bool isCqi, double carrierFrequency, const RbMap& rbmap, Direction dir,
        std::vector<double> *interference)
{
    EV << "**** Background Cell Interference **** " << endl;

    // get bg schedulers list
    BackgroundSchedulerList *list = binder_->getBackgroundSchedulerList(carrierFrequency);
    BackgroundSchedulerList::iterator it = list->begin();

    Coord c;
    double dist, // meters
           txPwr, // dBm
           recvPwr, // watt
           recvPwrDBm, // dBm
           att, // dBm
           angularAtt; // dBm

    //compute distance for each cell
    while (it != list->end()) {
        if (dir == DL) {
            // compute interference with respect to the background base station

            // get external cell position
            c = (*it)->getPosition();
            // compute distance between UE and the ext cell
            dist = ueCoord.distance(c);

            EV << "\t distance between UE[" << ueCoord.x << "," << ueCoord.y <<
                "] and backgroundCell[" << c.x << "," << c.y << "] is -> "
               << dist << "\t";

            // compute attenuation according to some path loss model
            att = computeExtCellPathLoss(dist, nodeId);

            txPwr = (*it)->getTxPower();

            //=============== ANGULAR ATTENUATION =================
            if ((*it)->getTxDirection() == OMNI) {
                angularAtt = 0;
            }
            else {
                // compute the angle between uePosition and reference axis, considering the eNB as center
                double ueAngle = computeAngle(c, ueCoord);

                // compute the reception angle between ue and eNB
                double recvAngle = fabs((*it)->getTxAngle() - ueAngle);

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

            unsigned int numBands = std::min(numBands_, (*it)->getNumBands());
            EV << " - shared bands [" << numBands << "]\t";
            EV << " - interfering bands[";

            // add interference in those bands where the ext cell is active
            for (unsigned int i = 0; i < numBands; i++) {
                int occ = 0;
                if (isCqi) { // check slot occupation for this TTI
                    occ = (*it)->getBandStatus(i, DL);
                }
                else if (!rbmap.empty() && rbmap.at(MACRO).at(i) != 0) {     // error computation. We need to check the slot occupation of the previous TTI (only if the band has been used by the UE)
                    occ = (*it)->getPrevBandStatus(i, DL);
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

            unsigned int numBands = std::min(numBands_, (*it)->getNumBands());
            EV << " - shared bands [" << numBands << "]" << endl;

            // add interference in those bands where a UE in the background cell is active
            for (unsigned int i = 0; i < numBands; i++) {
                int occ = 0;

                if (isCqi) { // check slot occupation for this TTI
                    occ = (*it)->getBandStatus(i, UL);
                    if (occ)
                        bgUe = (*it)->getBandInterferingUe(i);
                }
                else if (rbmap.at(MACRO).at(i) != 0) {     // error computation. We need to check the slot occupation of the previous TTI (only if the band has been used by the UE)
                    occ = (*it)->getPrevBandStatus(i, UL);
                    if (occ)
                        bgUe = (*it)->getPrevBandInterferingUe(i);
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
                    att = computeExtCellPathLoss(dist, nodeId);

                    recvPwrDBm = txPwr - att - angularAtt - cableLoss_ + antennaGainEnB_ + antennaGainBgUe;
                    recvPwr = dBmToLinear(recvPwrDBm);

                    (*interference)[i] += recvPwr;
                }
            }
        }
        it++;
    }

    return true;
}

double LteRealisticChannelModel::computeExtCellPathLoss(double dist, MacNodeId nodeId)
{

    //compute attenuation based on selected scenario and based on LOS or NLOS
    bool los = losMap_[nodeId];

    if (!enable_extCell_los_)
        los = false;
    double dbp = 0;
    double attenuation = LteRealisticChannelModel::computePathLoss(dist, dbp, los);

//   //TODO Apply shadowing to each interfering extCell signal
//
//   //    Applying shadowing only if it is enabled by configuration
//   //    log-normal shadowing
//   if (shadowing_)
//   {
//       // double mean = 0;
//
//       //        Get std deviation according to los/nlos and selected scenario
//
//       //        double stdDev = getStdDev(dist < dbp, nodeId);
//       // double time = 0;
//       // double space = 0;
//       double att;
//       //
//       //
//       //        //If the shadowing attenuation has been computed at least one time for this user
//       //        // and the distance traveled by the UE is greated than correlation distance
//       //        if ((NOW - lastComputedSF_.at(nodeId).first).dbl() * speed > correlationDistance_)
//       //        {
//       //            //get the temporal mark of the last computed shadowing attenuation
//       //            time = (NOW - lastComputedSF_.at(nodeId).first).dbl();
//       //
//       //            //compute the traveled distance
//       //            space = time * speed;
//       //
//       //            //Compute shadowing with a EAW (Exponential Average Window) (step1)
//       //            double a = exp(-0.5 * (space / correlationDistance_));
//       //
//       //            //Get last shadowing attenuation computed
//       //            double old = lastComputedSF_.at(nodeId).second;
//       //
//       //            //Compute shadowing with a EAW (Exponential Average Window) (step2)
//       //            att = a * old + sqrt(1 - pow(a, 2)) * normal(getEnvir()->getRNG(0), mean, stdDev);
//       //        }
//       //         if the distance traveled by the UE is smaller than correlation distance shadowing attenuation remain the same
//       //        else
//       {
//           att = lastComputedSF_.at(nodeId).second;
//       }
//       attenuation += att;
//   }

    return attenuation;
}

LteRealisticChannelModel::JakesFadingMap *LteRealisticChannelModel::obtainUeJakesMap(MacNodeId id)
{
    // obtain a reference to UE phy
    LtePhyBase *phy = nullptr;

    std::vector<UeInfo *> *ueList = binder_->getUeList();
    for (auto ueInfo : *ueList) {
        if (ueInfo->id == id) {
            phy = ueInfo->phy;
            break;
        }
    }

    if (phy == nullptr)
        return nullptr;

    // get the associated channel and get a reference to its Jakes Map
    JakesFadingMap *j;
    LteRealisticChannelModel *re = dynamic_cast<LteRealisticChannelModel *>(phy->getChannelModel(carrierFrequency_));
    if (re == nullptr)
        throw cRuntimeError("LteRealisticChannelModel::obtainUeJakesMap - channel model is a null pointer. Abort.");
    else
        j = re->getJakesMap();

    return j;
}

LteRealisticChannelModel::ShadowFadingMap *LteRealisticChannelModel::obtainShadowingMap(MacNodeId id)
{
    // obtain a reference to UE phy
    LtePhyBase *phy = nullptr;

    std::vector<UeInfo *> *ueList = binder_->getUeList();
    for (auto ueInfo : *ueList) {
        if (ueInfo->id == id) {
            phy = ueInfo->phy;
            break;
        }
    }

    if (phy == nullptr)
        return nullptr;

    // get the associated channel and get a reference to its shadowing Map
    LteRealisticChannelModel *re = dynamic_cast<LteRealisticChannelModel *>(phy->getChannelModel(carrierFrequency_));
    ShadowFadingMap *j = re->getShadowingMap();
    return j;
}

bool LteRealisticChannelModel::computeDownlinkInterference(MacNodeId eNbId, MacNodeId ueId, Coord coord, bool isCqi, double carrierFrequency, const RbMap& rbmap,
        std::vector<double> *interference)
{
    EV << "**** Downlink Interference ****" << endl;

    // reference to the mac/phy/channel of each cell

    int temp;
    double att;

    double txPwr;

    std::vector<EnbInfo *> *enbList = binder_->getEnbList();
    std::vector<EnbInfo *>::iterator it = enbList->begin(), et = enbList->end();

    while (it != et) {
        MacNodeId id = (*it)->id;

        if (id == eNbId) {
            ++it;
            continue;
        }

        // initialize eNB data structures
        if (!(*it)->init) {
            // obtain a reference to eNB phy and obtain tx power
            (*it)->phy = check_and_cast<LtePhyBase *>(getSimulation()->getModule(binder_->getOmnetId(id))->getSubmodule("cellularNic")->getSubmodule("phy"));

            (*it)->txPwr = (*it)->phy->getTxPwr();//dBm

            // get tx direction
            (*it)->txDirection = (*it)->phy->getTxDirection();

            // get tx angle
            (*it)->txAngle = (*it)->phy->getTxAngle();

            //get reference to mac layer
            (*it)->mac = check_and_cast<LteMacEnb *>(getMacByMacNodeId(binder_, id));

            (*it)->init = true;
        }

        LteRealisticChannelModel *interfChanModel = dynamic_cast<LteRealisticChannelModel *>((*it)->phy->getChannelModel(carrierFrequency));

        // if the eNB does not use the selected carrier frequency, skip it
        if (interfChanModel == nullptr) {
            ++it;
            continue;
        }

        // compute attenuation using data structures within the cell
        att = interfChanModel->getAttenuation(ueId, UL, coord, isCqi);
        EV << "EnbId [" << id << "] - attenuation [" << att << "]";

        //=============== ANGULAR ATTENUATION =================
        double angularAtt = 0;
        if ((*it)->txDirection == ANISOTROPIC) {
            //get tx angle
            double txAngle = (*it)->txAngle;

            // compute the angle between uePosition and reference axis, considering the eNB as center
            double ueAngle = computeAngle(interfChanModel->phy_->getCoord(), coord);

            // compute the reception angle between ue and eNB
            double recvAngle = fabs(txAngle - ueAngle);
            if (recvAngle > 180)
                recvAngle = 360 - recvAngle;

            double verticalAngle = computeVerticalAngle(interfChanModel->phy_->getCoord(), coord);

            // compute attenuation due to sectorial tx
            angularAtt = computeAngularAttenuation(recvAngle, verticalAngle);

            EV << "angular attenuation [" << angularAtt << "]";
        }
        // else, antenna is omni-directional
        //=============== END ANGULAR ATTENUATION =================

        txPwr = (*it)->txPwr - angularAtt - cableLoss_ + antennaGainEnB_ + antennaGainUe_;

        unsigned int numBands = std::min(numBands_, interfChanModel->getNumBands());
        EV << " - shared bands [" << numBands << "]" << endl;

        if (isCqi) {// check slot occupation for this TTI
            for (unsigned int i = 0; i < numBands; i++) {
                // compute the number of occupied slot (unnecessary)
                temp = (*it)->mac->getDlBandStatus(i);
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
                temp = (*it)->mac->getDlPrevBandStatus(i);
                if (temp != 0)
                    (*interference)[i] += dBmToLinear(txPwr - att); //(dBm-dB)=dBm

                EV << "\t band " << i << " occupied " << temp << "/pwr[" << txPwr << "]-int[" << (*interference)[i] << "]" << endl;
            }
        }
        ++it;
    }

    return true;
}

bool LteRealisticChannelModel::computeUplinkInterference(MacNodeId eNbId, MacNodeId senderId, bool isCqi, double carrierFrequency, const RbMap& rbmap, std::vector<double> *interference)
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
                    MacNodeId ueId = ue_it.nodeId;
                    MacCellId cellId = ue_it.cellId;
                    Direction dir = ue_it.dir;
                    double txPwr;
                    inet::Coord ueCoord;
                    LtePhyUe *uePhy = nullptr;
                    TrafficGeneratorBase *trafficGen = nullptr;
                    if (ue_it.phy != nullptr) {
                        uePhy = check_and_cast<LtePhyUe *>(ue_it.phy);
                        txPwr = uePhy->getTxPwr(dir);
                        ueCoord = uePhy->getCoord();
                    }
                    else { // this is a backgroundUe
                        trafficGen = check_and_cast<TrafficGeneratorBase *>(ue_it.trafficGen);
                        txPwr = trafficGen->getTxPwr();
                        ueCoord = trafficGen->getCoord();
                    }

                    // no self-interference
                    if (ueId == senderId)
                        continue;

                    // no interference from UL/D2D connections of the same cell  (no D2D-UL reuse allowed)
                    if (cellId == eNbId)
                        continue;

                    EV << NOW << " LteRealisticChannelModel::computeUplinkInterference - Interference from UE: " << ueId << "(dir " << dirToA(dir) << ") on band[" << i << "]" << endl;

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
                    MacNodeId ueId = ue_it.nodeId;
                    MacCellId cellId = ue_it.cellId;
                    Direction dir = ue_it.dir;
                    double txPwr;
                    inet::Coord ueCoord;
                    LtePhyUe *uePhy = nullptr;
                    TrafficGeneratorBase *trafficGen = nullptr;
                    if (ue_it.phy != nullptr) {
                        uePhy = check_and_cast<LtePhyUe *>(ue_it.phy);
                        txPwr = uePhy->getTxPwr(dir);
                        ueCoord = uePhy->getCoord();
                    }
                    else { // this is a backgroundUe
                        trafficGen = check_and_cast<TrafficGeneratorBase *>(ue_it.trafficGen);
                        txPwr = trafficGen->getTxPwr();
                        ueCoord = trafficGen->getCoord();
                    }

                    // no self-interference
                    if (ueId == senderId)
                        continue;

                    // no interference from UL connections of the same cell (no D2D-UL reuse allowed)
                    if (cellId == eNbId)
                        continue;

                    EV << NOW << " LteRealisticChannelModel::computeUplinkInterference - Interference from UE: " << ueId << "(dir " << dirToA(dir) << ") on band[" << i << "]" << endl;

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
    EV << NOW << " LteRealisticChannelModel::computeUplinkInterference - Final Band Interference Status: " << endl;
    for (unsigned int i = 0; i < numBands_; i++)
        EV << "\t band " << i << " int[" << (*interference)[i] << "]" << endl;

    return true;
}

bool LteRealisticChannelModel::computeD2DInterference(MacNodeId eNbId, MacNodeId senderId, Coord senderCoord, MacNodeId destId, Coord destCoord, bool isCqi, double carrierFrequency, const RbMap& rbmap,
        std::vector<double> *interference, Direction dir)
{
    EV << "**** D2D Interference for cellId[" << eNbId << "] node[" << destId << "] ****" << endl;

    // get the reference to the MAC of the eNodeB
    LteMacEnbD2D *macEnb = check_and_cast<LteMacEnbD2D *>(binder_->getMacFromMacNodeId(eNbId));

    const std::vector<std::vector<UeAllocationInfo>> *ulTransmissionMap;
    const std::vector<UeAllocationInfo> *allocatedUes;

    if (isCqi) {// check slot occupation for this TTI
        ulTransmissionMap = binder_->getUlTransmissionMap(carrierFrequency, CURR_TTI);
        if (ulTransmissionMap != nullptr && !ulTransmissionMap->empty()) {
            for (unsigned int i = 0; i < numBands_; i++) {
                // get the UEs transmitting on the same band
                allocatedUes = &(ulTransmissionMap->at(i));

                for (auto& ue_it : *allocatedUes) {
                    MacNodeId ueId = ue_it.nodeId;
                    MacCellId cellId = ue_it.cellId;
                    Direction dir = ue_it.dir;
                    double txPwr;
                    inet::Coord ueCoord;
                    LtePhyUe *uePhy = nullptr;
                    TrafficGeneratorBase *trafficGen = nullptr;
                    if (ue_it.phy != nullptr) {
                        uePhy = check_and_cast<LtePhyUe *>(ue_it.phy);
                        txPwr = uePhy->getTxPwr(dir);
                        ueCoord = uePhy->getCoord();
                    }
                    else { // this is a backgroundUe
                        trafficGen = check_and_cast<TrafficGeneratorBase *>(ue_it.trafficGen);
                        txPwr = trafficGen->getTxPwr();
                        ueCoord = trafficGen->getCoord();
                    }

                    // no self-interference
                    if (ueId == senderId || ueId == destId)
                        continue;

                    // no interference from UL connections of the same cell (no D2D-UL reuse allowed)
                    if (dir == UL && cellId == eNbId)
                        continue;

                    // no interference from D2D connections of the same cell when reuse is disabled (otherwise, computation of CQI is misleading)
                    if (cellId == eNbId && (!macEnb->isReuseD2DEnabled() && !macEnb->isReuseD2DMultiEnabled()))
                        continue;

                    EV << NOW << " LteRealisticChannelModel::computeD2DInterference - Interference from UE: " << ueId << "(dir " << dirToA(dir) << ") on band[" << i << "]" << endl;

                    // get tx power and attenuation from this UE
                    double rxPwr = txPwr - cableLoss_ + 2 * antennaGainUe_;
                    double att = getAttenuation_D2D(ueId, D2D, ueCoord, destId, destCoord, false);
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
                // get the UEs transmitting on the same band
                allocatedUes = &(ulTransmissionMap->at(i));

                for (auto& ue_it : *allocatedUes) {
                    MacNodeId ueId = ue_it.nodeId;
                    MacCellId cellId = ue_it.cellId;
                    Direction dir = ue_it.dir;
                    double txPwr;
                    inet::Coord ueCoord;
                    LtePhyUe *uePhy = nullptr;
                    TrafficGeneratorBase *trafficGen = nullptr;
                    if (ue_it.phy != nullptr) {
                        uePhy = check_and_cast<LtePhyUe *>(ue_it.phy);
                        txPwr = uePhy->getTxPwr(dir);
                        ueCoord = uePhy->getCoord();
                    }
                    else { // this is a backgroundUe
                        trafficGen = check_and_cast<TrafficGeneratorBase *>(ue_it.trafficGen);
                        txPwr = trafficGen->getTxPwr();
                        ueCoord = trafficGen->getCoord();
                    }

                    // no self-interference
                    if (ueId == senderId || ueId == destId)
                        continue;

                    // no interference from UL connections of the same cell (no D2D-UL reuse allowed)
                    if (dir == UL && cellId == eNbId)
                        continue;

                    // no interference from D2D connections of the same cell when reuse is disabled
                    if (cellId == eNbId && (!macEnb->isReuseD2DEnabled() && !macEnb->isReuseD2DMultiEnabled()))
                        continue;

                    EV << NOW << " LteRealisticChannelModel::computeD2DInterference - Interference from UE: " << ueId << "(dir " << dirToA(dir) << ") on band[" << i << "]" << endl;

                    // get tx power and attenuation from this UE
                    double rxPwr = txPwr - cableLoss_ + 2 * antennaGainUe_;
                    double att = getAttenuation_D2D(ueId, D2D, ueCoord, destId, destCoord, false);
                    (*interference)[i] += dBmToLinear(rxPwr - att);//(dBm-dB)=dBm

                    EV << "\t band " << i << "/pwr[" << rxPwr - att << "]-int[" << (*interference)[i] << "]" << endl;
                }
            }
        }
    }

    // Debug Output
    EV << NOW << " LteRealisticChannelModel::computeD2DInterference - Final Band Interference Status: " << endl;
    for (unsigned int i = 0; i < numBands_; i++)
        EV << "\t band " << i << " int[" << (*interference)[i] << "]" << endl;

    return true;
}

} //namespace

