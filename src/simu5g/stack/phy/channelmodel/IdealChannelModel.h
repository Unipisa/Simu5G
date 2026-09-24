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

#ifndef STACK_PHY_CHANNELMODEL_IDEALCHANNELMODEL_H_
#define STACK_PHY_CHANNELMODEL_IDEALCHANNELMODEL_H_

#include "simu5g/common/LteControlInfo.h"
#include "simu5g/stack/phy/channelmodel/ChannelModelBase.h"

namespace simu5g {

using namespace omnetpp;

class IdealChannelModel : public ChannelModelBase
{
  private:
    // volatile, so that the error rate can be made a function of simulation time --
    // which is how a coverage loss is scripted
    cPar *perDl_ = nullptr;
    cPar *perUl_ = nullptr;
    cPar *perD2D_ = nullptr;
    double harqReduction_ = 0;

    /**
     * Error probability of the txNumber-th transmission attempt of a frame sent in
     * the given direction.
     */
    virtual double getErrorProbability(Direction dir, unsigned char txNumber) const;

  public:
    void initialize(int stage) override;

    /*
     * Compute the error probability of the transmitted packet
     *
     * @param frame pointer to the packet
     * @param lteInfo pointer to the user control info
     */
    bool isReceptionSuccessful(AirFrame *frame, UserControlInfo *lteInfo, const std::vector<double>& rsrpVector) override;
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
    double getAttenuation(const RadioLink& link) override
    {
        return 0;
    }

    /*
     * Compute fake SINR for each band for user nodeId according to path loss, shadowing (optional) and multipath fading
     *
     * @param frame pointer to the packet
     * @param lteInfo pointer to the user control info
     */
    std::vector<double> getSINR(AirFrame *frame, UserControlInfo *lteInfo) override;
    /*
     * Compute fake received useful signal for each band for user nodeId according to path loss, shadowing (optional) and multipath fading
     *
     * @param frame pointer to the packet
     * @param lteInfo pointer to the user control info
     */
    std::vector<double> getRSRP(AirFrame *frame, UserControlInfo *lteInfo) override;
    /*
     * Compute SINR for each band for a background UE according to path loss
     *
     * @param frame pointer to the packet
     * @param lteInfo pointer to the user control info
     */
    std::vector<double> getSINR_bgUe(AirFrame *frame, UserControlInfo *lteInfo) override;
};

} //namespace

#endif /* STACK_PHY_CHANNELMODEL_IDEALCHANNELMODEL_H_ */

