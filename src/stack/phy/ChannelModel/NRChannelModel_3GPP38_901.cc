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

#include "stack/phy/ChannelModel/NRChannelModel_3GPP38_901.h"

// attenuation value to be returned if max. distance of a scenario has been violated
// and tolerating the maximum distance violation is enabled
#define ATT_MAXDISTVIOLATED 1000

Define_Module(NRChannelModel_3GPP38_901);

void NRChannelModel_3GPP38_901::initialize(int stage)
{
    NRChannelModel::initialize(stage);
}

void NRChannelModel_3GPP38_901::computeLosProbability(double d, MacNodeId nodeId)
{
   double p = 0;
   if (!dynamicLos_)
   {
       losMap_[nodeId] = fixedLos_;
       return;
   }

   if (scenario_ == URBAN_MACROCELL)
   {
       if (d <= 18.0)
           p = 1.0;
       else
       {
           double C = (hUe_ <= 13.0) ? 0 : pow((hUe_-13.0)/10.0, 1.5);
           p = ( (18 / d) + exp(-1 * d / 63) * (1 - (18 / d)) ) * ( 1 + C * (5.0/4.0) * pow(d/100.0,3) * exp(-1 * d/150.0) );
       }

       double random = uniform(0.0, 1.0);
       if (random <= p)
           losMap_[nodeId] = true;
       else
           losMap_[nodeId] = false;
   }
   else
       NRChannelModel::computeLosProbability(d, nodeId);
}


double NRChannelModel_3GPP38_901::computePathLoss(double threeDimDistance, double twoDimDistance, bool los)
{
    //compute attenuation based on selected scenario and based on LOS or NLOS
    double pathLoss = 0;
    switch (scenario_)
    {
    case INDOOR_HOTSPOT:
        pathLoss = computeIndoor(threeDimDistance, twoDimDistance, los);
        break;
    case URBAN_MICROCELL:
        pathLoss = computeUrbanMicro(threeDimDistance, twoDimDistance, los);
        break;
    case URBAN_MACROCELL:
        pathLoss = computeUrbanMacro(threeDimDistance, twoDimDistance, los);
        break;
    case RURAL_MACROCELL:
        pathLoss = computeRuralMacro(threeDimDistance, twoDimDistance, los);
        break;
    default:
        return NRChannelModel::computePathLoss(twoDimDistance, 0.0, los);
    }
    return pathLoss;
}


