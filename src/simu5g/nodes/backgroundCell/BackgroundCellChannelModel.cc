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

#include "nodes/backgroundCell/BackgroundCellChannelModel.h"
#include "nodes/backgroundCell/BackgroundScheduler.h"
#include "stack/phy/LtePhyBase.h"
#include "stack/phy/LtePhyUe.h"
#include "stack/phy/ChannelModel/LteRealisticChannelModel.h"

namespace simu5g {

Define_Module(BackgroundCellChannelModel);

void BackgroundCellChannelModel::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        scenario_ = aToDeploymentScenario(par("scenario").stringValue());
        hNodeB_ = par("nodeb_height");
        hBuilding_ = par("building_height");
        inside_building_ = par("inside_building");
        if (inside_building_)
            inside_distance_ = uniform(0.0, 25.0);
        tolerateMaxDistViolation_ = par("tolerateMaxDistViolation");
        hUe_ = par("ue_height");

        wStreet_ = par("street_wide");

        antennaGainUe_ = par("antennaGainUe");
        antennaGainEnB_ = par("antennGainEnB");
        antennaGainMicro_ = par("antennGainMicro");
        thermalNoise_ = par("thermalNoise");
        cableLoss_ = par("cable_loss");
        ueNoiseFigure_ = par("ue_noise_figure");
        bsNoiseFigure_ = par("bs_noise_figure");
        shadowing_ = par("shadowing");
        correlationDistance_ = par("correlation_distance");

        dynamicLos_ = par("dynamic_los");
        fixedLos_ = par("fixed_los");

        fading_ = par("fading");
        std::string fType = par("fading_type");
        if (fType == "JAKES")
            fadingType_ = JAKES;
        else if (fType == "RAYLEIGH")
            fadingType_ = RAYLEIGH;
        else if (fType == "JAKES")
            fadingType_ = JAKES;
        else
            throw cRuntimeError("Unrecognized value in 'fading_type' parameter: \"%s\"", fType.c_str());

        fadingPaths_ = par("fading_paths");
        delayRMS_ = par("delay_rms");

        enableBackgroundCellInterference_ = par("bgCell_interference");
        enableDownlinkInterference_ = par("downlink_interference");
        enableUplinkInterference_ = par("uplink_interference");

        //get binder
        binder_.reference(this, "binderModule", true);
    }
}

