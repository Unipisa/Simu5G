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


#ifndef _LTE_AIRPHYBASE_H_
#define _LTE_AIRPHYBASE_H_

#include <map>
#include <vector>
#include <iostream>
#include <omnetpp.h>

#include "world/radio/ChannelAccess.h"
#include "world/radio/ChannelControl.h"
#include "common/LteCommon.h"
#include "common/LteControlInfo.h"
#include "stack/phy/packet/LteAirFrame.h"
#include "stack/mac/layer/LteMacEnb.h"
#include "stack/mac/amc/LteAmc.h"
#include "stack/phy/ChannelModel/LteChannelModel.h"
#include "stack/phy/feedback/LteFeedbackComputationRealistic.h"

/**
 * @class LtePhy
 * @brief Physical layer of Lte Nic.
 *
 * This class implements the physical layer of the Lte Nic.
 * It contains methods to manage analog models and decider.
 *
 * The module receives packets from the LteStack and
 * sends them to the air channel, encapsulated in LteAirFrames.
 *
 * The module receives LteAirFrames from the radioIn gate,
 * filters the received signal using the analog models,
 * processes the received signal using the decider,
 * then decapsulates the inner packet and sends it to the
 * LteStack with LteDeciderControlInfo attached.
 */

class LteChannelModel;

class LtePhyBase : public ChannelAccess
{
    friend class DasFilter;

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
    std::map<double, LteChannelModel*> channelModel_;
    LteChannelModel* primaryChannelModel_;

    /** The id of the in-data gate from the Stack */
    int upperGateIn_;
    /** The id of the out-data gate to the Stack */
    int upperGateOut_;
    /** The id of the radioIn gate to receive LteAirFrames */
    int radioInGate_;

    /** Pointer to the World Utility, to obtain some global information*/
    //BaseWorldUtility* world_;
    /** Statistics */
    unsigned int numAirFrameReceived_;    /// number of LteAirFrame correctly received
    unsigned int numAirFrameNotReceived_; /// number of LteAirFrame not received

    /** Local device MacNodeId */
    MacNodeId nodeId_;

    /** Node type */
    RanNodeType nodeType_;

    /// Reference to Binder
    Binder *binder_;

    /// Reference to CellInfo
    CellInfo* cellInfo_;

    // used in multicast D2D to prevent a send direct towards out-of-range UEs. Range is expressed via multicastD2DRange_
    bool enableMulticastD2DRangeCheck_;

    // used with the enableMulticastD2DRangeCheck_ parameter
    double multicastD2DRange_;

    /*
     * If true, UEs associate to the best serving cell at initialization
     */
    bool dynamicCellAssociation_;

    //Ue  Tx Power
    double ueTxPower_;
    // eNodeB Tx Power
    double eNodeBtxPower_;
    //Micro eNb Tx Power
    double microTxPower_;
    // Tx Power
    double txPower_;
    // Tx Direction
    TxDirectionType txDirection_;
    // Tx Angle
    double txAngle_;
    // Attenuation array
    AttenuationVector attenuationVector_;
    //Used only for PisaPhy
    LteFeedbackComputation* lteFeedbackComputation_;

    double carrierFrequency_;

    /*
     * NR Support
     */
    bool isNr_;           // this flag is true if this module is part of the NR stack

    //Statistics
    omnetpp::simsignal_t averageCqiDl_;
    omnetpp::simsignal_t averageCqiUl_;
    omnetpp::simsignal_t averageCqiD2D_;

    // User that are trasmitting (uplink)
    //receiveng(downlink) current packet
    MacNodeId connectedNodeId_;

    // last time that the node has transmitted (currently, used only by UEs)
    omnetpp::simtime_t lastActive_;

    public:

    /**
     * Constructor
     */
    LtePhyBase();

    /**
     * Destructor
     */
    ~LtePhyBase();

    const LteChannelModel* getPrimaryChannelModel()
    {
        return primaryChannelModel_;
    }

    const std::map<double, LteChannelModel*>* getChannelModels()
    {
        return &channelModel_;
    }

    LteChannelModel* getChannelModel(double carrierFreq = 0.0)
    {
        if (carrierFreq == 0.0) // when not specified, returns the first channel model (primary cell) TODO check this
        {
            std::map<double, LteChannelModel*>::iterator it = channelModel_.begin();
            if (it != channelModel_.end())
                return channelModel_.begin()->second;
        }

        if (channelModel_.find(carrierFreq) == channelModel_.end())
            return nullptr;

        return channelModel_[carrierFreq];
    }

