//
//                  Simu5G
//
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef STACK_PHY_MEDIUM_CELLULARRADIOMEDIUM_H_
#define STACK_PHY_MEDIUM_CELLULARRADIOMEDIUM_H_

#include <map>

#include <omnetpp.h>

#include <inet/common/INETDefs.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/stack/phy/medium/CellularTransmission.h"
#include "simu5g/stack/phy/medium/ChannelState.h"

namespace simu5g {

using namespace omnetpp;

class AirFrame;
class IRadioEndpoint;

/**
 * The radio medium of a cellular network (see the NED documentation). So far
 * it keeps the registry of the radios on the medium -- every PHY registers its
 * radio endpoint under its own MacNodeId, so a dual-stack UE's two PHYs are
 * two radios -- delivers broadcast frames to the radios in range, and keeps
 * the channel state every channel model evaluates links against.
 */
class CellularRadioMedium : public cSimpleModule
{
  protected:
    /** A radio that can be sent frames. */
    struct RadioModule {
        IRadioEndpoint *radio = nullptr;
        cGate *radioInGate = nullptr; // where frames sent to the radio enter its node
    };

    std::map<MacNodeId, IRadioEndpoint *> radios; // the radios with a node id
    std::map<int, RadioModule> radioModules; // by the endpoint's (PHY's) module id: the order broadcast frames are delivered in
    double maxInterferenceDistance = 0; // the range of a broadcast
    long nextTransmissionId = 0;
    std::map<GHz, std::vector<const CellularTransmission *>> transmissions; // per carrier, in creation order; the ones not yet over
    std::map<const cComponent *, ChannelState> channelStates; // per channel model evaluating links
    std::map<GHz, std::map<LinkKey, bool>> losMaps; // per carrier: whether each link is in line of sight
    std::map<GHz, ChannelState::ShadowFadingMap> shadowingMaps; // per carrier: the last shadowing sample of each link
    std::map<GHz, ChannelState::JakesFadingMap> jakesFadingMaps; // per carrier: the Jakes fading paths of each link

  protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

  public:
    virtual ~CellularRadioMedium();

    /**
     * Adds a radio under the node id of its PHY. A radio registered with
     * NODEID_NONE is on the medium, but cannot be looked up. If radioModule is
     * given, broadcast frames are sent to its radioIn gate (entering at the
     * node). Throws if the id is taken.
     */
    virtual void addRadio(MacNodeId nodeId, IRadioEndpoint *radio, cModule *radioModule = nullptr);

    /** Removes the radio. Throws if it is not on the medium. */
    virtual void removeRadio(IRadioEndpoint *radio);

    /** The radio of the given node id, or nullptr if none is registered. */
    virtual IRadioEndpoint *findRadio(MacNodeId nodeId) const;

    /** The radio of the given node id. Throws if none is registered. */
    virtual IRadioEndpoint *getRadio(MacNodeId nodeId) const;

    /**
     * Sends a copy of the frame to every other radio closer to the sender than
     * the broadcast range, in the order of their module ids, with no
     * propagation delay and the given duration; deletes the original. Called
     * by the sending radio module, which sends the copies.
     */
    virtual void sendToNeighbors(IRadioEndpoint *sender, cSimpleModule *sendingModule, AirFrame *frame, simtime_t duration);

    /**
     * Adds a transmission, giving it the next id; the medium owns it. The
     * transmissions on the same carrier that ended before now are dropped.
     */
    virtual void addTransmission(CellularTransmission *transmission);

    /**
     * Whether the uplink transmissions on the carrier that end now -- data
     * frames sent in UL or on the sidelink -- are, band by band and in
     * creation order, the entries of a PHY in the Binder's uplink
     * transmission map (the entries of a background UE are left out).
     */
    virtual bool matchesUplinkTransmissionMap(GHz carrierFrequency, const std::vector<std::vector<UeAllocationInfo>>& map) const;

    /**
     * The channel state the given channel model evaluates links against,
     * created empty on first use. The reference stays valid until
     * removeChannelState() is called for the same channel model.
     */
    virtual ChannelState& getChannelState(const cComponent *channelModel);

    /** Drops the channel state of the given channel model, if it has any. */
    virtual void removeChannelState(const cComponent *channelModel);

    /**
     * Whether each link on the carrier is in line of sight, by link: one state
     * per link, shared by every channel model evaluating it. Created empty on
     * first use; the reference stays valid for the lifetime of the medium.
     * Entries are never removed: node ids are not reused, so the links of a
     * radio that has left are simply never looked up again.
     */
    virtual std::map<LinkKey, bool>& getLosMap(GHz carrierFrequency);

    /**
     * The last shadowing sample of each link on the carrier, with when and
     * where it was drawn, by link: one per link, shared by every channel model
     * evaluating it. Created and kept as getLosMap().
     */
    virtual ChannelState::ShadowFadingMap& getShadowingMap(GHz carrierFrequency);

    /**
     * The Jakes fading paths of each link on the carrier, every band's, by
     * link: one realization per link, shared by every channel model
     * evaluating it. Created and kept as getLosMap().
     */
    virtual ChannelState::JakesFadingMap& getJakesFadingMap(GHz carrierFrequency);
};

} // namespace simu5g

#endif
