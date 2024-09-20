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

namespace simu5g {

using namespace omnetpp;

class LteDummyChannelModel : public LteChannelModel
{
  private:
    double per_;
    double harqReduction_;

  public:
    void initialize(int stage) override;

    /*
     * Compute the error probability of the transmitted packet
     *
     * @param frame pointer to the packet
     * @param lteInfo pointer to the user control info
     */
    bool isError(LteAirFrame *frame, UserControlInfo *lteInfo) override;
    /*
     * Compute the path-loss attenuation according to the selected scenario
     */
    double computePathLoss(double distance, double dbp, bool los) override
    {
        return 0;
    }

    /*
     * Compute attenuation caused by path loss and shadowing (optional)
     */
    double getAttenuation(MacNodeId nodeId, Direction dir, inet::Coord coord, bool cqiDl) override
    {
        return 0;
    }

    /*
     * Compute fake SIR for each band for user nodeId according to multipath fading
     *
     * @param frame pointer to the packet
     * @param lteInfo pointer to the user control info
     */
    std::vector<double> getSIR(LteAirFrame *frame, UserControlInfo *lteInfo) override;
    /*
     * Compute fake SINR for each band for user nodeId according to path loss, shadowing (optional) and multipath fading
     *
     * @param frame pointer to the packet
     * @param lteInfo pointer to the user control info
     */
    std::vector<double> getSINR(LteAirFrame *frame, UserControlInfo *lteInfo) override;
    /*
     * Compute fake received useful signal for each band for user nodeId according to path loss, shadowing (optional) and multipath fading
     *
     * @param frame pointer to the packet
     * @param lteInfo pointer to the user control info
     */
    std::vector<double> getRSRP(LteAirFrame *frame, UserControlInfo *lteInfo) override;
    /*
     * Compute SINR for each band for a background UE according to path loss
     *
     * @param frame pointer to the packet
     * @param lteInfo pointer to the user control info
     */
    std::vector<double> getSINR_bgUe(LteAirFrame *frame, UserControlInfo *lteInfo) override;
    /*
     * Compute received power for a background UE according to path loss
     *
     */
    double getReceivedPower_bgUe(double txPower, inet::Coord txPos, inet::Coord rxPos, Direction dir, bool losStatus, MacNodeId bsId) override;
    /*
     * Compute the error probability of the transmitted packet according to CQI used, tx mode, and the received power
     * after that it throws a random number in order to check if this packet will be corrupted or not
     *
     * @param frame pointer to the packet
     * @param lteInfo pointer to the user control info
     * @param rsrpVector the received signal for each RB, if it has already been computed
     */
    bool isError_D2D(LteAirFrame *frame, UserControlInfo *lteInfo, const std::vector<double>& rsrpVector) override;
    /*
     * Compute received useful signal for D2D transmissions
     */
    std::vector<double> getRSRP_D2D(LteAirFrame *frame, UserControlInfo *lteInfo_1, MacNodeId destId, inet::Coord destCoord) override;
    /*
     * Compute fake SINR (D2D) for each band for user nodeId according to path loss, shadowing (optional) and multipath fading
     *
     * @param frame pointer to the packet
     * @param lteInfo pointer to the user control info
     */
    std::vector<double> getSINR_D2D(LteAirFrame *frame, UserControlInfo *lteInfo_1, MacNodeId destId, inet::Coord destCoord, MacNodeId enbId) override;
    std::vector<double> getSINR_D2D(LteAirFrame *frame, UserControlInfo *lteInfo_1, MacNodeId destId, inet::Coord destCoord, MacNodeId enbId, const std::vector<double>& rsrpVector) override;
    //TODO
    bool isErrorDas(LteAirFrame *frame, UserControlInfo *lteInfo) override
    {
        throw cRuntimeError("DAS PHY LAYER TO BE IMPLEMENTED");
        return false;
    }

};

} //namespace

#endif /* STACK_PHY_CHANNELMODEL_LTEDUMMYCHANNELMODEL_H_ */

