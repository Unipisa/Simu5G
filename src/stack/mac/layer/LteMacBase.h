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

#ifndef _LTE_LTEMACBASE_H_
#define _LTE_LTEMACBASE_H_

#include <inet/common/ModuleRefByPar.h>

#include "common/binder/Binder.h"
#include "common/LteCommon.h"
#include "common/LteControlInfo.h"

namespace simu5g {

using namespace omnetpp;

class LteHarqBufferTx;
class LteHarqBufferRx;
class Binder;
class FlowControlInfo;
class LteMacBuffer;
class PacketFlowManagerBase;

/**
 * Map associating a nodeId with the corresponding TX H-ARQ buffer.
 * Used in eNB, where there is more than one TX H-ARQ buffer.
 */
typedef std::map<MacNodeId, LteHarqBufferTx *> HarqTxBuffers;

/**
 * Map associating a nodeId with the corresponding RX H-ARQ buffer.
 * Used in eNB, where there is more than one RX H-ARQ buffer.
 */
typedef std::map<MacNodeId, LteHarqBufferRx *> HarqRxBuffers;

/*
 * MultiMap associating a LCG group with all connections belonging to it and
 * corresponding virtual buffer pointer
 */
typedef std::pair<MacCid, LteMacBuffer *> CidBufferPair;
typedef std::pair<LteTrafficClass, CidBufferPair> LcgPair;
typedef std::multimap<LteTrafficClass, CidBufferPair> LcgMap;

/**
 * @class LteMacBase
 * @brief MAC Layer
 *
 * This is the MAC layer of LTE Stack:
 * it performs buffering/sending packets.
 *
 * On each TTI, the handleSelfMessage() is called
 * to perform scheduling and other tasks
 */
class LteMacBase : public cSimpleModule
{
    friend class LteHarqBufferTx;
    friend class LteHarqBufferRx;
    friend class LteHarqBufferTxD2D;
    friend class LteHarqBufferRxD2D;

  protected:

    unsigned int totalOverflowedBytes_;
    static simsignal_t macBufferOverflowDlSignal_;
    static simsignal_t macBufferOverflowUlSignal_;
    static simsignal_t macBufferOverflowD2DSignal_;
    static simsignal_t receivedPacketFromUpperLayerSignal_;
    static simsignal_t receivedPacketFromLowerLayerSignal_;
    static simsignal_t sentPacketToUpperLayerSignal_;
    static simsignal_t sentPacketToLowerLayerSignal_;

    /*
     * Data Structures
     */
    inet::ModuleRefByPar<Binder> binder_;

    /*
     * Gates
     */
    cGate *upInGate_ = nullptr;     /// MAC <-- RLC
    cGate *upOutGate_ = nullptr;    /// MAC --> RLC
    cGate *downInGate_ = nullptr;   /// MAC <-- PHY
    cGate *downOutGate_ = nullptr;  /// MAC --> PHY

    /*
     * MAC MIB Params
     */
    bool muMimo_;

    int harqProcesses_;

    /// TTI self message
    cMessage *ttiTick_ = nullptr;

    /// TTI for this node
    double ttiPeriod_ = TTI;

    /// MacNodeId
    MacNodeId nodeId_;

    opp_component_ptr<cModule> networkNode_;

    /// MacCellId
    MacCellId cellId_;

    /// Mac Buffers maximum queue size
    unsigned int queueSize_;

    /// Mac Sdu Real Buffers
    LteMacBuffers mbuf_;

    /// Mac Sdu Virtual Buffers
    LteMacBufferMap macBuffers_;

    /// List of pdus finalized for each user on each codeword (one entry per carrier)
    std::map<double, MacPduList> macPduList_;

    /// Harq Tx Buffers (one entry per carrier)
    std::map<double, HarqTxBuffers> harqTxBuffers_;

    /// Harq Rx Buffers (one entry per carrier)
    std::map<double, HarqRxBuffers> harqRxBuffers_;

    /* Connection Descriptors
     * Holds flow-related information
     */
    std::map<MacCid, FlowControlInfo> connDesc_;

