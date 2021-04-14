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

#include "stack/phy/ChannelModel/NRChannelModel.h"

// attenuation value to be returned if max. distance of a scenario has been violated
// and tolerating the maximum distance violation is enabled
#define ATT_MAXDISTVIOLATED 1000

Define_Module(NRChannelModel);

void NRChannelModel::initialize(int stage)
{
    LteRealisticChannelModel::initialize(stage);
}

double NRChannelModel::getAttenuation(MacNodeId nodeId, Direction dir, inet::Coord coord)
{
   double movement = .0;
   double speed = .0;

   //COMPUTE 3D and 2D DISTANCE between ue and eNodeB
   double threeDimDistance = phy_->getCoord().distance(coord);
   double twoDimDistance = getTwoDimDistance(phy_->getCoord(), coord);

   if (dir == DL) // sender is UE
       speed = computeSpeed(nodeId, phy_->getCoord());
   else
       speed = computeSpeed(nodeId, coord);

   //If traveled distance is greater than correlation distance UE could have changed its state and
   // its visibility from eNodeb, hence it is correct to recompute the los probability
   if (movement > correlationDistance_
           || losMap_.find(nodeId) == losMap_.end())
   {
       computeLosProbability(twoDimDistance, nodeId);
   }

   //compute attenuation based on selected scenario and based on LOS or NLOS
   bool los = losMap_[nodeId];
   double attenuation = computePathLoss(threeDimDistance, twoDimDistance, los);

   //    Applying shadowing only if it is enabled by configuration
   //    log-normal shadowing
   if (shadowing_)
       attenuation += computeShadowing(twoDimDistance, nodeId, speed);

   // update current user position

   //if sender is a eNodeB
   if (dir == DL)
       //store the position of user
       updatePositionHistory(nodeId, phy_->getCoord());
   else
       //sender is an UE
       updatePositionHistory(nodeId, coord);

   EV << "NRChannelModel::getAttenuation - computed attenuation at distance " << threeDimDistance << " for eNb is " << attenuation << endl;

   return attenuation;
}

double NRChannelModel::computeAngolarAttenuation(double hAngle, double vAngle) {

    // --- compute horizontal pattern attenuation --- //
    double angolarAttMin = 30;

    // compute attenuation due to horizontal angolar position
    double hAngolarAtt = 12 * pow(hAngle / 65.0, 2);
    if (hAngolarAtt > angolarAttMin)
        hAngolarAtt = angolarAttMin;

    // --- compute vertical pattern attenuation --- //
    double vTilt = 90;
    double vAngolarAtt = 12 * pow( (vAngle - vTilt) / 65.0, 2);
    if (vAngolarAtt > angolarAttMin)
        vAngolarAtt = angolarAttMin;

    double angolarAtt = hAngolarAtt + vAngolarAtt;
    return (angolarAtt < angolarAttMin) ? angolarAtt : angolarAttMin;
}

void NRChannelModel::computeLosProbability(double d, MacNodeId nodeId)
{
   double p = 0;
   if (!dynamicLos_)
   {
       losMap_[nodeId] = fixedLos_;
       return;
   }
   switch (scenario_)
   {
   case URBAN_MICROCELL:
       if (d <= 18.0)
           p = 1.0;
       else
           p = (18 / d) + exp(-1 * d / 36) * (1 - (18 / d));
       break;
   case URBAN_MACROCELL:
       if (d <= 18.0)
           p = 1.0;
       else
       {
           double C = (hUe_ <= 13.0) ? 0 : pow((hUe_-13.0)/10.0, 1.5);
           p = ( (18 / d) + exp(-1 * d / 63) * (1 - (18 / d)) ) * ( 1 + C * (5.0/4.0) * pow(d/100.0,3) * exp(-1 * d/150.0) );
       }
       break;
   case RURAL_MACROCELL:
       if (d <= 10)
           p = 1;
       else
           p = exp(-1 * (d - 10) / 1000);
       break;
   default:
       LteRealisticChannelModel::computeLosProbability(d,nodeId);
       return;
   }
   double random = uniform(0.0, 1.0);
   if (random <= p)
       losMap_[nodeId] = true;
   else
       losMap_[nodeId] = false;
}