    double getMicroTxPwr()
    {
        return microTxPower_;
    }
    double getMacroTxPwr()
    {
        return eNodeBtxPower_;
    }
    virtual double getTxPwr(Direction dir = UNKNOWN_DIRECTION)
    {
        return txPower_;
    }
    TxDirectionType getTxDirection()
    {
        return txDirection_;
    }
    double getTxAngle()
    {
        return txAngle_;
    }

  protected:

    /**
     * Performs initialization operations to prepare gates' IDs, analog models,
     * the decider and statistics.
     *
     * In stage 0 gets gates' IDs and a pointer to the world module.
     * It also get the CRNTI from RRC layer and initializes statistics
     * to be watched.
     * In stage 1 parses the xml file to fill the #analogModel list and
     * assign the #lteDecider_ pointer.
     *
     * @param stage initialization stage
     */
    virtual void initialize(int stage) override;

    virtual int numInitStages() const override {
        return std::max(inet::INITSTAGE_LAST+1, ChannelAccess::numInitStages());
    }

    /**
     * Processes messages received from #radioInGate_ or from the stack (#upperGateIn_).
     *
     * @param msg message received from stack or from air channel
     */
    virtual void handleMessage(omnetpp::cMessage *msg) override;

    /**
     * Sends a frame to all NICs in range.
     *
     * Frames are sent with zero transmission delay.
     */
    virtual void sendBroadcast(LteAirFrame *airFrame);

    /**
     * Sends a frame to the modules registered to the multicast group indicated in the frame
     *
     * Frames are sent with zero transmission delay.
     */
    virtual void sendMulticast(LteAirFrame *frame);

    /**
     * Sends a frame uniquely to the dest specified in carried control info.
     *
     * Delay is calculated based on sender's and receiver's positions.
     */
    virtual void sendUnicast(LteAirFrame *airFrame);

  protected:

    /**
     * Sends the given message to the wireless channel.
     *
     * Called by the handleMessage() method
     * when a message from #upperGateIn_ gate is received.
     *
     * The message is encapsulated into an LteAirFrame to which
     * a Signal object containing info about TX power, bit-rate and
     * move pattern is attached.
     * The LteAirFrame is then sent to the wireless channel.
     *
     * @param msg packet received from LteStack
     */
    virtual void handleUpperMessage(omnetpp::cMessage* msg);

    /**
     * Processes messages received from the wireless channel.
     *
     * Called by the handleMessage() method
     * when a message from #radioInGate_ is received.
     *
     * TODO Needs Work
     *
     * #####################################################################
     * This function handles the Airframe by performing following steps:
     * - If airframe is a broadcast/feedback packet and host is
     *   an UE attached to eNB or eNB calls the appropriate
     *   function of the DAS filter
     * - If airframe is received by a UE attached to a Relay
     *   it leaves the received signal unchanged
     * - If airframe is received by eNodeB it performs a loop over
     *   the remoteset written inside the control info and for each
     *   Remote changes the destination (current move variable) with
     *   the remote one before calling filterSignal().
     * - If airframe is received by UE attached to eNB it performs a loop over
     *   the remoteset written inside the control info and for each
     *   Remote changes the source (written inside the signal) with
     *   the remote one before calling filterSignal().
     * At the end only one packet is delivered to the upper layer
     * #####################################################################
     *
     * The analogModels prepared during the initialization phase are
     * applied to the Signal object carried with the received LteAirFrame.
     * Then the decider processes the frame which is sent out to #upperGateOut_
     * gate along with the decider's result (attached as a control info).
     *
     * @param msg LteAirFrame received from the air channel
     */
    virtual void handleAirFrame(omnetpp::cMessage* msg) = 0;

    virtual void handleSelfMessage(omnetpp::cMessage *msg) = 0;

    virtual void handleControlMsg(LteAirFrame *frame, UserControlInfo *userInfo);

    void initializeChannelModel();


    /**
     * Utility.
     * Shows current statistics above the icon.
     */
    void updateDisplayString();

    /**
     * Simple utility function to create a broadcast message for handover.
     */
    LteAirFrame *createHandoverMessage();

    /**
     * Returns the pointer to the AMC module, given a master ID (ENODEB)
     */
    LteAmc *getAmcModule(MacNodeId id);

    /**
     * Determine radio gate index of receiving node
     */
    int getReceiverGateIndex(const omnetpp::cModule*, bool isNr) const;

  public:
    /*
     * Returns the current position of the node
     */
    const inet::Coord& getCoord() { return getRadioPosition(); }
    /*
     * Returns the time of the last transmission performed
     */
    omnetpp::simtime_t getLastActive() { return lastActive_; }
    /*
     * Returns the MAC Node Id
     */
    MacNodeId getMacNodeId() { return nodeId_; }
};

#endif  /* _LTE_AIRPHYBASE_H_ */