std::vector<double> BackgroundCellChannelModel::getSINR(MacNodeId bgUeId, inet::Coord bgUePos, TrafficGeneratorBase *bgUe, BackgroundScheduler *bgScheduler, Direction dir)
{
    unsigned int numBands = bgScheduler->getNumBands();
    inet::Coord bgBsPos = bgScheduler->getPosition();
    int bgBsId = bgScheduler->getId();

    //get tx power
    double recvPower = (dir == DL) ? bgScheduler->getTxPower() : bgUe->getTxPwr(); // dBm

    double antennaGainTx = 0.0;
    double antennaGainRx = 0.0;
    double noiseFigure = 0.0;

    if (dir == DL) {
        //set noise Figure
        noiseFigure = ueNoiseFigure_; //dB
        //set antenna gain Figure
        antennaGainTx = antennaGainEnB_; //dB
        antennaGainRx = antennaGainUe_;  //dB
    }
    else { // if( dir == UL )
        // TODO check if antennaGainEnB should be added in UL direction too
        antennaGainTx = antennaGainUe_;
        antennaGainRx = antennaGainEnB_;
        noiseFigure = bsNoiseFigure_;
    }

    double attenuation = getAttenuation(bgUeId, dir, bgBsPos, bgUePos);

    //compute attenuation (PATHLOSS + SHADOWING)
    recvPower -= attenuation; // (dBm-dB)=dBm

    //add antenna gain
    recvPower += antennaGainTx; // (dBm+dB)=dBm
    recvPower += antennaGainRx; // (dBm+dB)=dBm

    //sub cable loss
    recvPower -= cableLoss_; // (dBm-dB)=dBm


    //=============== ANGULAR ATTENUATION =================
    if (dir == DL && bgScheduler->getTxDirection() == ANISOTROPIC) {

        // get tx angle
        double txAngle = bgScheduler->getTxAngle();

        // compute the angle between uePosition and reference axis, considering the Bs as center
        double ueAngle = computeAngle(bgBsPos, bgUePos);

        // compute the reception angle between ue and eNb
        double recvAngle = fabs(txAngle - ueAngle);

        if (recvAngle > 180)
            recvAngle = 360 - recvAngle;

        double verticalAngle = computeVerticalAngle(bgBsPos, bgUePos);

        // compute attenuation due to sectorial tx
        double angularAtt = computeAngularAttenuation(recvAngle, verticalAngle);

        recvPower -= angularAtt;
    }
    //=============== END ANGULAR ATTENUATION =================

    //===================== SINR COMPUTATION ========================
    std::vector<double> snrVector;
    snrVector.resize(numBands, recvPower);

    double speed = computeSpeed(bgUeId, bgUePos);

    // compute and add interference due to fading
    // Apply fading for each band
    double fadingAttenuation = 0;

    // for each logical band
    // FIXME compute fading only for used RBs
    for (unsigned int i = 0; i < numBands; i++) {
        fadingAttenuation = 0;
        //if fading is enabled
        if (fading_) {
            //Appling fading
            if (fadingType_ == RAYLEIGH)
                fadingAttenuation = rayleighFading(bgUeId, i);

            else if (fadingType_ == JAKES)
                fadingAttenuation = jakesFading(bgUeId, speed, i, numBands);
        }
        // add fading contribution to the received pwr
        double finalRecvPower = recvPower + fadingAttenuation; // (dBm+dB)=dBm

        snrVector[i] = finalRecvPower;
    }

    //============ MULTI CELL INTERFERENCE COMPUTATION =================
    // for background UEs, we only compute CQI
    RbMap rbmap;
    //vector containing the sum of multicell interference for each band
    std::vector<double> multiCellInterference; // Linear value (mW)
    // prepare data structure
    multiCellInterference.resize(numBands, 0);
    if (enableDownlinkInterference_ && dir == DL) {
        computeDownlinkInterference(bgUeId, bgUePos, carrierFrequency_, rbmap, numBands, &multiCellInterference);
    }
    else if (enableUplinkInterference_ && dir == UL) {
        computeUplinkInterference(bgUeId, bgBsPos, carrierFrequency_, rbmap, numBands, &multiCellInterference);
    }

    //============ BACKGROUND CELLS INTERFERENCE COMPUTATION =================
    //vector containing the sum of bg-cell interference for each band
    std::vector<double> bgCellInterference; // Linear value (mW)
    // prepare data structure
    bgCellInterference.resize(numBands, 0);
    if (enableBackgroundCellInterference_) {
        computeBackgroundCellInterference(bgUeId, bgUePos, bgBsId, bgBsPos, carrierFrequency_, rbmap, dir, numBands, &bgCellInterference); // dBm
    }

    // compute and linearize total noise
    double totN = dBmToLinear(thermalNoise_ + noiseFigure);

    for (unsigned int i = 0; i < numBands; i++) {
        // denominator expressed in dBm as (N+extCell+multiCell)
        //               (      mW                 +          mW           +  mW  )
        double den = linearToDBm(multiCellInterference[i] + bgCellInterference[i] + totN);

        // compute final SINR
        snrVector[i] -= den;
    }
    return snrVector;
}