    /* Incoming Connection Descriptors:
     * a connection is stored at the first MAC SDU delivered to the RLC
     */
    std::map<MacCid, FlowControlInfo> connDescIn_;

    /* LCG to CID and buffers map - used for supporting LCG - based scheduler operations
     * TODO: delete/update entries on handover
     */
    LcgMap lcgMap_;
    // Node Type;
    RanNodeType nodeType_;

    // record the last TTI that HARQ processes for a given UE have been aborted (useful for D2D switching)
    std::map<MacNodeId, simtime_t> resetHarq_;

    // reference to the phy layer
    opp_component_ptr<LtePhyBase> phy_;

    // @author Alessandro Noferi
    // reference to the packetFlowManager
    inet::ModuleRefByPar<PacketFlowManagerBase> packetFlowManager_;

    // support to different numerologies
    struct NumerologyPeriodCounter {
        unsigned int max;
        unsigned int current;
    };
    std::map<NumerologyIndex, NumerologyPeriodCounter> numerologyPeriodCounter_;

    unsigned int getNumerologyPeriodCounter(NumerologyIndex index) { return numerologyPeriodCounter_[index].current; }
    void decreaseNumerologyPeriodCounter();

    // statistics in visualization
    bool statDisplay_;
    uint64_t nrFromUpper_;
    uint64_t nrFromLower_;
    uint64_t nrToUpper_;
    uint64_t nrToLower_;

    // support to print harqErrorRate at the end of the simulation
    unsigned int totalHarqErrorRateDlSum_ = 0;
    unsigned int totalHarqErrorRateUlSum_ = 0;
    unsigned int totalHarqErrorRateDlCount_ = 0;
    unsigned int totalHarqErrorRateUlCount_ = 0;

  public:

    /**
     * Initializes MAC Buffers
     */

    /**
     * Deletes MAC Buffers
     */
    ~LteMacBase() override;

    /**
     * deleteQueues() must be called on handover
     * to delete queues for a given user
     *
     * @param nodeId Id of the node whose queues are deleted
     */
    virtual void deleteQueues(MacNodeId nodeId);

    //* public utility function - drops ownership of an object
    void dropObj(cOwnedObject *obj)
    {
        drop(obj);
    }

    //* public utility function - takes ownership of an object
    void takeObj(cOwnedObject *obj)
    {
        take(obj);
    }

    /*
     * Getters
     */

    double getTtiPeriod()
    {
        return ttiPeriod_;
    }

    LtePhyBase *getPhy()
    {
        return phy_;
    }

    MacNodeId getMacNodeId()
    {
        return nodeId_;
    }

    MacCellId getMacCellId()
    {
        return cellId_;
    }

    // Returns the virtual buffers
    LteMacBufferMap *getMacBuffers()
    {
        return &macBuffers_;
    }

    // Returns Traffic Class to cid mapping
    LcgMap& getLcgMap()
    {
        return lcgMap_;
    }

    // Returns connection descriptors
    std::map<MacCid, FlowControlInfo>& getConnDesc()
    {
        return connDesc_;
    }

    // Returns the harq tx buffers
    std::map<double, HarqTxBuffers> *getHarqTxBuffers()
    {
        return &harqTxBuffers_;
    }

    // Returns the harq rx buffers
    std::map<double, HarqRxBuffers> *getHarqRxBuffers()
    {
        return &harqRxBuffers_;
    }

    // Returns the harq tx buffers for the given carrier
    HarqTxBuffers *getHarqTxBuffers(double carrierFrequency)
    {
        if (harqTxBuffers_.find(carrierFrequency) == harqTxBuffers_.end())
            return nullptr;
        return &harqTxBuffers_[carrierFrequency];
    }

    // Returns the harq rx buffers for the given carrier
    HarqRxBuffers *getHarqRxBuffers(double carrierFrequency)
    {
        if (harqRxBuffers_.find(carrierFrequency) == harqRxBuffers_.end())
            return nullptr;
        return &harqRxBuffers_[carrierFrequency];
    }

