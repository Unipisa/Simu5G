//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "nodes/backgroundCell/BackgroundCellChannelModel.h"
#include "nodes/backgroundCell/BackgroundBaseStation.h"

Define_Module(BackgroundCellChannelModel);

void BackgroundCellChannelModel::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL)
    {
        scenario_ = aToDeploymentScenario(par("scenario").stringValue());
        hNodeB_ = par("nodeb_height");
        hBuilding_ = par("building_height");
        inside_building_ = par("inside_building");
        if (inside_building_)
            inside_distance_ = uniform(0.0,25.0);
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
        if (fType.compare("JAKES") == 0)
            fadingType_ = JAKES;
        else if (fType.compare("RAYLEIGH") == 0)
            fadingType_ = RAYLEIGH;
        else
            fadingType_ = JAKES;

        fadingPaths_ = par("fading_paths");
        delayRMS_ = par("delay_rms");

        //get binder
        binder_ = getBinder();
    }

}

std::vector<double> BackgroundCellChannelModel::getSINR(MacNodeId bgUeId, inet::Coord bgUePos, TrafficGeneratorBase* bgUe, BackgroundBaseStation* bgBaseStation, Direction dir)
{
    unsigned int numBands = bgBaseStation->getNumBands();
    inet::Coord bgBsPos = bgBaseStation->getPosition();

    //get tx power
    double recvPower = (dir == DL) ? bgBaseStation->getTxPower() : bgUe->getTxPwr(); // dBm

    double antennaGainTx = 0.0;
    double antennaGainRx = 0.0;
    double noiseFigure = 0.0;

    if (dir == DL)
    {
        //set noise Figure
        noiseFigure = ueNoiseFigure_; //dB
        //set antenna gain Figure
        antennaGainTx = antennaGainEnB_; //dB
        antennaGainRx = antennaGainUe_;  //dB
    }
    else // if( dir == UL )
    {
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

//    std::cout << "BackgroundCellChannelModel::getSinr - attenuation " << attenuation << " antennaGainTx-Rx " << antennaGainTx << " " << antennaGainRx << " cableLoss " << cableLoss_ << endl;

    //=============== ANGOLAR ATTENUATION =================
    if (dir == DL && bgBaseStation->getTxDirection() == ANISOTROPIC)
    {

        // get tx angle
        double txAngle = bgBaseStation->getTxAngle();

        // compute the angle between uePosition and reference axis, considering the Bs as center
        double ueAngle = computeAngle(bgBsPos, bgUePos);

        // compute the reception angle between ue and eNb
        double recvAngle = fabs(txAngle - ueAngle);

        if (recvAngle > 180)
            recvAngle = 360 - recvAngle;

        double verticalAngle = computeVerticalAngle(bgBsPos, bgUePos);

        // compute attenuation due to sectorial tx
        double angolarAtt = computeAngolarAttenuation(recvAngle,verticalAngle);

        recvPower -= angolarAtt;
    }
    //=============== END ANGOLAR ATTENUATION =================



    //===================== SINR COMPUTATION ========================
    std::vector<double> snrVector;
    snrVector.resize(numBands, recvPower);

    double speed = computeSpeed(bgUeId, bgUePos);

    // compute and add interference due to fading
    // Apply fading for each band
    double fadingAttenuation = 0;

    // for each logical band
    // FIXME compute fading only for used RBs
    for (unsigned int i = 0; i < numBands; i++)
    {
        fadingAttenuation = 0;
        //if fading is enabled
        if (fading_)
        {
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

    // compute and linearize total noise
    double totN = dBmToLinear(thermalNoise_ + noiseFigure);

    // denominator expressed in dBm as (N+extCell+multiCell)
    double den = linearToDBm(totN);

    for (unsigned int i = 0; i < numBands; i++)
    {
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
   if (positionHistory_.find(nodeId) != positionHistory_.end())
   {
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

    if (lastCorrelationPoint_.find(nodeId) == lastCorrelationPoint_.end()){
        // no lastCorrelationPoint set current point.
        lastCorrelationPoint_[nodeId] = Position(NOW, coord);
    } else if ((lastCorrelationPoint_[nodeId].first != NOW) &&
                lastCorrelationPoint_[nodeId].second.distance(coord) > correlationDistance_) {
        // check simtime_t first
        lastCorrelationPoint_[nodeId] = Position(NOW, coord);
    }
}

double BackgroundCellChannelModel::computeCorrelationDistance(const MacNodeId nodeId, const inet::Coord coord)
{
    double dist = 0.0;

    if (lastCorrelationPoint_.find(nodeId) == lastCorrelationPoint_.end()){
        // no lastCorrelationPoint found. Add current position and return dist = 0.0
        lastCorrelationPoint_[nodeId] = Position(NOW, coord);
    } else {
        dist = lastCorrelationPoint_[nodeId].second.distance(coord);
    }
    return dist;
}

double BackgroundCellChannelModel::computeSpeed(const MacNodeId nodeId, const inet::Coord coord)
{
   double speed = 0.0;

   if (positionHistory_.find(nodeId) == positionHistory_.end())
   {
       // no entries
       return speed;
   }
   else
   {
       //compute distance traveled from last update by UE (eNodeB position is fixed)

       if (positionHistory_[nodeId].size() == 1)
       {
           //  the only element refers to present , return 0
           return speed;
       }

       double movement = positionHistory_[nodeId].front().second.distance(coord);

       if (movement <= 0.0)
           return speed;
       else
       {
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
   if (!dynamicLos_)
   {
       losMap_[nodeId] = fixedLos_;
       return;
   }
   switch (scenario_)
   {
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
    switch (scenario_)
    {
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
   if (los)
   {
       if (d > 150 || d < 3)
           throw cRuntimeError("Error LOS indoor path loss model is valid for 3<d<150");
       a = 16.9;
       b = 32.8;
   }
   else
   {
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
   if (los)
   {
       // LOS situation
       if (d > 5000){
           if(tolerateMaxDistViolation_)
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
   if (d > 5000){
       if(tolerateMaxDistViolation_)
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
   if (los)
   {
       if (d > 5000){
           if(tolerateMaxDistViolation_)
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
   if (d > 5000){
       if(tolerateMaxDistViolation_)
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
   if (los)
   {
       if (d > 5000) {
           if(tolerateMaxDistViolation_)
               return ATT_MAXDISTVIOLATED;
           else
               throw cRuntimeError("Error LOS suburban macrocell path loss model is valid for d<5000 m");
       }
       double a1 = (0.03 * pow(hBuilding_, 1.72));
       double b1 = 0.044 * pow(hBuilding_, 1.72);
       double a = (a1 < 10) ? a1 : 10;
       double b = (b1 < 14.72) ? b1 : 14.72;
       if (d < dbp)
       {
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
       if(tolerateMaxDistViolation_)
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
   if (los)
   {
       // LOS situation
       if (d > 10000) {
           if(tolerateMaxDistViolation_)
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
       if(tolerateMaxDistViolation_)
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
    if (lastComputedSF_.find(nodeId) == lastComputedSF_.end())
    {
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
    else
    {
        att = lastComputedSF_.at(nodeId).second;
    }
    return att;
}

double BackgroundCellChannelModel::getStdDev(bool dist, MacNodeId nodeId)
{
   switch (scenario_)
   {
   case URBAN_MICROCELL:
   case INDOOR_HOTSPOT:
       if (losMap_[nodeId])
           return 3.;
       else
           return 4.;
       break;
   case URBAN_MACROCELL:
       if (losMap_[nodeId])
           return 4.;
       else
           return 6.;
       break;
   case RURAL_MACROCELL:
   case SUBURBAN_MACROCELL:
       if (losMap_[nodeId])
       {
           if (dist)
               return 4.;
           else
               return 6.;
       }
       else
           return 8.;
       break;
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
    double arccos = acos(twoDimDistance/threeDimDistance) * 180.0 / M_PI;
    return 90 + arccos;
}

double BackgroundCellChannelModel::getTwoDimDistance(inet::Coord a, inet::Coord b)
{
    a.z = 0.0;
    b.z = 0.0;
    return a.distance(b);
}

double BackgroundCellChannelModel::computeAngolarAttenuation(double hAngle, double vAngle)
{
   // in this implementation, vertical angle is not considered

   double angolarAtt;
   double angolarAttMin = 25;
   // compute attenuation due to angolar position
   // see TR 36.814 V9.0.0 for more details
   angolarAtt = 12 * pow(hAngle / 70.0, 2);

   //  EV << "\t angolarAtt[" << angolarAtt << "]" << endl;
   // max value for angolar attenuation is 25 dB
   if (angolarAtt > angolarAttMin)
       angolarAtt = angolarAttMin;

   return angolarAtt;
}

double BackgroundCellChannelModel::rayleighFading(MacNodeId id, unsigned int band)
{
   //get raylegh variable from trace file
   double temp1 = binder_->phyPisaData.getChannel(getCellInfo(id)->getLambda(id)->channelIndex + band);
   return linearToDb(temp1);
}

double BackgroundCellChannelModel::jakesFading(MacNodeId nodeId, double speed, unsigned int band, unsigned int numBands)
{
   JakesFadingMap* actualJakesMap = &jakesFadingMap_;

   //if this is the first time that we compute fading for current user
   if (actualJakesMap->find(nodeId) == actualJakesMap->end())
   {
       //clear the map
       // FIXME: possible memory leak
       (*actualJakesMap)[nodeId].clear();

       //for each band we are going to create a jakes fading
       for (unsigned int j = 0; j < numBands; j++)
       {
           //clear some structure
           JakesFadingData temp;
           temp.angleOfArrival.clear();
           temp.delaySpread.clear();

           //for each fading path
           for (int i = 0; i < fadingPaths_; i++)
           {
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

   for (int i = 0; i < fadingPaths_; i++)
   {
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

       //        EV << "ID=" << nodeId << " - t[" << t << "] - dopplerShift[" << doppler_shift << "] - phiD[" <<
       //                phi_d << "] - phiI[" << phi_i << "] - phi[" << phi << "] - attenuation[" << attenuation << "] - f["
       //                << f << "] - Band[" << band << "] - cos(phi)["
       //                << cos(phi) << "]" << endl;
   }

   // Output: |H_f|^2 = absolute channel impulse response due to fading.
   // Note that this may be >1 due to constructive interference.
   return linearToDb(re_h * re_h + im_h * im_h);
}
