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

#ifndef _LTE_LTEMACENB_H_
#define _LTE_LTEMACENB_H_

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/cellInfo/CellInfo.h"
#include "simu5g/stack/mac/LteMacBase.h"
#include "simu5g/stack/mac/amc/LteAmc.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/background/trafficGenerator/IBackgroundTrafficManager.h"
#include "simu5g/stack/mac/DrbQosProfile.h"
#include "simu5g/stack/mac/UlBacklogRegistry.h"

namespace simu5g {

using namespace omnetpp;

class MacBsr;
class LteAmc;
class LteSchedulerEnbDl;
class LteSchedulerEnbUl;
class ConflictGraph;
class LteHarqProcessRx;

class LteMacEnb : public LteMacBase
{
  protected:
    /// Local CellInfo
    inet::ModuleRefByPar<CellInfo> cellInfo_;

    /// Lte AMC module
    LteAmc *amc_ = nullptr;

    /// List of scheduled users (one per carrier) - Downlink
    std::map<GHz, LteMacScheduleList> *scheduleListDl_ = nullptr;

    // For NR-SO DL cids: number of RLC PDUs requested this TTI (>1 = multiplexed into
    // one MAC PDU). The MAC PDU is built only once all of them have arrived from RLC.
    std::map<MacCid, unsigned int> soExpectedSdus_;

    int eNodeBCount;

    /// Reference to the background traffic manager
    std::map<GHz, IBackgroundTrafficManager *> bgTrafficManager_;

    /*******************************************************************************************/

    /// Number of RACH preambles for contention-based random access
    int numPreambles_ = 64;

    /// Pending RAC requests received during this TTI, grouped by preamble index.
    /// Resolved at the start of handleSelfMessage() to detect collisions.
    std::map<int, std::vector<inet::Packet *>> pendingRacRequests_;

    /// Reported uplink backlog, as buffer status reports deliver it. Keys are
    /// pseudo-connections: (ueId, BSR_UL_LCID_BASE + lcg) for the per-LCG figures
    /// of an uplink report, (ueId, D2D_SHORT_BSR / D2D_MULTI_SHORT_BSR) for the
    /// single figure of a D2D-typed report.
    UlBacklogRegistry ulBacklog_;

    /// Lte Mac Scheduler - Downlink
    LteSchedulerEnbDl *enbSchedulerDl_ = nullptr;

    /// Lte Mac Scheduler - Uplink
    LteSchedulerEnbUl *enbSchedulerUl_ = nullptr;

    /// Number of HARQ processes needing retransmission, per carrier and direction
    std::map<GHz, std::map<Direction, int>> needRtx_;

    /// DRB QoS map (DrbKey -> QoS profile), pushed by RRC (for QoS-aware scheduling)
    std::map<DrbKey, DrbQosProfile> drbQosMap_;

    /**
     * Reads MAC parameters for eNb and performs initialization.
     */
    void initialize(int stage) override;

    /**
     * Analyze gate of incoming packet
     * and call proper handler
     */
    void handleMessage(cMessage *msg) override;

    /**
     * Creates scheduling grants (one for each nodeId) according to the Schedule List.
     * It sends them to the lower layer.
     */
    virtual void sendGrants(std::map<GHz, LteMacScheduleList> *scheduleList);

    /// Blocks granted to one UE in one direction, by codeword: what the schedule
    /// entries of one carrier fold into, one map entry per grant to be sent.
    typedef std::map<std::pair<MacNodeId, Direction>, std::map<Codeword, unsigned int>> PerUeGrantBlocks;

    /**
     * foldScheduleEntries() folds one carrier's schedule entries into the grants
     * they stand for. A grant is the UE's allocation for the TTI -- the UE holds
     * ONE grant per carrier and its own LCP divides it among its channels -- while
     * the entries are the eNB's bookkeeping: one per backlog group, plus the RAC
     * entry of a grant issued for a BSR. Entries of one UE and one grant direction
     * therefore accumulate; entries of different directions (the D2D report types)
     * stay separate, since their grants differ.
     */
    virtual PerUeGrantBlocks foldScheduleEntries(const LteMacScheduleList& entries) const;

