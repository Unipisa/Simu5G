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

#ifndef STACK_PHY_CHANNELMODEL_LTEDUMMYCHANNELMODEL_H_
#define STACK_PHY_CHANNELMODEL_LTEDUMMYCHANNELMODEL_H_

#include "common/LteControlInfo.h"
#include "stack/phy/ChannelModel/LteChannelModel.h"

class LteDummyChannelModel : public LteChannelModel
{
     private:
   double per_;
   double harqReduction_;
 public:
   void initialize(int stage);

   /*
    * Compute the error probability of the transmitted packet
    *
    * @param frame pointer to the packet
    * @param lteinfo pointer to the user control info
    */
   virtual bool isError(LteAirFrame *frame, UserControlInfo* lteInfo);
   /*
    * Compute the path-loss attenuation according to the selected scenario
    */
   virtual double computePathLoss(double distance, double dbp, bool los)
   {
       return 0;
   }
   /*
    * Compute Attenuation caused by pathloss and shadowing (optional)
    */
   virtual double getAttenuation(MacNodeId nodeId, Direction dir, inet::Coord coord)
   {
       return 0;
   }
   /*
    * Compute FAKE sir for each band for user nodeId according to multipath fading
    *
    * @param frame pointer to the packet
    * @param lteinfo pointer to the user control info
    */
   virtual std::vector<double> getSIR(LteAirFrame *frame, UserControlInfo* lteInfo);
   /*
    * Compute FAKE sinr for each band for user nodeId according to pathloss, shadowing (optional) and multipath fading
    *
    * @param frame pointer to the packet
    * @param lteinfo pointer to the user control info
    */
   virtual std::vector<double> getSINR(LteAirFrame *frame, UserControlInfo* lteInfo);
   /*
    * Compute the error probability of the transmitted packet according to cqi used, txmode, and the received power
    * after that it throws a random number in order to check if this packet will be corrupted or not
    *
    * @param frame pointer to the packet
    * @param lteinfo pointer to the user control info
    * @param rsrpVector the received signal for each RB, if it has already been computed
    */
   virtual bool isError_D2D(LteAirFrame *frame, UserControlInfo* lteInfo, const std::vector<double>& rsrpVector);
   /*
    * Compute Received useful signal for D2D transmissions
    */
   virtual std::vector<double> getRSRP_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, inet::Coord destCoord);
   /*
    * Compute FAKE sinr (D2D) for each band for user nodeId according to pathloss, shadowing (optional) and multipath fading
    *
    * @param frame pointer to the packet
    * @param lteinfo pointer to the user control info
    */
   virtual std::vector<double> getSINR_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, inet::Coord destCoord,MacNodeId enbId);
   virtual std::vector<double> getSINR_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, inet::Coord destCoord,MacNodeId enbId,const std::vector<double>& rsrpVector);
   //TODO
   virtual bool isErrorDas(LteAirFrame *frame, UserControlInfo* lteI)
   {
       throw omnetpp::cRuntimeError("DAS PHY LAYER TO BE IMPLEMENTED");
       return false;
   }

};

#endif /* STACK_PHY_CHANNELMODEL_LTEDUMMYCHANNELMODEL_H_ */
