#ifndef __SIMU5G_NTNPHYBASE_H_
#define __SIMU5G_NTNPHYBASE_H_

#include <map>

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/InitStages.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/mobility/georeference/GeographicReferenceSystem.h"
#include "simu5g/stack/phy/NtnPropagationDelay.h"
#include "simu5g/stack/phy/channelmodel/LteChannelModel.h"
#include "simu5g/stack/phy/antennamodel/IAntennaModel.h"
#include "simu5g/world/radio/ChannelAccess.h"

namespace simu5g {

class NtnPhyBase : public ChannelAccess
{
  protected:
    //
    // What this NIC must do with an incoming air frame before relaying it further.
    // A transparent path evaluates the channel twice: the satellite stores the result
    // of the first radio hop in the frame, and the last radio receiver combines it with
    // its own hop.
    //
    enum class HopAction {
        RELAY_ONLY,             // no per-hop evaluation, just forward
        STORE_RELAY_HOP_SINR,   // first radio hop: measure and carry the result
        STORE_RELAY_HOP_RSRP,   // first radio hop: measure and carry the RSRP result
        STORE_END_TO_END_SINR,  // last radio hop: combine both hops into a CSI measurement
        STORE_RECEPTION_RESULT, // last radio hop: combine both hops into a decoding decision
    };

    inet::ModuleRefByPar<Binder> binder_;
    inet::ModuleRefByPar<IAntennaModel> antennaModel_;
    GeographicReferenceSystem *referenceSystem_ = nullptr;
    std::map<GHz, opp_component_ptr<LteChannelModel>> channelModel_;
    inet::ModuleRefByPar<LteChannelModel> primaryChannelModel_;
    MacNodeId nodeId_ = NODEID_NONE;
    RanNodeType nodeType_ = UNKNOWN_NODE_TYPE;
    bool isFeederLink_ = false;
    GHz feederLinkFrequencyOffset_ = GHz(NTN_FEEDER_LINK_FREQUENCY_OFFSET_GHZ);
    NtnPropagationDelay propagationDelay_;

    void initialize(int stage) override;
    void initializeChannelModels();
    // Adds a channel model to the carrier lookup, retuning it to the feeder carrier first
    // when this NIC serves the frequency-translated hop.
    void registerChannelModel(LteChannelModel *channelModel);
    LteChannelModel *getChannelModel(GHz carrierFreq) const;
    GHz shiftFrequencyBand(GHz carrierFreq) const;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(omnetpp::cMessage *msg) override;
    void handleAirFrame(omnetpp::cMessage *msg);
    HopAction getHopAction(const UserControlInfo& lteInfo) const;
    void handleUpperMessage(omnetpp::cMessage *msg);

    omnetpp::cGate *resolvePeerGate() const;
    omnetpp::cModule *resolvePeerNode() const;
    int getReceiverGateIndex(const omnetpp::cModule *receiver, bool isNr) const;

    // Fills in the transmitter-side radio metadata carried by every NTN air frame, and
    // returns the transmitter's ECEF position. The receiver id is left to the caller: the
    // CSI-RS/beacon fan-out sets a different one on each duplicated frame.
    inet::Coord setRadioTransmitterInfo(UserControlInfo& lteInfo) const;

  public:
    // Carrier that the given hop carrier corresponds to on the UE-facing service link.
    // Feeder-link hops are frequency-translated, so their carrier differs from the one the
    // UE transmits and receives on.
    GHz toServiceLinkCarrier(GHz carrierFreq) const;
};

} // namespace simu5g

#endif