    /// What one folded (UE, direction) entry grants: the blocks of every codeword
    /// the UE was allocated, and how many codewords they span.
    struct GrantBlocks { unsigned int totalBlocks = 0; unsigned int codewords = 0; };

    /**
     * grantBlocksOf() sums one UE's per-codeword blocks into the two figures a
     * grant carries. A grant's codewords are dense: the UE reads granted bytes for
     * cw 0..codewords-1 (LteSchedulerUeUl::schedule), so an allocation that skipped
     * a codeword has no grant that expresses it, and is an error rather than a
     * silently truncated grant.
     */
    virtual GrantBlocks grantBlocksOf(MacNodeId nodeId, const std::map<Codeword, unsigned int>& cwBlocks) const;

    /// direction of the grant created by sendGrants() for a scheduled connection:
    /// derived from the BSR's logical CID, so that a grant answering a D2D BSR is
    /// issued for the D2D direction. Absent the D2D BSR LCIDs this is always UL,
    /// which is why a non-D2D cell cannot tell the difference.
    virtual Direction grantDirection(LogicalCid lcid) const { return directionFromBsrLcid(lcid, UL); }

    /// Length of the grant header prepended to the grant packet: one byte, the
    /// .msg file's own default.
    /// NB neither this nor the 1 bit the NR/D2D MACs used to set is a modelled
    /// DCI: a real UL grant travels in DCI format 0 / 0_0 on PDCCH and runs to
    /// several bytes depending on bandwidth. Simu5G does not model PDCCH resource
    /// usage either, so this is a placeholder that keeps the grant octet-aligned,
    /// not a spec value. Sizing it properly belongs with control-channel realism.
    virtual inet::b grantChunkLength() const { return inet::B(1); }

    /**
     * macPduMake() creates MAC PDUs (one for each CID)
     * by extracting SDUs from Real Mac Buffers according
     * to the Schedule List (stored after scheduling).
     * It sends them to H-ARQ.
     */
    void macPduMake(MacCid cid) override;

    /**
     * macPduUnmake() extracts SDUs from a received MAC
     * PDU and sends them to the upper layer.
     *
     * On ENB it also extracts the BSR Control Element
     * and stores it in the BSR buffer (for the cid from
     * which the packet was received).
     *
     * @param pkt container packet
     */
    void macPduUnmake(cPacket *pkt) override;

    /**
     * macSduRequest() sends a message to the RLC layer
     * requesting MAC SDUs (one for each CID),
     * according to the Schedule List.
     */
    virtual void macSduRequest();

    /**
     * bufferizeBsr() applies a received BSR control element to the UL backlog
     * registry. An uplink report (packet LCID SHORT_BSR) carries per-LCG figures
     * and updates one mirror per group; a D2D-typed report carries one figure
     * and updates its type's single mirror.
     *
     * @param bsr the received BSR control element (not retained)
     * @param lteInfo control info of the PDU that carried it (source, packet LCID)
     */
    virtual void bufferizeBsr(const MacBsr *bsr, const UserControlInfo *lteInfo);

    /**
     * bufferizePacket() is called every time a packet is
     * received from the upper layer.
     *
     * @param pkt Packet to be buffered
     * @return TRUE if the packet was buffered successfully, FALSE otherwise.
     */
    bool bufferizePacket(cPacket *pkt) override;

    /**
     * handleUpperMessage() is called every time a packet is
     * received from the upper layer.
     */
    void handleUpperMessage(cPacket *pkt) override;

    /**
     * Main loop.
     */
    void handleSelfMessage() override;
    /**
     * macHandleFeedbackPkt is called every time a feedback packet arrives on MAC.
     */
    void macHandleFeedbackPkt(cPacket *pkt) override;

    /*
     * Buffers incoming RAC requests for preamble collision detection.
     */
    void macHandleRac(cPacket *pkt) override;

    /*
     * Resolves buffered RAC requests: detects preamble collisions and sends
     * success/failure responses. Called at the start of each TTI.
     */
    virtual void resolveRacCollisions();