double NRChannelModel::computePathLoss(double threeDimDistance, double twoDimDistance, bool los)
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
        return LteRealisticChannelModel::computePathLoss(twoDimDistance, 0.0, los);
    }
    return pathLoss;
}

double NRChannelModel::computeIndoor(double threeDimDistance, double twoDimDistance, bool los)
{
   double a, b;
   if (los)
   {
       if (twoDimDistance > 150 || twoDimDistance < 3)
           throw cRuntimeError("Error LOS indoor path loss model is valid for 3<d<150");
       a = 16.9;
       b = 32.8;
   }
   else
   {
       if (twoDimDistance > 250 || twoDimDistance < 6)
           throw cRuntimeError("Error NLOS indoor path loss model is valid for 6<d<250");
       a = 43.3;
       b = 11.5;
   }
   return a * log10(threeDimDistance) + b + 20 * log10(carrierFrequency_);
}

double NRChannelModel::computeUrbanMicro(double threeDimDistance, double twoDimDistance, bool los)
{
   if (twoDimDistance < 10)
       twoDimDistance = 10;

   if (twoDimDistance > 5000){
       if(tolerateMaxDistViolation_)
           return ATT_MAXDISTVIOLATED;
       else
           throw cRuntimeError("Error urban microcell path loss model is valid for d<5000 m");
   }

   // compute break-point distance
   double hNodeB = hNodeB_ - 1.0;
   double hUe = hUe_ - 1.0;
   double dbp = 4 * hNodeB * hUe * ((carrierFrequency_ * 1000000000) / SPEED_OF_LIGHT);

   double pLoss_los = 0.0;
   if (twoDimDistance < dbp)
       pLoss_los = 22 * log10(threeDimDistance) + 28 + 20 * log10(carrierFrequency_);
   else
       pLoss_los = 40 * log10(threeDimDistance) + 28 + 20 * log10(carrierFrequency_) - 9 * log10( (dbp * dbp + (hNodeB_ - hUe_) * (hNodeB_ - hUe_) ) );

   if (los)
       return pLoss_los;

   // NLOS case

   if (twoDimDistance > 2000.0){
          if(tolerateMaxDistViolation_)
              twoDimDistance = 2000.0;
          else
              throw cRuntimeError("Error NLOS urban microcell path loss model is valid for d<2000 m");
   }

   double pLoss_nlos = 36.7 * log10(threeDimDistance) + 22.7
           + 26 * log10(carrierFrequency_) - 0.3 * (hUe_ - 1.5);

   return (pLoss_los > pLoss_nlos) ? pLoss_los : pLoss_nlos;
}

double NRChannelModel::computeUrbanMacro(double threeDimDistance, double twoDimDistance, bool los)
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
   double C = (hUe_ < 13.0) ? 0 : pow( ((hUe_-13.0)/10.0) , 1.5);
   double prob = 1.0 / (1.0 + C);
   if (prob < uniform(0.0,1.0))
       hEnvir = 1.0;
   else
   {
       double bound = hUe_ - 1.5;
       std::vector<double> hVec;
       for (double h=12; h<bound; h+=3)
           hVec.push_back(h);
       hVec.push_back(bound);
       int index = intuniform(0, hVec.size()-1);
       hEnvir = hVec.at(index);
   }

   double hNodeB = hNodeB_ - hEnvir;
   double hUe = hUe_ - hEnvir;

   double dbp = 4 * hNodeB * hUe * ((carrierFrequency_ * 1000000000) / SPEED_OF_LIGHT);

   double pLoss_los = 0.0;
   if (twoDimDistance < dbp)
       pLoss_los = 22 * log10(threeDimDistance) + 28 + 20 * log10(carrierFrequency_);
   else
       pLoss_los = 40 * log10(threeDimDistance) + 28 + 20 * log10(carrierFrequency_) - 9 * log10( (dbp * dbp + (hNodeB_ - hUe_) * (hNodeB_ - hUe_) ) );

   if (los)
       return pLoss_los + penetrationLoss;

   // NLOS case

   double pLoss_nlos = 161.04 - 7.1 * log10(wStreet_) + 7.5 * log10(hBuilding_)
   - (24.37 - 3.7 * pow(hBuilding_ / hNodeB_, 2)) * log10(hNodeB_)
   + (43.42 - 3.1 * log10(hNodeB_)) * (log10(threeDimDistance) - 3) + 20 * log10(carrierFrequency_)
   - (3.2 * (pow(log10(17.625), 2)) - 4.97) - 0.6 * (hUe_ - 1.5);

   return (pLoss_los > pLoss_nlos) ? pLoss_los + penetrationLoss : pLoss_nlos + penetrationLoss;
}