double NRChannelModel_3GPP38_901::computeUrbanMacro(double threeDimDistance, double twoDimDistance, bool los)
{
   if (twoDimDistance < 10)
       twoDimDistance = 10;

   if (twoDimDistance > 5000){
       if(tolerateMaxDistViolation_)
           return ATT_MAXDISTVIOLATED;
       else
           throw cRuntimeError("Error LOS urban macrocell path loss model is valid for d<5000 m");
   }

   // compute penetration loss
   double penetrationLoss = 0.0;
   if (inside_building_)
   {
       double inside_distance = (inside_distance_ < threeDimDistance) ? inside_distance_ : threeDimDistance;
       double pLoss_in = 0.5 * inside_distance;
       double pLoss_tw = 0.0;
       if (carrierFrequency_ <= 6.0)
           pLoss_tw = 20.0;
       else
       {
           double Lglass = 2 + 0.2 * carrierFrequency_;
           double Lconcrete = 5 + 4 * carrierFrequency_;
           pLoss_tw = 5 - 10 * log10( 0.7 * pow(10,(-Lglass/10)) + 0.7 * pow(10,(-Lconcrete/10)) ) + normal(0.0,4.4);
       }
       penetrationLoss = pLoss_tw + pLoss_in;
   }

   // compute break-point distance
   double hEnvir = 0.0;
   double G_2d = (twoDimDistance < 18.0) ? 0 : (5.0/4.0) * pow( twoDimDistance/100.0 , 3) * exp(twoDimDistance/150);
   double C = (hUe_ < 13.0) ? 0 : pow( ((hUe_-13.0)/10.0) , 1.5) * G_2d;
   double prob = 1.0 / (1.0 + C);
   if (uniform(0.0,1.0) < prob)
       hEnvir = 1.0;
   else
   {
       double bound = hUe_ - 1.5;
       std::vector<double> hVec;
       for (double h=12; h<bound; h+=3)
           hVec.push_back(h);
       hVec.push_back(bound);
       hEnvir = hVec[intuniform(0, hVec.size())];
   }

   double hNodeB = hNodeB_ - hEnvir;
   double hUe = hUe_ - hEnvir;

   double dbp = 4 * hNodeB * hUe * ((carrierFrequency_ * 1000000000) / SPEED_OF_LIGHT);

   double pLoss_los = 0.0;
   if (twoDimDistance < dbp)
       pLoss_los = 28 + 22 * log10(threeDimDistance) + 20 * log10(carrierFrequency_);
   else
       pLoss_los = 28 + 40 * log10(threeDimDistance) + 20 * log10(carrierFrequency_) - 9 * log10( (dbp * dbp + (hNodeB_ - hUe_) * (hNodeB_ - hUe_) ) );

   if (los)
       return pLoss_los + penetrationLoss;

   // NLOS case (optional PL in 3GPP 38.901 v16.1.0

   double pLoss_nlos = 32.4 + 20 * log10(carrierFrequency_) + 30 * log10(threeDimDistance);
   return pLoss_nlos;

//   double pLoss_nlos = 13.54 + 39.08 * log10(threeDimDistance) + 20 * log10(carrierFrequency_) - 0.6 * (hUe_ - 1.5);
//
//   return (pLoss_los > pLoss_nlos) ? pLoss_los + penetrationLoss : pLoss_nlos + penetrationLoss;
}

double NRChannelModel_3GPP38_901::getStdDev(bool dist, MacNodeId nodeId)
{
    if (scenario_ == URBAN_MACROCELL)
    {
        if (losMap_[nodeId])
            return 4.;
        else
            return 6.;
    }
    return NRChannelModel::getStdDev(dist, nodeId);
}


double NRChannelModel_3GPP38_901::computeShadowing(double sqrDistance, MacNodeId nodeId, double speed, bool cqiDl)
{
    ShadowFadingMap* actualShadowingMap;

    if (cqiDl) // if we are computing a DL CQI we need the Shadowing Map stored on the UE side
        actualShadowingMap = obtainShadowingMap(nodeId);
    else
        actualShadowingMap = &lastComputedSF_;

    double mean = 0;
    double dbp = 0.0;
    //Get std deviation according to los/nlos and selected scenario

    double stdDev = getStdDev(sqrDistance < dbp, nodeId);
    double time = 0;
    double space = 0;
    double att;

    // if direction is DOWNLINK it means that this module is located in UE stack than
    // the Move object associated to the UE is myMove_ variable
    // if direction is UPLINK it means that this module is located in UE stack than
    // the Move object associated to the UE is move variable

    // if shadowing for current user has never been computed
    if (actualShadowingMap->find(nodeId) == actualShadowingMap->end())
    {
        //Get the log normal shadowing with std deviation stdDev
        att = normal(mean, stdDev);

        //store the shadowing attenuation for this user and the temporal mark
        std::pair<simtime_t, double> tmp(NOW, att);
        (*actualShadowingMap)[nodeId] = tmp;

        //If the shadowing attenuation has been computed at least one time for this user
        // and the distance traveled by the UE is greated than correlation distance
    }
    else if ((NOW - actualShadowingMap->at(nodeId).first).dbl() * speed
            > correlationDistance_)
    {

        //get the temporal mark of the last computed shadowing attenuation
        time = (NOW - actualShadowingMap->at(nodeId).first).dbl();

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
        (*actualShadowingMap)[nodeId] = tmp;

        // if the distance traveled by the UE is smaller than correlation distance shadowing attenuation remain the same
    }
    else
    {
        att = actualShadowingMap->at(nodeId).second;
    }
    EV <<  " NRChannelModel_3GPP38_901::computeShadowing - shadowing att = " << att << endl;

    return att;
}

