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

#ifndef _PHYBASE_H_
#define _PHYBASE_H_

#include <map>
#include <vector>
#include <iostream>
#include <math.h>
#include <inet/common/ModuleRefByPar.h>
#include <inet/common/Units.h>

#include "simu5g/world/radio/ChannelControl.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/stack/phy/packet/AirFrame_m.h"
#include "simu5g/stack/mac/amc/LteAmc.h"
#include "simu5g/stack/phy/channelmodel/ChannelModelBase.h"
#include "simu5g/stack/phy/channelmodel/IRadioEndpoint.h"

namespace simu5g {

using namespace omnetpp;

class ChannelModelBase;

/**
 * Base class of the physical layer of the LTE and NR NICs.
 *
 * Packets from the stack (#upperGateIn_) are encapsulated into AirFrames and
 * sent directly to the receiving PHYs; received AirFrames arrive on
 * #radioInGate_ and are handled by the subclasses.
 *
 * The PHY also registers its radio with the ChannelControl module and keeps
 * the radio's position up to date from the host's mobility module; the
 * ChannelControl uses both to deliver broadcast frames (the base station
 * beacons) to the radios in range.
 */
class PhyBase : public cSimpleModule, public cListener, public IRadioEndpoint
{

  protected:

    /**
     * Defines the scheduling priority of AirFrames.
     *
     * AirFrames use a slightly higher priority than normal to ensure
     * channel consistency. This means that before anything else happens
     * at a time point t every AirFrame which ended at t has been removed and
     * every AirFrame started at t has been added to the channel.
     *
     * An example where this matters is a ChannelSenseRequest which ends at
     * the same time as an AirFrame starts (or ends). Depending on which message
     * is handled first the result of ChannelSenseRequest would differ.
     */
    static short airFramePriority_;
    /** channel models to use.*/
    std::map<GHz, opp_component_ptr<ChannelModelBase>> channelModel_;
    inet::ModuleRefByPar<ChannelModelBase> primaryChannelModel_;

    /** The id of the in-data gate from the Stack */
    int upperGateIn_ = -1;
    /** The id of the out-data gate to the Stack */
    int upperGateOut_ = -1;
    /** The id of the radioIn gate to receive AirFrames */
    int radioInGate_ = -1;

    /// the ChannelControl module, which delivers broadcast frames to the radios in range
    opp_component_ptr<ChannelControl> channelControl_;
    /// identifies this radio in the ChannelControl module
    ChannelControl::RadioRef radioRef_ = nullptr;
    /// the host that contains this PHY
    opp_component_ptr<cModule> hostModule_;
    /// the position of the radio, kept up to date from the host's mobility module
    inet::Coord radioPos_;
    /// whether the host's mobility module has reported a position yet
    bool positionUpdateArrived_ = false;

    /** Statistics */
    unsigned int numAirFrameReceived_ = 0;    /// number of AirFrame correctly received
    unsigned int numAirFrameNotReceived_ = 0; /// number of AirFrame not received

    /** Local device MacNodeId */
    MacNodeId nodeId_ = NODEID_NONE;

    /** Node type */
    RanNodeType nodeType_ = UNKNOWN_NODE_TYPE;

    /// Reference to Binder
    inet::ModuleRefByPar<Binder> binder_;

    /// Reference to CellInfo
    opp_component_ptr<CellInfo> cellInfo_;

    //Ue  Tx Power
    double ueTxPower_ = NAN;
    // eNodeB Tx Power
    double eNodeBtxPower_ = NAN;
    //Micro eNb Tx Power
    double microTxPower_ = NAN;
    // Tx Power
    double txPower_ = NAN;
    // Tx Direction
    TxDirectionType txDirection_ = OMNI;
    // Tx Angle
    double txAngle_ = NAN;

    /*
     * NR Support
     */
    bool isNr_ = false;           // this flag is true if this module is part of the NR stack

    //Statistics
    static simsignal_t averageCqiDlSignal_;
    static simsignal_t averageCqiUlSignal_;

    // last time that the node has transmitted (currently, used only by UEs)
    simtime_t lastActive_;

  public:

    ~PhyBase() override;

    /**
     * Called when the host's mobility module reports a position change:
     * updates the radio position in ChannelControl and emits the mobility
     * statistics.
     */
    void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *) override;

    const ChannelModelBase *getPrimaryChannelModel()
    {
        return primaryChannelModel_;
    }

    const std::map<GHz, opp_component_ptr<ChannelModelBase>>& getChannelModels()
    {
        return channelModel_;
    }