double NRChannelModel::computeRuralMacro(double threeDimDistance, double twoDimDistance, bool los)
{
    if (twoDimDistance < 10)
        twoDimDistance = 10;

    if (los)
    {
        // LOS situation
        if (twoDimDistance > 10000) {
            if(tolerateMaxDistViolation_)
                return ATT_MAXDISTVIOLATED;
            else
                throw cRuntimeError("Error rural macrocell path loss model is valid for d < 10000 m");
        }

        double dbp = 2 * M_PI * hNodeB_ * hUe_ * ((carrierFrequency_ * 1000000000) / SPEED_OF_LIGHT);

        double a1 = (0.03 * pow(hBuilding_, 1.72));
        double b1 = 0.044 * pow(hBuilding_, 1.72);
        double a = (a1 < 10) ? a1 : 10;
        double b = (b1 < 14.77) ? b1 : 14.77;

        if (twoDimDistance < dbp)
            return 20 * log10((40 * M_PI * threeDimDistance * carrierFrequency_) / 3)
                   + a * log10(threeDimDistance) - b + 0.002 * log10(hBuilding_) * threeDimDistance;
        else
            return 20 * log10((40 * M_PI * dbp * carrierFrequency_) / 3)
                   + a * log10(dbp) - b + 0.002 * log10(hBuilding_) * dbp
                   + 40 * log10(threeDimDistance / dbp);
    }

    // NLOS situation
    if (twoDimDistance > 5000) {
        if(tolerateMaxDistViolation_)
            return ATT_MAXDISTVIOLATED;
        else
            throw cRuntimeError("Error NLOS rural macrocell path loss model is valid for d<5000 m");
    }

    double pLoss_nlos = 161.04 - 7.1 * log10(wStreet_) + 7.5 * log10(hBuilding_)
                 - (24.37 - 3.7 * pow(hBuilding_ / hNodeB_, 2)) * log10(hNodeB_)
                 + (43.42 - 3.1 * log10(hNodeB_)) * (log10(threeDimDistance) - 3) + 20 * log10(carrierFrequency_)
                 - (3.2 * (pow(log10(11.75 * hUe_), 2)) - 4.97);
    return pLoss_nlos;
}

