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

#ifndef _LTE_LTEMACUE_H_
#define _LTE_LTEMACUE_H_

#include "simu5g/stack/mac/LteMacBase.h"
#include "simu5g/stack/phy/PhyBase.h"
#include "simu5g/stack/mac/buffer/harq/LteHarqBufferTx.h"
#include "simu5g/stack/phy/feedback/LteFeedback.h"

namespace simu5g {

using namespace omnetpp;

class LteSchedulingGrant;
class LteSchedulerUeUl;
class LcgScheduler;
class Binder;
class ChannelModelBase;

class LteMacUe : public LteMacBase
{
  protected:
    // false if currentHarq_ counter needs to be initialized
    bool firstTx = false;

    // one per carrier
    std::map<GHz, LteSchedulerUeUl *> lcgScheduler_;

    // NR-SO: per-cid continuation state of each SO connection's front SDU, SHARED across
    // the per-carrier UL schedulers (they all drain the same RLC TX buffer), so a carrier
    // sees segmentation done by another carrier and reserves the matching header.
    std::map<MacCid, bool> soFrontIsContinuation_;

    // configured grant - one for each codeword
    std::map<GHz, inet::IntrusivePtr<const LteSchedulingGrant>> schedulingGrant_;

    /// List of scheduled connections for this UE
    std::map<GHz, LteMacScheduleList *> scheduleList_;

    // current H-ARQ process counter
    unsigned char currentHarq_ = 0;

    // periodic grant handling - one per carrier
    std::map<GHz, unsigned int> periodCounter_;
    std::map<GHz, unsigned int> expirationCounter_;

    // number of MAC SDUs requested to the RLC
    int requestedSdus_ = 0;

    bool debugHarq_ = false;

    // RAC and BSR configuration
    // TODO adjust C++ names to match NED parameter names
    int numPreambles_ = 64;
    unsigned int maxRacTryouts_ = 0;
    unsigned int minRacBackoff_ = 0;
    unsigned int maxRacBackoff_ = 0;
    unsigned int raRespWinStart_ = 0;
    unsigned int bsrRtxTimerStart_ = 0;

    // RAC handling state
    bool racRequested_ = false;
    unsigned int racBackoffTimer_ = 0;
    unsigned int currentRacTry_ = 0;
    unsigned int raRespTimer_ = 0;

    // BSR handling
    unsigned int bsrRtxTimer_ = 0;
    bool bsrTriggered_ = false;

    /**
     * Reads MAC parameters for UE and performs initialization.
     */
    void initialize(int stage) override;

    /**
     * Analyze gate of incoming packets
     * and call proper handler
     */
    void handleMessage(cMessage *msg) override;

    /**
     * macSduRequest() sends a message to the RLC layer
     * requesting MAC SDUs (one for each CID),
     * according to the Schedule List.
     */
    virtual int macSduRequest();

    /**
     * bufferizePacket() is called every time a packet is
     * received from the upper layer
     */
    bool bufferizePacket(cPacket *pkt) override;

    /**
     * macPduMake() creates MAC PDUs (one for each CID)
     * by extracting SDUs from Real Mac Buffers according
     * to the Schedule List.
     * It sends them to H-ARQ (at the moment lower layer)
     *
     * On UE it also adds a BSR control element to the MAC PDU
     * containing the size of its buffer (for that CID)
     */
    void macPduMake(MacCid cid = MacCid()) override;

    /// Factory for the TX HARQ buffer created upon first transmission towards a node.
    /// The base implementation supports the UL direction only (buffer paired with the serving cell's MAC).
    virtual LteHarqBufferTx *createTxHarqBuffer(MacNodeId destId, Direction dir);

    /// Whether a BSR is waiting to be sent (checked in macPduMake() before appending a BSR control element).
    virtual bool isBsrPending() const { return bsrTriggered_; }

    /// The buffer occupancy a BSR reports: every UL connection's virtual-buffer
    /// backlog plus, per connection with backlog, its RLC header. Used by both
    /// reporting paths (the standalone BSR-only PDU and the piggybacked control
    /// element), which must agree on what "the UE's backlog" means.
    virtual int64_t computeUlBsrSize() const;

    // ---- macPduMake() seams ----
    // macPduMake() is shared by every UE MAC (LTE, NR and the D2D mixin); the
    // points where those differ are the virtuals below. Each default is the plain
    // LTE UE's behavior.

    /// Builds a standalone BSR-only PDU when a grant arrives with an empty schedule
    /// list, and returns whether it did; if so, macPduMake() skips the SDU loop.
    /// This is what handleSelfMessage() calls macPduMake() for in that situation.
    virtual bool buildStandaloneBsr();

    /// The destination of an UL MAC PDU carrying this connection. Needed before the
    /// PDU exists, since it keys macPduList_. At an LTE UE it is always the cell.
    virtual MacNodeId pduDestId(MacCid destCid) { return getMacCellId(); }