double BackgroundCellChannelModel::getAttenuation(MacNodeId nodeId, Direction dir, inet::Coord bgBsCoord, inet::Coord bgUeCoord)
{
    //COMPUTE DISTANCE between ue and bs
    double sqrDistance = bgBsCoord.distance(bgUeCoord);

    double speed = computeSpeed(nodeId, bgUeCoord);
    double correlationDist = computeCorrelationDistance(nodeId, bgUeCoord);

    // If euclidean distance since last Los probabilty computation is greater than
    // correlation distance UE could have changed its state and
    // its visibility from eNodeb, hence it is correct to recompute the los probability
    if (correlationDist > correlationDistance_
        || losMap_.find(nodeId) == losMap_.end())
    {
        computeLosProbability(sqrDistance, nodeId);
    }

    //compute attenuation based on selected scenario and based on LOS or NLOS
    bool los = losMap_[nodeId];
    double dbp = 0;
    double attenuation = computePathLoss(sqrDistance, dbp, los);

    // TODO compute shadowing based on speed

    //    Applying shadowing only if it is enabled by configuration
    //    log-normal shadowing
    if (shadowing_)
        attenuation += computeShadowing(sqrDistance, nodeId, speed);

    EV << "BackgroundCellChannelModel::getAttenuation - computed attenuation at distance " << sqrDistance << " is " << attenuation << endl;

    return attenuation;
}

void BackgroundCellChannelModel::updatePositionHistory(const MacNodeId nodeId, const inet::Coord coord)
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

void BackgroundCellChannelModel::updateCorrelationDistance(const MacNodeId nodeId, const inet::Coord coord)
{

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

double BackgroundCellChannelModel::computeCorrelationDistance(const MacNodeId nodeId, const inet::Coord coord)
{
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

double BackgroundCellChannelModel::computeSpeed(const MacNodeId nodeId, const inet::Coord coord)
{
    double speed = 0.0;

    if (positionHistory_.find(nodeId) == positionHistory_.end()) {
        // no entries
        return speed;
    }
    else {
        //compute distance traveled from last update by UE (eNodeB position is fixed)

        if (positionHistory_[nodeId].size() == 1) {
            //  the only element refers to present , return 0
            return speed;
        }

        double movement = positionHistory_[nodeId].front().second.distance(coord);

        if (movement <= 0.0)
            return speed;
        else {
            double time = (NOW.dbl()) - (positionHistory_[nodeId].front().first.dbl());
            if (time <= 0.0) // time not updated since last speed call
                throw cRuntimeError("Multiple entries detected in position history referring to same time");
            // compute speed
            speed = (movement) / (time);
        }
    }
    return speed;
}

void BackgroundCellChannelModel::computeLosProbability(double d, MacNodeId nodeId)
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

double BackgroundCellChannelModel::computePathLoss(double distance, double dbp, bool los)
{
    //compute attenuation based on selected scenario and based on LOS or NLOS
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

double BackgroundCellChannelModel::computeIndoor(double d, bool los)
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

double BackgroundCellChannelModel::computeUrbanMicro(double d, bool los)
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
                throw cRuntimeError("Error LOS urban microcell path loss model is valid for d<5000 m");
        }
        if (d < dbp)
            return 22 * log10(d) + 28 + 20 * log10(carrierFrequency_);
        else
            return 40 * log10(d) + 7.8 - 18 * log10(hNodeB_ - 1)
                   - 18 * log10(hUe_ - 1) + 2 * log10(carrierFrequency_);
    }
    // NLOS situation
    if (d < 10)
        throw cRuntimeError("Error NLOS urban microcell path loss model is valid for 10m < d ");
    if (d > 5000) {
        if (tolerateMaxDistViolation_)
            return ATT_MAXDISTVIOLATED;
        else
            throw cRuntimeError("Error NLOS urban microcell path loss model is valid for d <2000 m");
    }
    return 36.7 * log10(d) + 22.7 + 26 * log10(carrierFrequency_);
}

