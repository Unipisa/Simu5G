//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/background/cell/BackgroundCellChannelModel.h"
#include "simu5g/background/cell/BackgroundScheduler.h"
#include "simu5g/stack/phy/PhyBase.h"
#include "simu5g/stack/phy/PhyUe.h"
#include "simu5g/stack/phy/channelmodel/StochasticChannelModel.h"
#include "simu5g/stack/phy/channelmodel/Tr36814PathLossModel.h"

namespace simu5g {

Define_Module(BackgroundCellChannelModel);

BackgroundCellChannelModel::~BackgroundCellChannelModel()
{
    delete pathLoss_;
}

void BackgroundCellChannelModel::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        scenario_ = aToDeploymentScenario(par("scenario").stringValue());
        hNodeB_ = par("nodebHeight");
        hBuilding_ = par("buildingHeight");
        inside_building_ = par("insideBuilding");
        if (inside_building_)
            inside_distance_ = uniform(0.0, 25.0);
        tolerateMaxDistViolation_ = par("tolerateMaxDistViolation");
        hUe_ = par("ueHeight");

        wStreet_ = par("streetWidth");

        antennaGainUe_ = par("antennaGainUe");
        antennaGainEnB_ = par("antennGainEnB");
        antennaGainMicro_ = par("antennGainMicro");
        thermalNoise_ = par("thermalNoise");
        cableLoss_ = par("cableLoss");
        ueNoiseFigure_ = par("ueNoiseFigure");
        bsNoiseFigure_ = par("bsNoiseFigure");
        shadowing_ = par("shadowing");
        correlationDistance_ = par("correlationDistance");

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
        delayRMS_ = par("delayRms");

        enableBackgroundCellInterference_ = par("bgCellInterference");
        enableDownlinkInterference_ = par("downlinkInterference");
        enableUplinkInterference_ = par("uplinkInterference");

        //get binder
        binder_.reference(this, "binderModule", true);

        WATCH(lastComputedSF_);
        WATCH(losMap_);
        WATCH(lastCorrelationPoint_);
        WATCH(jakesFadingMap_);
    }
    else if (stage == INITSTAGE_SIMU5G_AMC_SETUP) {
        // carrierFrequencyHz_/GHz_/log10CarrierFrequencyGHz_ are set by
        // setCarrierFrequency(), called by the owning BackgroundScheduler at
        // INITSTAGE_SIMU5G_BINDER_ACCESS, which has completed for all
        // modules by this later stage
        pathLoss_ = new Tr36814PathLossModel();
        pathLoss_->initialize(this, scenario_, hNodeB_, hUe_, hBuilding_, wStreet_,
                inside_building_, inside_distance_,
                carrierFrequencyHz_, carrierFrequencyGHz_, log10CarrierFrequencyGHz_,
                tolerateMaxDistViolation_);
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
    if (!dynamicLos_) {
        losMap_[nodeId] = fixedLos_;
        return;
    }
    double p = pathLoss_->computeLosProbability(d, d);
    double random = uniform(0.0, 1.0);
    if (random <= p)
        losMap_[nodeId] = true;
    else
        losMap_[nodeId] = false;
}

double BackgroundCellChannelModel::computePathLoss(double distance, double dbp, bool los)
{
    return pathLoss_->computePathLoss(distance, distance, los);
}