bool NRChannelModel::computeExtCellInterference(MacNodeId eNbId, MacNodeId nodeId, Coord coord, bool isCqi, double carrierFrequency,
       std::vector<double>* interference)
{
   EV << "**** Ext Cell Interference **** " << endl;

   // get external cell list
   ExtCellList list = binder_->getExtCellList(carrierFrequency);
   ExtCellList::iterator it = list.begin();

   Coord c;
   double threeDimDist, // meters
   twoDimDist, // meters
   recvPwr, // watt
   recvPwrDBm, // dBm
   att, // dBm
   angolarAtt; // dBm

   std::vector<double> fadingAttenuation;

   //compute distance for each cell
   while (it != list.end())
   {
       // get external cell position
       c = (*it)->getPosition();
       // computer distance between UE and the ext cell
       threeDimDist = coord.distance(c);
       twoDimDist = getTwoDimDistance(coord, c);

       EV << "\t distance between UE[" << coord.x << "," << coord.y <<
               "] and extCell[" << c.x << "," << c.y << "] is -> "
               << threeDimDist << "\t";

       // compute attenuation according to some path loss model
       att = computeExtCellPathLoss(threeDimDist, twoDimDist, nodeId);

       //=============== ANGOLAR ATTENUATION =================
       if ((*it)->getTxDirection() == OMNI)
       {
           angolarAtt = 0;
       }
       else
       {
           // compute the angle between uePosition and reference axis, considering the eNb as center
           double ueAngle = computeAngle(c, coord);

           // compute the reception angle between ue and eNb
           double recvAngle = fabs((*it)->getTxAngle() - ueAngle);

           if (recvAngle > 180)
               recvAngle = 360 - recvAngle;

           double verticalAngle = computeVerticalAngle(c, coord);

           // compute attenuation due to sectorial tx
           angolarAtt = computeAngolarAttenuation(recvAngle,verticalAngle);
       }
       //=============== END ANGOLAR ATTENUATION =================

       // TODO do we need to use (- cableLoss_ + antennaGainEnB_) in ext cells too?
       // compute and linearize received power
       recvPwrDBm = (*it)->getTxPower() - att - angolarAtt - cableLoss_ + antennaGainEnB_ + antennaGainUe_;
       recvPwr = dBmToLinear(recvPwrDBm);

       int numBands = std::min(numBands_, (*it)->getNumBands());
       EV << " - shared bands [" << numBands << "]" << endl;

       // add interference in those bands where the ext cell is active
       for (unsigned int i = 0; i < numBands; i++)
       {
           int occ;
           if (isCqi)  // check slot occupation for this TTI
           {
               occ = (*it)->getBandStatus(i);
           }
           else        // error computation. We need to check the slot occupation of the previous TTI
           {
               occ = (*it)->getPrevBandStatus(i);
           }

           // if the ext cell is active, add interference
           if (occ)
           {
               (*interference)[i] += recvPwr;
           }
       }

       it++;
   }

   return true;
}

double NRChannelModel::computeExtCellPathLoss(double threeDimDistance, double twoDimDistance, MacNodeId nodeId)
{
   double movement = .0;
   double speed = .0;

   speed = computeSpeed(nodeId, phy_->getCoord());

   //    EV << "LteRealisticChannelModel::computeExtCellPathLoss:" << scenario_ << "-" << shadowing_ << "\n";

   //compute attenuation based on selected scenario and based on LOS or NLOS
   bool los = losMap_[nodeId];

   if(!enable_extCell_los_)
      los = false;
   double dbp = 0;
   double attenuation = computePathLoss(threeDimDistance, twoDimDistance, los);

   //TODO Apply shadowing to each interfering extCell signal

   //    Applying shadowing only if it is enabled by configuration
   //    log-normal shadowing
   if (shadowing_)
   {
       double mean = 0;

       //        Get std deviation according to los/nlos and selected scenario

       //        double stdDev = getStdDev(dist < dbp, nodeId);
       double time = 0;
       double space = 0;
       double att;
       //
       //
       //        //If the shadowing attenuation has been computed at least one time for this user
       //        // and the distance traveled by the UE is greated than correlation distance
       //        if ((NOW - lastComputedSF_.at(nodeId).first).dbl() * speed > correlationDistance_)
       //        {
       //            //get the temporal mark of the last computed shadowing attenuation
       //            time = (NOW - lastComputedSF_.at(nodeId).first).dbl();
       //
       //            //compute the traveled distance
       //            space = time * speed;
       //
       //            //Compute shadowing with a EAW (Exponential Average Window) (step1)
       //            double a = exp(-0.5 * (space / correlationDistance_));
       //
       //            //Get last shadowing attenuation computed
       //            double old = lastComputedSF_.at(nodeId).second;
       //
       //            //Compute shadowing with a EAW (Exponential Average Window) (step2)
       //            att = a * old + sqrt(1 - pow(a, 2)) * normal(getEnvir()->getRNG(0), mean, stdDev);
       //        }
       //         if the distance traveled by the UE is smaller than correlation distance shadowing attenuation remain the same
       //        else
       {
           att = lastComputedSF_.at(nodeId).second;
       }
       EV << "(" << att << ")";
       attenuation += att;
   }

   return attenuation;
}