double BackgroundCellChannelModel::computeUrbanMacro(double d, bool los)
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
                throw cRuntimeError("Error LOS urban macrocell path loss model is valid for d<5000 m");
        }
        if (d < dbp)
            return 22 * log10(d) + 28 + 20 * log10(carrierFrequency_);
        else
            return 40 * log10(d) + 7.8 - 18 * log10(hNodeB_ - 1)
                   - 18 * log10(hUe_ - 1) + 2 * log10(carrierFrequency_);
    }

    if (d < 10)
        throw cRuntimeError("Error NLOS urban macrocell path loss model is valid for 10m < d ");
    if (d > 5000) {
        if (tolerateMaxDistViolation_)
            return ATT_MAXDISTVIOLATED;
        else
            throw cRuntimeError("Error NLOS urban macrocell path loss model is valid for d <5000 m");
    }

    double att = 161.04 - 7.1 * log10(wStreet_) + 7.5 * log10(hBuilding_)
        - (24.37 - 3.7 * pow(hBuilding_ / hNodeB_, 2)) * log10(hNodeB_)
        + (43.42 - 3.1 * log10(hNodeB_)) * (log10(d) - 3)
        + 20 * log10(carrierFrequency_)
        - (3.2 * (pow(log10(11.75 * hUe_), 2)) - 4.97);
    return att;
}

double BackgroundCellChannelModel::computeSubUrbanMacro(double d, double& dbp, bool los)
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
                throw cRuntimeError("Error LOS suburban macrocell path loss model is valid for d<5000 m");
        }
        double a1 = (0.03 * pow(hBuilding_, 1.72));
        double b1 = 0.044 * pow(hBuilding_, 1.72);
        double a = (a1 < 10) ? a1 : 10;
        double b = (b1 < 14.72) ? b1 : 14.72;
        if (d < dbp) {
            double primo = 20 * log10((40 * M_PI * d * carrierFrequency_) / 3);
            double secondo = a * log10(d);
            double quarto = 0.002 * log10(hBuilding_) * d;
            return primo + secondo - b + quarto;
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
            throw cRuntimeError("Error NLOS suburban macrocell path loss model is valid for 10 < d < 5000 m");
    }
    double att = 161.04 - 7.1 * log10(wStreet_) + 7.5 * log10(hBuilding_)
        - (24.37 - 3.7 * pow(hBuilding_ / hNodeB_, 2)) * log10(hNodeB_)
        + (43.42 - 3.1 * log10(hNodeB_)) * (log10(d) - 3)
        + 20 * log10(carrierFrequency_)
        - (3.2 * (pow(log10(11.75 * hUe_), 2)) - 4.97);
    return att;
}

double BackgroundCellChannelModel::computeRuralMacro(double d, double& dbp, bool los)
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
                throw cRuntimeError("Error LOS rural macrocell path loss model is valid for d < 10000 m");
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
            throw cRuntimeError("Error NLOS rural macrocell path loss model is valid for d<5000 m");
    }

    double att = 161.04 - 7.1 * log10(wStreet_) + 7.5 * log10(hBuilding_)
        - (24.37 - 3.7 * pow(hBuilding_ / hNodeB_, 2)) * log10(hNodeB_)
        + (43.42 - 3.1 * log10(hNodeB_)) * (log10(d) - 3)
        + 20 * log10(carrierFrequency_)
        - (3.2 * (pow(log10(11.75 * hUe_), 2)) - 4.97);
    return att;
}

