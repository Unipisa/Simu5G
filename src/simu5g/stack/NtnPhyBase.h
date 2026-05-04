#ifndef __SIMU5G_NTNPHYBASE_H_
#define __SIMU5G_NTNPHYBASE_H_

#include <map>

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/InitStages.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/mobility/georeference/GeographicReferenceSystem.h"
#include "simu5g/stack/phy/channelmodel/LteChannelModel.h"
#include "simu5g/stack/phy/antennamodel/IAntennaModel.h"
#include "simu5g/world/radio/ChannelAccess.h"

namespace simu5g {

class NtnPhyBase : public ChannelAccess
{
  protected:
    inet::ModuleRefByPar<Binder> binder_;
    inet::ModuleRefByPar<IAntennaModel> antennaModel_;
    GeographicReferenceSystem *referenceSystem_ = nullptr;
    std::map<GHz, opp_component_ptr<LteChannelModel>> channelModel_;
    inet::ModuleRefByPar<LteChannelModel> primaryChannelModel_;
    MacNodeId nodeId_ = NODEID_NONE;
    RanNodeType nodeType_ = UNKNOWN_NODE_TYPE;
    bool isFeederLink_ = false;
    GHz feederLinkFrequencyOffset_ = GHz(NTN_FEEDER_LINK_FREQUENCY_OFFSET_GHZ);

    void initialize(int stage) override;
    void initializeChannelModels();
    LteChannelModel *getChannelModel(GHz carrierFreq) const;
    GHz shiftFrequencyBand(GHz carrierFreq) const;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(omnetpp::cMessage *msg) override;
    void handleAirFrame(omnetpp::cMessage *msg);
    void handleUpperMessage(omnetpp::cMessage *msg);

    omnetpp::cGate *resolvePeerGate() const;
    omnetpp::cModule *resolvePeerNode() const;
    int getReceiverGateIndex(const omnetpp::cModule *receiver, bool isNr) const;
};

} // namespace simu5g

#endif