    // Returns number of Harq Processes
    unsigned int harqProcesses() const
    {
        return harqProcesses_;
    }

    // Returns the MU-MIMO enabled flag
    bool muMimo() const
    {
        return muMimo_;
    }

    RanNodeType getNodeType()
    {
        return nodeType_;
    }

    virtual bool isD2DCapable()
    {
        return false;
    }

    // check whether HARQ processes have been aborted during this TTI
    bool isHarqReset(MacNodeId srcId)
    {
        if (resetHarq_.find(srcId) != resetHarq_.end()) {
            if (resetHarq_[srcId] == NOW)
                return true;
        }
        return false;
    }

    void unregisterHarqBufferRx(MacNodeId nodeId);

    // visualization
    void refreshDisplay() const override;

    void recordHarqErrorRate(unsigned int sample, Direction dir);
    double getHarqErrorRate(Direction dir);

    /*
     * @author Alessandro Noferi
     *
     * methods called by mac layer and the HarqBuffers to notify
     * MAC pdus events to packetFlowManager
     */
    virtual void insertMacPdu(const inet::Packet *macPdu);
    virtual void harqAckToFlowManager(inet::Ptr<const UserControlInfo> lteInfo, inet::Ptr<const LteMacPdu> macPdu);
    virtual void discardMacPdu(const inet::Packet *macPdu);
    virtual void discardRlcPdu(inet::IntrusivePtr<const UserControlInfo> lteInfo, unsigned int rlcSno);

  protected:

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    /**
     * Grabs NED parameters, initializes gates
     * and the TTI self message
     */
    void initialize(int stage) override;

    /**
     * Analyze gate of incoming packet
     * and call proper handler
     */
    void handleMessage(cMessage *msg) override;

    /**
     * Statistics recording
     */
    void finish() override;

    /**
     * Deleting the module
     *
     * Method is overridden to cancel the periodic TTI self-message,
     * afterwards the deleteModule method of cSimpleModule is called.
     */
    void deleteModule() override;

    /**
     * Main loop of the Mac level, calls the scheduler
     * and every other function every TTI: must be reimplemented
     * by derived classes
     */
    virtual void handleSelfMessage() = 0;

    /**
     * sendLowerPackets() is used
     * to send packets to lower layer
     *
     * @param pkt Packet to send
     */
    void sendLowerPackets(cPacket *pkt);

    /**
     * sendUpperPackets() is used
     * to send packets to upper layer
     *
     * @param pkt Packet to send
     */
    void sendUpperPackets(cPacket *pkt);

    /*
     * Functions to be redefined by derived classes
     */

    virtual void macPduMake(MacCid cid = 0) = 0;
    virtual void macPduUnmake(cPacket *pkt) = 0;

    /**
     * bufferizePacket() is called every time a packet is
     * received from the upper layer
     */
    virtual bool bufferizePacket(cPacket *pktAux);

    /**
     * handleUpperMessage() is called every time a packet is
     * received from the upper layer
     */
    virtual void handleUpperMessage(cPacket *pkt)
    {
        bufferizePacket(pkt);
    }

    /**
     * macHandleFeedbackPkt is called every time a feedback pkt arrives on MAC
     */
    virtual void macHandleFeedbackPkt(cPacket *pkt)
    {
    }

    /*
     * Receives and handles scheduling grants - implemented in LteMacUe
     */
    virtual void macHandleGrant(cPacket *pkt)
    {
    }

    /*
     * Receives and handles RAC requests (eNodeB implementation) and responses (LteMacUe implementation)
     */
    virtual void macHandleRac(cPacket *pkt)
    {
    }

    /*
     * Update UserTxParam stored in every lteMacPdu when an rtx changes this information
     */
    virtual void updateUserTxParam(cPacket *pkt) = 0;

    /// Upper Layer Handler
    void fromRlc(cPacket *pkt);

    /// Lower Layer Handler
    virtual void fromPhy(cPacket *pkt);
};

} //namespace

#endif