double BackgroundCellChannelModel::computeShadowing(double sqrDistance, MacNodeId nodeId, double speed)
{
    double mean = 0;
    double dbp = 0.0;
    //Get std deviation according to los/nlos and selected scenario

    double stdDev = getStdDev(sqrDistance < dbp, nodeId);
    double time = 0;
    double space = 0;
    double att;

    // if direction is DOWNLINK it means that this module is located in UE stack than
    // the Move object associated to the UE is myMove_ varible
    // if direction is UPLINK it means that this module is located in UE stack than
    // the Move object associated to the UE is move varible

    // if shadowing for current user has never been computed
    if (lastComputedSF_.find(nodeId) == lastComputedSF_.end()) {
        //Get the log normal shadowing with std deviation stdDev
        att = normal(mean, stdDev);

        //store the shadowing attenuation for this user and the temporal mark
        std::pair<simtime_t, double> tmp(NOW, att);
        lastComputedSF_[nodeId] = tmp;

        //If the shadowing attenuation has been computed at least one time for this user
        // and the distance traveled by the UE is greated than correlation distance
    }
    else if ((NOW - lastComputedSF_.at(nodeId).first).dbl() * speed
             > correlationDistance_)
    {

        //get the temporal mark of the last computed shadowing attenuation
        time = (NOW - lastComputedSF_.at(nodeId).first).dbl();

        //compute the traveled distance
        space = time * speed;

        //Compute shadowing with a EAW (Exponential Average Window) (step1)
        double a = exp(-0.5 * (space / correlationDistance_));

        //Get last shadowing attenuation computed
        double old = lastComputedSF_.at(nodeId).second;

        //Compute shadowing with a EAW (Exponential Average Window) (step2)
        att = a * old + sqrt(1 - pow(a, 2)) * normal(mean, stdDev);

        // Store the new computed shadowing
        std::pair<simtime_t, double> tmp(NOW, att);
        lastComputedSF_[nodeId] = tmp;

        // if the distance traveled by the UE is smaller than correlation distance shadowing attenuation remain the same
    }
    else {
        att = lastComputedSF_.at(nodeId).second;
    }
    return att;
}

double BackgroundCellChannelModel::getStdDev(bool dist, MacNodeId nodeId)
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

