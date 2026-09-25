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

#ifndef STACK_D2D_PHY_CHANNELMODEL_D2DCHANNELMODEL_H_
#define STACK_D2D_PHY_CHANNELMODEL_D2DCHANNELMODEL_H_

#include "simu5g/stack/d2d/phy/channelmodel/ID2dChannelModel.h"
#include "simu5g/stack/phy/channelmodel/StochasticChannelModel.h"

namespace simu5g {

using namespace inet;
using namespace omnetpp;

/**
 * Channel model for D2D-capable NICs: StochasticChannelModel (whose
 * pathLossType parameter selects the propagation study -- TR 36.814, 36.873
 * or 38.901) plus the ~800 lines of D2D channel math (attenuation, RSRP/SINR,
 * interference and reception decision) layered on top of it.
 *
 * The rcvdSinrD2D signal is owned and interned here, not in the core channel model.
 */
class D2dChannelModel : public StochasticChannelModel, public ID2dChannelModel
{
  protected:
    // enable/disable the interference computation for D2D connections
    bool enableD2DInterference_ = false;

    // Interned in initialize() rather than registered by a static initializer, so that
    // linking the D2D package in cannot shift the signal ids the core assigns. See the
    // "Signals" note in D2dUeMacBase.h.
    simsignal_t rcvdSinrD2DSignal_ = SIMSIGNAL_NULL;

    /*
     * Build the RadioLink for a UE-to-UE transmission, so that the core
     * propagation path can evaluate it. Both endpoints being UEs is the whole of
     * what makes a D2D link different: no sectorial antenna (hence no angular
     * attenuation); the gains and the noise figure are the two UEs' radios'.
     */
    RadioLink d2dLink(MacNodeId srcId, inet::Coord srcCoord, MacNodeId destId, inet::Coord destCoord, bool useUeSideMaps);

    /*
     * Compute interference coming from neighboring UEs for the D2D/D2D_MULTI direction
     */
    bool computeD2DInterference(MacNodeId eNbId, MacNodeId senderId, inet::Coord senderCoord, MacNodeId destId, inet::Coord destCoord, bool isCqi, GHz carrierFrequency, std::vector<double> *interference, Direction dir);

    // Route D2D/D2D_MULTI receptions through getSINR_D2D (called from the core
    // isReceptionSuccessful()).
    std::vector<double> getReceptionSinr(AirFrame *frame, UserControlInfo *lteInfo, const std::vector<double>& rsrpVector) override;

    // Report D2D receptions under rcvdSinrD2D rather than letting them fall into
    // the core's uplink statistic.
    void emitRcvdSinr(Direction dir, MacNodeId ueId, GHz carrierFrequency, double sinr) override;

    // Substitute the UE-to-UE interference for the cellular contributions.
    void computeInterferencePlusNoise(const RadioLink& link, UserControlInfo *lteInfo,
            RbMap& rbmap, double totN, std::vector<double>& den) override;

  public:
    void initialize(int stage) override;

    // ---- ID2dChannelModel ----
    std::vector<double> getRSRP_D2D(AirFrame *frame, UserControlInfo *lteInfo_1, MacNodeId destId, inet::Coord destCoord) override;
    std::vector<double> getSINR_D2D(AirFrame *frame, UserControlInfo *lteInfo, MacNodeId destId, inet::Coord destCoord, MacNodeId enbId = NODEID_NONE) override;
    std::vector<double> getSINR_D2D(AirFrame *frame, UserControlInfo *lteInfo_1, MacNodeId destId, inet::Coord destCoord, MacNodeId enbId, const std::vector<double>& rsrpVector) override;

    virtual bool isD2DInterferenceEnabled() { return enableD2DInterference_; }
    bool recordsUlTransmissionMap() override { return isUplinkInterferenceEnabled() || enableD2DInterference_; }
};

} //namespace

#endif /* STACK_D2D_PHY_CHANNELMODEL_D2DCHANNELMODEL_H_ */