double BackgroundCellChannelModel::computeShadowing(double sqrDistance, MacNodeId nodeId, double speed)
{
    double mean = 0;

    // Get std deviation according to los/nlos and selected scenario
    double stdDev = pathLoss_->getShadowingStdDev(sqrDistance, sqrDistance, losMap_[nodeId]);
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
    else if (relx >= 0 && rely < 0) // quadrant IV, and the negative y axis
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
    const int channelIndex = 0;
    double temp1 = binder_->phyPisaData.getChannel(channelIndex + band);
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
    //get transmission time start (TTI =1ms)
    simtime_t t = simTime().dbl() - 0.001;

    double re_h = 0;
    double im_h = 0;

    const JakesFadingData& actualJakesData = actualJakesMap->at(nodeId).at(band);

    // Compute Doppler shift.
    double doppler_shift = (speed * carrierFrequencyHz_) / SPEED_OF_LIGHT;

    for (int i = 0; i < fadingPaths_; i++) {
        // Phase shift due to Doppler => t-selectivity.
        double phi_d = actualJakesData.angleOfArrival[i] * doppler_shift;

        // Phase shift due to delay spread => f-selectivity.
        double phi_i = actualJakesData.delaySpread[i].dbl() * carrierFrequencyHz_;

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

bool BackgroundCellChannelModel::computeDownlinkInterference(MacNodeId bgUeId, inet::Coord bgUePos, GHz carrierFrequency, const RbMap& rbmap, unsigned int numBands,
        std::vector<double> *interference)
{
    EV << "**** Downlink Interference ****" << endl;

    // reference to the mac/phy/channel of each cell

    int temp;
    double att;

    double txPwr;

    for (const auto& enb : binder_->getEnbList()) {
        MacNodeId id = enb->id;

        // initialize eNb data structures
        if (!enb->init) {
            // obtain a reference to enb phy and obtain tx power
            enb->phy = binder_->getPhy(id);

            enb->txPwr = enb->phy->getTxPwr();//dBm

            // get tx direction
            enb->txDirection = enb->phy->getTxDirection();

            // get tx angle
            enb->txAngle = enb->phy->getTxAngle();

            //get reference to mac layer
            enb->mac = check_and_cast<LteMacEnb *>(binder_->getMacByNodeId(id));

            enb->init = true;
        }

        Coord bsPos = enb->phy->getCoord();

        StochasticChannelModel *interfChanModel = dynamic_cast<StochasticChannelModel *>(enb->phy->getChannelModel(carrierFrequency));

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

bool BackgroundCellChannelModel::computeUplinkInterference(MacNodeId bgUeId, inet::Coord bgBsPos, GHz carrierFrequency, const RbMap& rbmap, unsigned int numBands,
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
                PhyUe *uePhy = nullptr;
                TrafficGeneratorBase *trafficGen = nullptr;
                if (ueInfo.phy != nullptr) {
                    uePhy = check_and_cast<PhyUe *>(ueInfo.phy);
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

bool BackgroundCellChannelModel::computeBackgroundCellInterference(MacNodeId bgUeId, inet::Coord bgUeCoord, int bgBsId, inet::Coord bgBsCoord, GHz carrierFrequency, const RbMap& rbmap, Direction dir,
        unsigned int numBands, std::vector<double> *interference)
{
    EV << "**** Background Cell Interference **** " << endl;

    // get external cell list
    const BackgroundSchedulerList& list = binder_->getBackgroundSchedulerList(carrierFrequency);

    double dist, // meters
           txPwr, // dBm
           recvPwr, // watt
           recvPwrDBm, // dBm
           att, // dBm
           angularAtt; // dBm

    //compute distance for each cell
    for (auto& bgScheduler : list) {
        // skip interference from serving Bg Bs
        if (bgScheduler->getId() == bgBsId)
            continue;

        if (dir == DL) {
            // compute interference with respect to the background base station

            // get external cell position
            Coord c = bgScheduler->getPosition();

            // computer distance between UE and the ext cell
            dist = bgUeCoord.distance(c);

            EV << "\t distance between BgUe[" << bgUeCoord.x << "," << bgUeCoord.y <<
                "] and backgroundCell[" << c.x << "," << c.y << "] is -> "
               << dist << "\t";

            // compute attenuation according to some path loss model
            bool los = false;
            double dbp = 0;
            att = computePathLoss(dist, dbp, los);

            txPwr = bgScheduler->getTxPower();

            //=============== ANGULAR ATTENUATION =================
            if (bgScheduler->getTxDirection() == OMNI) {
                angularAtt = 0;
            }
            else {
                // compute the angle between uePosition and reference axis, considering the eNb as center
                double ueAngle = computeAngle(c, bgUeCoord);

                // compute the reception angle between ue and eNb
                double recvAngle = fabs(bgScheduler->getTxAngle() - ueAngle);

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

            numBands = std::min(numBands, bgScheduler->getNumBands());

            // add interference in those bands where the ext cell is active
            for (unsigned int i = 0; i < numBands; i++) {
                int occ = 0;
                occ = bgScheduler->getBandStatus(i, DL);

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

            numBands = std::min(numBands, bgScheduler->getNumBands());

            // add interference in those bands where a UE in the background cell is active
            for (unsigned int i = 0; i < numBands; i++) {
                int occ = 0;

                occ = bgScheduler->getBandStatus(i, UL);
                if (occ)
                    bgUe = bgScheduler->getBandInterferingUe(i);

                // if the ext cell is active, add interference
                if (occ) {
                    txPwr = bgUe->getTxPwr();
                    Coord c = bgUe->getCoord();
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
    }

    return true;
}

} //namespace