double BackgroundCellChannelModel::computeAngle(inet::Coord center, inet::Coord point) {
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

double BackgroundCellChannelModel::computeVerticalAngle(inet::Coord center, inet::Coord point)
{
    double threeDimDistance = center.distance(point);
    double twoDimDistance = getTwoDimDistance(center, point);
    double arccos = acos(twoDimDistance / threeDimDistance) * 180.0 / M_PI;
    return 90 + arccos;
}

double BackgroundCellChannelModel::getTwoDimDistance(inet::Coord a, inet::Coord b)
{
    a.z = 0.0;
    b.z = 0.0;
    return a.distance(b);
}

double BackgroundCellChannelModel::computeAngularAttenuation(double hAngle, double vAngle)
{
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

double BackgroundCellChannelModel::rayleighFading(MacNodeId id, unsigned int band)
{
    //get raylegh variable from trace file
    double temp1 = binder_->phyPisaData.getChannel(getCellInfo(binder_, id)->getLambda(id)->channelIndex + band);
    return linearToDb(temp1);
}

double BackgroundCellChannelModel::jakesFading(MacNodeId nodeId, double speed, unsigned int band, unsigned int numBands)
{
    JakesFadingMap *actualJakesMap = &jakesFadingMap_;

    //if this is the first time that we compute fading for current user
    if (actualJakesMap->find(nodeId) == actualJakesMap->end()) {
        //clear the map
        // FIXME: possible memory leak
        (*actualJakesMap)[nodeId].clear();

        //for each band we are going to create a jakes fading
        for (unsigned int j = 0; j < numBands; j++) {
            //clear some structure
            JakesFadingData temp;
            temp.angleOfArrival.clear();
            temp.delaySpread.clear();

            //for each fading path
            for (int i = 0; i < fadingPaths_; i++) {
                //get angle of arrivals
                temp.angleOfArrival.push_back(cos(uniform(0, M_PI)));

                //get delay spread
                temp.delaySpread.push_back(exponential(delayRMS_));
            }
            //store the jakes fadint for this user
            (*actualJakesMap)[nodeId].push_back(temp);
        }
    }
    // convert carrier frequency from GHz to Hz
    double f = carrierFrequency_ * 1000000000;

    //get transmission time start (TTI =1ms)
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
        // Since we are interested in attenuation a:=1, attenuation per path is then:
        double attenuation = (1.00 / sqrt(static_cast<double>(fadingPaths_)));

        // Convert to cartesian form and aggregate {Re, Im} over all fading paths.
        re_h = re_h + attenuation * cos(phi);
        im_h = im_h - attenuation * sin(phi);
    }

    // Output: |H_f|^2 = absolute channel impulse response due to fading.
    // Note that this may be >1 due to constructive interference.
    return linearToDb(re_h * re_h + im_h * im_h);
}

double BackgroundCellChannelModel::getReceivedPower_bgUe(double txPower, inet::Coord txPos, inet::Coord rxPos, Direction dir, bool losStatus, const BackgroundScheduler *bgScheduler)
{
    double antennaGainTx = 0.0;
    double antennaGainRx = 0.0;

    EV << NOW << " BackgroundCellChannelModel::getReceivedPower_bgUe" << endl;

    //===================== PARAMETERS SETUP ============================
    if (dir == DL) {
        antennaGainTx = antennaGainEnB_; //dB
        antennaGainRx = antennaGainUe_;  //dB
    }
    else { // if( dir == UL )
        antennaGainTx = antennaGainUe_;
        antennaGainRx = antennaGainEnB_;
    }

    EV << "BackgroundCellChannelModel::getReceivedPower_bgUe - DIR=" << ((dir == DL) ? "DL" : "UL")
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

    //=============== ANGULAR ATTENUATION =================
    if (dir == DL && bgScheduler->getTxDirection() == ANISOTROPIC) {
        // get tx angle
        double txAngle = bgScheduler->getTxAngle();

        // compute the angle between uePosition and reference axis, considering the Bs as center
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
    //=============== END ANGULAR ATTENUATION =================

    //============ END PATH LOSS + ANGULAR ATTENUATION ===============

    return recvPower;
}

bool BackgroundCellChannelModel::computeDownlinkInterference(MacNodeId bgUeId, inet::Coord bgUePos, double carrierFrequency, const RbMap& rbmap, unsigned int numBands,
        std::vector<double> *interference)
{
    EV << "**** Downlink Interference ****" << endl;

    // reference to the mac/phy/channel of each cell

    int temp;
    double att;

    double txPwr;

    std::vector<EnbInfo *> *enbList = binder_->getEnbList();

    for (auto enb : *enbList) {
        MacNodeId id = enb->id;

        // initialize eNb data structures
        if (!enb->init) {
            // obtain a reference to enb phy and obtain tx power
            enb->phy = check_and_cast<LtePhyBase *>(getSimulation()->getModule(binder_->getOmnetId(id))->getSubmodule("cellularNic")->getSubmodule("phy"));

            enb->txPwr = enb->phy->getTxPwr();//dBm

            // get tx direction
            enb->txDirection = enb->phy->getTxDirection();

            // get tx angle
            enb->txAngle = enb->phy->getTxAngle();

            //get reference to mac layer
            enb->mac = check_and_cast<LteMacEnb *>(getMacByMacNodeId(binder_, id));

            enb->init = true;
        }

        Coord bsPos = enb->phy->getCoord();

        LteRealisticChannelModel *interfChanModel = dynamic_cast<LteRealisticChannelModel *>(enb->phy->getChannelModel(carrierFrequency));

        // if the interfering BS does not use the selected carrier frequency, skip it
        if (interfChanModel == nullptr) {
            continue;
        }

        att = getAttenuation(bgUeId, DL, bsPos, bgUePos);

        EV << "BsId [" << id << "] - attenuation [" << att << "]";

        //=============== ANGULAR ATTENUATION =================
        double angularAtt = 0;
        if (enb->txDirection == ANISOTROPIC) {
            //get tx angle
            double txAngle = enb->txAngle;

            // compute the angle between uePosition and reference axis, considering the eNb as center
            double ueAngle = computeAngle(bsPos, bgUePos);

            // compute the reception angle between ue and eNb
            double recvAngle = fabs(txAngle - ueAngle);
            if (recvAngle > 180)
                recvAngle = 360 - recvAngle;

            double verticalAngle = computeVerticalAngle(bsPos, bgUePos);

            // compute attenuation due to sectorial tx
            angularAtt = computeAngularAttenuation(recvAngle, verticalAngle);

            EV << "angular attenuation [" << angularAtt << "]";
        }
        // else, antenna is omni-directional
        //=============== END ANGULAR ATTENUATION =================

        txPwr = enb->txPwr - angularAtt - cableLoss_ + antennaGainEnB_ + antennaGainUe_;

        numBands = std::min(numBands, interfChanModel->getNumBands());
        for (unsigned int i = 0; i < numBands; i++) {
            // compute the number of occupied slot (unnecessary)
            temp = enb->mac->getDlBandStatus(i);
            if (temp != 0)
                (*interference)[i] += dBmToLinear(txPwr - att); //(dBm-dB)=dBm

            EV << "\t band " << i << " occupied " << temp << "/pwr[" << txPwr << "]-int[" << (*interference)[i] << "]" << endl;
        }
    }

    return true;
}

bool BackgroundCellChannelModel::computeUplinkInterference(MacNodeId bgUeId, inet::Coord bgBsPos, double carrierFrequency, const RbMap& rbmap, unsigned int numBands,
        std::vector<double> *interference)
{
    EV << "**** Uplink Interference ****" << endl;

    const std::vector<std::vector<UeAllocationInfo>> *ulTransmissionMap = binder_->getUlTransmissionMap(carrierFrequency, CURR_TTI);
    if (ulTransmissionMap != nullptr && !ulTransmissionMap->empty()) {
        for (unsigned int i = 0; i < numBands; i++) {
            // get the set of UEs transmitting on the same band
            const std::vector<UeAllocationInfo>& allocatedUes = ulTransmissionMap->at(i);
            for (const auto& ueInfo : allocatedUes) {
                MacNodeId ueId = ueInfo.nodeId;
                // MacCellId cellId = ueInfo.cellId;
                Direction dir = ueInfo.dir;
                double txPwr;
                inet::Coord ueCoord;
                LtePhyUe *uePhy = nullptr;
                TrafficGeneratorBase *trafficGen = nullptr;
                if (ueInfo.phy != nullptr) {
                    uePhy = check_and_cast<LtePhyUe *>(ueInfo.phy);
                    txPwr = uePhy->getTxPwr(dir);
                    ueCoord = uePhy->getCoord();
                }
                else { // this is a backgroundUe
                    trafficGen = check_and_cast<TrafficGeneratorBase *>(ueInfo.trafficGen);
                    txPwr = trafficGen->getTxPwr();
                    ueCoord = trafficGen->getCoord();
                }

                EV << NOW << " BackgroundCellChannelModel::computeUplinkInterference - Interference from UE: " << ueId << "(dir " << dirToA(dir) << ") on band[" << i << "]" << endl;

                // get rx power and attenuation from this UE
                double rxPwr = txPwr - cableLoss_ + antennaGainUe_ + antennaGainEnB_;
                double att = getAttenuation(ueId, UL, bgBsPos, ueCoord);
                (*interference)[i] += dBmToLinear(rxPwr - att);//(dBm-dB)=dBm

                EV << "\t band " << i << "/pwr[" << rxPwr - att << "]-int[" << (*interference)[i] << "]" << endl;
            }
        }
    }

    // Debug Output
    EV << NOW << " BackgroundCellChannelModel::computeUplinkInterference - Final Band Interference Status: " << endl;
    for (unsigned int i = 0; i < numBands; i++)
        EV << "\t band " << i << " int[" << (*interference)[i] << "]" << endl;

    return true;
}

bool BackgroundCellChannelModel::computeBackgroundCellInterference(MacNodeId bgUeId, inet::Coord bgUeCoord, int bgBsId, inet::Coord bgBsCoord, double carrierFrequency, const RbMap& rbmap, Direction dir,
        unsigned int numBands, std::vector<double> *interference)
{
    EV << "**** Background Cell Interference **** " << endl;

    // get external cell list
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
        // skip interference from serving Bg Bs
        if ((*it)->getId() == bgBsId) {
            it++;
            continue;
        }

        if (dir == DL) {
            // compute interference with respect to the background base station

            // get external cell position
            c = (*it)->getPosition();

            // computer distance between UE and the ext cell
            dist = bgUeCoord.distance(c);

            EV << "\t distance between BgUe[" << bgUeCoord.x << "," << bgUeCoord.y <<
                "] and backgroundCell[" << c.x << "," << c.y << "] is -> "
               << dist << "\t";

            // compute attenuation according to some path loss model
            bool los = false;
            double dbp = 0;
            att = computePathLoss(dist, dbp, los);

            txPwr = (*it)->getTxPower();

            //=============== ANGULAR ATTENUATION =================
            if ((*it)->getTxDirection() == OMNI) {
                angularAtt = 0;
            }
            else {
                // compute the angle between uePosition and reference axis, considering the eNb as center
                double ueAngle = computeAngle(c, bgUeCoord);

                // compute the reception angle between ue and eNb
                double recvAngle = fabs((*it)->getTxAngle() - ueAngle);

                if (recvAngle > 180)
                    recvAngle = 360 - recvAngle;

                double verticalAngle = computeVerticalAngle(c, bgUeCoord);

                // compute attenuation due to sectorial tx
                angularAtt = computeAngularAttenuation(recvAngle, verticalAngle);
            }
            //=============== END ANGULAR ATTENUATION =================

            // TODO do we need to use (- cableLoss_ + antennaGainEnB_) in ext cells too?
            // compute and linearize received power
            recvPwrDBm = txPwr - att - angularAtt - cableLoss_ + antennaGainEnB_ + antennaGainUe_;
            recvPwr = dBmToLinear(recvPwrDBm);

            numBands = std::min(numBands, (*it)->getNumBands());

            // add interference in those bands where the ext cell is active
            for (unsigned int i = 0; i < numBands; i++) {
                int occ = 0;
                occ = (*it)->getBandStatus(i, DL);

                // if the ext cell is active, add interference
                if (occ > 0) {
                    (*interference)[i] += recvPwr;
                }
            }
        }
        else { // dir == UL
            // for each RB occupied in the background cell, compute interference with respect to the
            // background UE that is using that RB
            TrafficGeneratorBase *bgUe;

            double antennaGainBgUe = antennaGainUe_;  // TODO get this from the bgUe

            angularAtt = 0;  // we assume OMNI directional UEs

            numBands = std::min(numBands, (*it)->getNumBands());

            // add interference in those bands where a UE in the background cell is active
            for (unsigned int i = 0; i < numBands; i++) {
                int occ = 0;

                occ = (*it)->getBandStatus(i, UL);
                if (occ)
                    bgUe = (*it)->getBandInterferingUe(i);

                // if the ext cell is active, add interference
                if (occ) {
                    txPwr = bgUe->getTxPwr();
                    c = bgUe->getCoord();
                    dist = bgBsCoord.distance(c);

                    EV << "\t distance between BgBS[" << bgBsCoord.x << "," << bgBsCoord.y <<
                        "] and backgroundUE[" << c.x << "," << c.y << "] is -> "
                       << dist << "\t";

                    // compute attenuation according to some path loss model
                    bool los = false;
                    double dbp = 0;
                    att = computePathLoss(dist, dbp, los);

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

} //namespace