    /*
     * Update UserTxParam stored in every lteMacPdu when an RTX changes this information.
     */
    void updateUserTxParam(cPacket *pkt) override;

    /**
     * Flush Tx H-ARQ buffers for all users.
     */
    virtual void flushHarqBuffers();

  public:

    LteMacEnb();
    ~LteMacEnb() override;

    /// Returns the backlog mirror of a reported pseudo-connection.
    LteMacBuffer *getBsrVirtualBuffer(MacCid cid) { return ulBacklog_.mirror(cid); }

    /// Returns the pseudo-connections reports have created so far.
    std::vector<MacCid> getActiveBsrVirtualBufferCids() { return ulBacklog_.keys(); }

    /// The UE's uplink backlog mirrors, in logical channel group order.
    std::vector<LteMacBuffer *> getUlBacklogMirrors(MacNodeId nodeId) const { return ulBacklog_.ulMirrorsOf(nodeId); }

    /// Empties all backlog mirrors belonging to the given UE.
    virtual void clearBsrBuffers(MacNodeId ueId);

    /**
     * deleteQueues() on ENB performs actions
     * from the base class and also deletes the BSR buffer.
     *
     * @param nodeId id of node performing handover.
     */
    void deleteQueues(MacNodeId nodeId) override;

    /**
     * Getter for AMC module.
     */
    virtual LteAmc *getAmc()
    {
        return amc_;
    }

    /**
     * Getter for cellInfo.
     */
    virtual CellInfo *getCellInfo();

    /**
     * Getter for the backgroundTrafficManager.
     */
    virtual IBackgroundTrafficManager *getBackgroundTrafficManager(GHz carrierFrequency)
    {
        if (bgTrafficManager_.find(carrierFrequency) == bgTrafficManager_.end())
            throw cRuntimeError("LteMacEnb::getBackgroundTrafficManager - carrier frequency [%f] not valid.", carrierFrequency.get());
        return bgTrafficManager_[carrierFrequency];
    }

    /**
     * Returns the number of system antennas (MACRO included).
     */
    virtual int getNumAntennas();

    /**
     * Returns the scheduling discipline for the given direction.
     * @param dir link direction.
     */
    SchedDiscipline getSchedDiscipline(Direction dir);

    /*
     * Return the current active set (active connections).
     * @param direction
     */
    ActiveSet *getActiveSet(Direction dir);

    /*
     * Inform the base station that the given node will need a retransmission.
     */
    virtual void signalProcessForRtx(MacNodeId nodeId, GHz carrierFrequency, Direction dir, bool rtx = true);

    /*
     * Get the number of nodes requesting retransmissions for the given carrier and direction.
     */
    virtual int getProcessForRtx(GHz carrierFrequency, Direction dir);

    /// Total number of HARQ processes needing retransmission on this carrier in all directions except 'excluded'
    virtual int getPendingRtxOtherThan(GHz carrierFrequency, Direction excluded);

    // Get band occupation for this/previous TTI. Used for interference computation purposes.
    unsigned int getDlBandStatus(Band b);
    unsigned int getDlPrevBandStatus(Band b);

    // Configuration push: RRC installs (or replaces) a bearer's QoS profile. MAC does
    // not author its own configuration; this is the only write path into the map.
    virtual void configureDrbQos(DrbKey key, const DrbQosProfile& qos);

    // Get DRB QoS map (DrbKey -> QoS profile), pushed by RRC
    const std::map<DrbKey, DrbQosProfile> *getDrbQosMap() {
        return drbQosMap_.empty() ? nullptr : &drbQosMap_;
    }

    /*
     * @author Alessandro Noferi
     * Gets percentage of block utilized during the last TTI.
     * @param dir UL or DL.
     */
    double getUtilization(Direction dir);

    /* Gets the number of active users based on the direction.
     * A user is active (according to TS 136 314) if:
     * - it has buffered data in MAC RLC or PDCP layers -> ActiveSet.
     * - it has data for which HARQ transmission has not yet terminated -> !EMPTY HarqBuffer.
     *
     * @param direction
     */
    int getActiveUesNumber(Direction dir);

};

} //namespace

#endif