    ChannelModelBase *getChannelModel(GHz carrierFreq = GHz(0.0)) override
    {
        if (channelModel_.empty())
            return nullptr;
        if (carrierFreq == GHz(0.0)) // when not specified, returns the first channel model (primary cell) TODO check this
            return channelModel_.begin()->second;
        auto it = channelModel_.find(carrierFreq);
        return (it == channelModel_.end()) ? nullptr : it->second;
    }

    double getMicroTxPwr()
    {
        return microTxPower_;
    }

    double getMacroTxPwr()
    {
        return eNodeBtxPower_;
    }

    double getTxPwr(Direction dir = UNKNOWN_DIRECTION) override
    {
        return txPower_;
    }

    TxDirectionType getTxDirection() override
    {
        return txDirection_;
    }

    double getTxAngle() override
    {
        return txAngle_;
    }

    /**
     * Delivers a decoded packet to the upper (stack) layer: records the
     * reception outcome, attaches the decider result and sends the packet
     * to #upperGateOut_. A D2D-agnostic core operation used, e.g., by the
     * capture-effect decoding in the D2D UE-PHY helper.
     */
    void sendDecodedPacketUp(inet::Packet *pkt, bool receptionSuccessful);

  protected:

    /**
     * Performs initialization operations to prepare gates' IDs and statistics.
     *
     * In the local stage gets gates' IDs, TX power parameters and initializes
     * statistics to be watched.
     * In the Simu5G registrations stage initializes the channel model(s).
     *
     * @param stage initialization stage
     */
    void initialize(int stage) override;

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    /**
     * Processes messages received from #radioInGate_ or from the stack (#upperGateIn_).
     *
     * @param msg message received from stack or from air channel
     */
    void handleMessage(cMessage *msg) override;

    /**
     * Sends a frame to all radios in range (see ChannelControl), with zero
     * propagation delay; the transmission lasts for the given duration.
     */
    virtual void sendBroadcast(AirFrame *airFrame, simtime_t duration);

    /**
     * Sends a frame to the destination specified in its control info, with
     * zero propagation delay; the transmission lasts for the given duration.
     */
    virtual void sendUnicast(AirFrame *airFrame, simtime_t duration);

    /**
     * Called when the host's mobility module reports a position change.
     * Subclasses emit their position-related statistics here.
     */
    virtual void emitMobilityStats() {}

  protected:

    /**
     * Sends the given message to the wireless channel.
     *
     * Called by the handleMessage() method
     * when a message from #upperGateIn_ gate is received.
     *
     * The message is encapsulated into an AirFrame to which
     * a Signal object containing info about TX power, bit-rate and
     * movement pattern is attached.
     * The AirFrame is then sent to the wireless channel.
     *
     * @param msg packet received from LteStack
     */
    virtual void handleUpperMessage(cMessage *msg);

    /// name of the air frame created for an outgoing upper-layer packet
    virtual const char *airFrameNameFor(const UserControlInfo *info);

    /// scheduling priority of the air frame created for an outgoing upper-layer packet
    virtual short airFramePriorityFor(const UserControlInfo *info) { return airFramePriority_; }

    /// stamps additional per-technology fields on the outgoing control info (called after the Tx power)
    virtual void stampExtraTxControlInfo(UserControlInfo *info) {}

    /// hands the prepared air frame to the channel, for the given transmission duration (default: unicast to the destination)
    virtual void transmitFrame(AirFrame *frame, const UserControlInfo *info, simtime_t duration);

    /**
     * Processes messages received from the wireless channel.
     *
     * Called by the handleMessage() method
     * when a message from #radioInGate_ is received.
     *
     * The channel model prepared during the initialization phase determines
     * whether reception is successful, and the frame's inner packet is then
     * sent out to #upperGateOut_ gate along with the result (attached as
     * control info). Concrete behavior is implemented by subclasses.
     *
     * @param msg AirFrame received from the air channel
     */
    virtual void handleAirFrame(cMessage *msg) = 0;

    virtual void handleSelfMessage(cMessage *msg) = 0;

    virtual void handleControlMsg(AirFrame *frame, UserControlInfo *userInfo);

    virtual void initializeChannelModel();

    /**
     * Utility.
     * Shows current statistics above the icon.
     */
    virtual void updateDisplayString();

    /**
     * Determine the radio gate index on the receiving node that serves the
     * destination's stack leg.
     */
    virtual int getReceiverGateIndex(const cModule *receiver, MacNodeId dest) const;

  public:
    /*
     * Returns the current position of the node
     */
    const inet::Coord& getCoord() const override { return radioPos_; }
    /*
     * Returns the time of the last transmission performed
     */
    simtime_t getLastActive() { return lastActive_; }
    /*
     * Returns the MAC Node Id
     */
    MacNodeId getMacNodeId() { return nodeId_; }
};

} //namespace

#endif /* _PHYBASE_H_ */