    /// Whether a schedule-list entry that carries no SDUs should be skipped.
    virtual bool shouldSkipScheduleEntry(unsigned int sduPerCid) const { return false; }

    /// Creates an UL MAC PDU and fills in its control info.
    virtual inet::Packet *createUlMacPdu(MacCid destCid, GHz carrierFreq, MacNodeId destId);

    /// Appends a BSR control element reporting the given buffer occupancy to the MAC PDU
    /// and resets the BSR trigger state. Called from macPduMake() when isBsrPending().
    virtual void appendBsr(inet::Ptr<LteMacPdu> macPdu, int size);

    /// Whether it is this carrier's turn in the current TTI. The LTE MAC serves
    /// every carrier every TTI; the NR MAC overrides this with the numerology
    /// period check.
    virtual bool isCarrierActive(GHz carrierFrequency) { return true; }

    /// H-ARQ TX unit reservation policy used by macPduMake(): the LTE MAC uses
    /// the synchronous current process; the NR MAC picks the first available one.
    virtual UnitList reserveTxHarqUnits(LteHarqBufferTx *txBuf, Direction dir);

    /// end-of-main-loop purge of corrupted PDUs from the RX H-ARQ buffers
    /// (default: none; the D2D MAC purges its DL buffer only, so that mirror
    /// buffers for D2D communication stay intact)
    virtual void purgeRxHarqBuffers() {}

    // Scheduling bookkeeping: true when no carrier produced a schedule this TTI.
    // Written at the top of the UL scheduling block in every UE main loop (LTE, NR
    // and D2D) and read only downstream of that write: directly below it, and from
    // macPduMake(), whose only two call sites are that same block and the
    // all-SDUs-arrived path, which is reachable only via the macSduRequest() in it.
    // The initial value is therefore inert -- verified by running the full suite
    // with it set to true, which reproduced every fingerprint. It is initialized
    // anyway: reading an indeterminate bool is UB the moment that dominance is
    // broken, and false keeps a UE that ever got there on the normal SDU-request
    // path instead of emitting a spurious BSR.
    bool emptyScheduleList_ = false;

    /**
     * macPduUnmake() extracts SDUs from a received MAC
     * PDU and sends them to the upper layer.
     *
     * @param pkt container packet
     */
    void macPduUnmake(cPacket *pkt) override;

    /**
     * handleUpperMessage() is called every time a packet is
     * received from the upper layer
     */
    void handleUpperMessage(cPacket *pkt) override;

    /**
     * Main loop
     */
    void handleSelfMessage() override;

    /*
     * Receives and handles scheduling grants
     */
    void macHandleGrant(cPacket *pkt) override;

    /*
     * Receives and handles RAC responses
     */
    void macHandleRac(cPacket *pkt) override;

    /*
     * Checks RAC status
     */
    virtual void checkRAC();
    /*
     * Update UserTxParam stored in every lteMacPdu when an RTX changes this information
     */
    void updateUserTxParam(cPacket *pkt) override;

    /**
     * Flush Tx H-ARQ buffers for the user
     */
    virtual void flushHarqBuffers();

  public:
    LteMacUe();
    ~LteMacUe() override;

    // NR-SO: per-cid front-SDU continuation state, shared across the per-carrier UL
    // schedulers so they agree with the single shared RLC TX buffer.
    std::map<MacCid, bool>& getSoContinuationMap() { return soFrontIsContinuation_; }

    /// Factory for the LCG scheduler used by this UE's uplink scheduler (LteSchedulerUeUl).
    virtual LcgScheduler *createLcgScheduler();

    /*
 * Access scheduling grant
 */
inline const LteSchedulingGrant *getSchedulingGrant(GHz carrierFrequency) const
{
    if (schedulingGrant_.find(carrierFrequency) == schedulingGrant_.end())
        return nullptr;
    return schedulingGrant_.at(carrierFrequency).get();
}

    /*
     * Access current H-ARQ pointer
     */
    inline const unsigned char getCurrentHarq() const
    {
        return currentHarq_;
    }

    /*
     * Access BSR trigger flag
     */
    inline const bool bsrTriggered() const
    {
        return bsrTriggered_;
    }

    /*
     * Cancel a pending buffer status report trigger
     */
    void cancelBsr()
    {
        bsrTriggered_ = false;
    }

    /* utility functions used by LCP scheduler
     * <cid> and <priority> are returned by reference
     * @return true if at least one backlogged connection exists
     */
    virtual bool getHighestBackloggedFlow(MacCid& cid, unsigned int& priority);
    virtual bool getLowestBackloggedFlow(MacCid& cid, unsigned int& priority);

    /**
     * deleteQueues() must be called on handover
     * to delete queues for a given user
     *
     * @param nodeId Id of the node whose queues are deleted
     */
    void deleteQueues(MacNodeId nodeId) override;

    // update ID of the serving cell during handover
    virtual void doHandover(MacNodeId targetEnb);
};

} //namespace

#endif
