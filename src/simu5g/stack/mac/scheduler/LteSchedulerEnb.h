//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
// Editor: Mohamed Seliem (University College Cork)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTESCHEDULERENB_H_
#define _LTE_LTESCHEDULERENB_H_

#include "simu5g/common/LteCommon.h"
#include "simu5g/stack/mac/buffer/harq/LteHarqBufferTx.h"
#include "simu5g/stack/mac/allocator/LteAllocatorUtils.h"
#include "simu5g/stack/mac/LteMacEnb.h"

namespace simu5g {

using namespace omnetpp;

/// forward declarations
class LteScheduler;
class LteAllocationModule;
class LteMacEnb;

/**
 */
class LteSchedulerEnb : public cSimpleModule
{
    /******************
    * Friend classes
    ******************/

    // Scheduling disciplines reach the orchestrator's protected state (mac_, the
    // allocator bookkeeping, the band-limit and retransmission helpers) through their
    // eNbScheduler_ back-pointer. Friendship is NOT inherited, so deriving from
    // LteScheduler does not carry its grant: every discipline needs its own line here.
    // That is why this list grows with each added discipline, and why an entry cannot
    // be dropped without promoting the members it uses to public.
    friend class LteScheduler;
    friend class LteDrr;
    friend class LtePf;
    friend class LteMaxCi;
    friend class LteMaxCiMultiband;
    friend class LteMaxCiOptMB;
    friend class LteMaxCiComp;
    friend class QoSAwareScheduler;

    // The two D2D entries below are the only places the core NAMES a class from the
    // D2D package. This is a name only -- no include, no NED import, no type lookup --
    // so the build dependency stays one-way and the core still compiles with the
    // Simu5G_D2D feature disabled.
    //
    // LteAllocatorBestFit is a LteScheduler subclass like the disciplines above, and
    // needs the same kind of access: mac_, allocatedCws(), getOccupiedBands(),
    // storeAllocationEnb(), storeScListId().
    friend class LteAllocatorBestFit;
    // D2dRtxScheduling is not a discipline but a standalone helper shared by the D2D
    // uplink schedulers; it holds an LteSchedulerEnb* and touches exactly three
    // protected members: mac_, applyAllowedBandLimits() and allocateRtxBytes().
    friend class D2dRtxScheduling;

  protected:

    /*******************
    * Data structures
    *******************/

    // booked requests list : Band, bytes, blocks
    struct Request
    {
        Band b_;
        unsigned int bytes_;
        unsigned int blocks_;

        Request(Band b, unsigned int bytes, unsigned int blocks) : b_(b), bytes_(bytes), blocks_(blocks)
        {
        }

        Request()
        {
            Request(0, 0, 0);
        }
    };

    // Owner MAC module. Set via initialize().
    opp_component_ptr<LteMacEnb> mac_;

    // Reference to the LTE Binder
    opp_component_ptr<Binder> binder_;

    // System allocator, carries out the block-allocation functions.
    LteAllocationModule *allocator_ = nullptr;

    // Scheduling agent. One per carrier
    std::vector<LteScheduler *> scheduler_;

    // Operational Direction. Set via initialize().
    Direction direction_ = DL;

    //! Set of active connections.
    ActiveSet activeConnectionSet_;

    // Schedule list. One per carrier
    std::map<GHz, LteMacScheduleList> scheduleList_;

    // For NR-SO (no RLC concatenation) DL connections: the per-PDU payload sizes that
    // fill the grant, so the gNB MAC issues one SDU request per planned PDU and the MAC
    // PDU multiplexes them. Empty/absent for LTE-FI connections (one concatenated PDU).
    std::map<MacCid, std::vector<unsigned int>> scheduledSoPduSizes_;

    // NR-SO: per-DL-connection flag for whether the front SDU is a continuation, so the
    // scheduler reserves the same segment-state header the RLC TX emits.
    std::map<MacCid, bool> soFrontIsContinuation_;

    // Codeword list
    LteMacAllocatedCws allocatedCws_;

    // Pointer to Harq Tx Buffers (that are in LteMacBase)
    std::map<GHz, HarqTxBuffers> *harqTxBuffers_ = nullptr;

    // Pointer to Harq Rx Buffers (that are in LteMacBase)
    std::map<GHz, HarqRxBuffers> *harqRxBuffers_ = nullptr;

    /// Total available resource blocks (switch on direction)
    /// Initialized by LteMacEnb::handleSelfMessage() using resourceBlocks()
    unsigned int resourceBlocks_ = 0;

    /// Statistics
    static simsignal_t avgServedBlocksDlSignal_;
    static simsignal_t avgServedBlocksUlSignal_;

    // pre-made BandLimit structure used when no band limit is given to the scheduler
    std::vector<BandLimit> emptyBandLim_;

    // @author Alessandro Noferi
    double utilization_ = 0; // it records the utilization in the last TTI

  public:

    /**
     * Default constructor.
     */
    LteSchedulerEnb() {}

    /**
     * Destructor.
     */
    virtual ~LteSchedulerEnb();

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override { throw cRuntimeError("LteSchedulerEnb does not handle messages"); }

    virtual Direction getDirection() const = 0;

  public:

    /*
     * Initialize counters for schedulers
     */
    virtual void initializeSchedulerPeriodCounter(NumerologyIndex maxNumerologyIndex);

    /**
     * Schedule data. Returns one schedule list per carrier
     * @param list
     */
    virtual std::map<GHz, LteMacScheduleList> *schedule();

    /* After scheduling, returns the planned per-PDU payload sizes for an NR-SO DL
     * connection (one SDU request per entry), or nullptr for LTE-FI connections.
     */
    virtual const std::vector<unsigned int> *getScheduledSoPduSizes(MacCid cid) const
    {
        auto it = scheduledSoPduSizes_.find(cid);
        return it != scheduledSoPduSizes_.end() ? &it->second : nullptr;
    }

    /**
     * Adds an entry (if not already in) to scheduling list.
     * The function calls the LteScheduler notify().
     * @param cid connection identifier
     */
    virtual void backlog(MacCid cid);

    /**
     * Get/Set current available Resource Blocks.
     */
    unsigned int& resourceBlocks()
    {
        return resourceBlocks_;
    }

    /**
     * Gets the amount of the system resource block
     */
    unsigned int getResourceBlocks()
    {
        return resourceBlocks_;
    }

    /**
     * Gets the utilization in the last TTI
     */
    double getUtilization()
    {
        return utilization_;
    }

    /**
     * Returns the number of blocks allocated by the UE on the given antenna
     * into the logical band provided.
     */
    virtual unsigned int readPerUeAllocatedBlocks(const MacNodeId nodeId, const Remote antenna, const Band b);

    /**
     * Returns the number of available blocks for the UE on the given antenna
     * in the logical band provided.
     */
    virtual unsigned int readAvailableRbs(const MacNodeId id, const Remote antenna, const Band b);

    /**
     * Returns the number of available blocks.
     */
    virtual unsigned int readTotalAvailableRbs();

    /**
     * Resource Block IDs computation function.
     */
    virtual unsigned int readRbOccupation(const MacNodeId id, GHz carrierFrequency, RbMap& rbMap);

    /**
     * Schedules retransmission for the Harq Process of the given UE on a set of logical bands.
     * Each band also has an assigned limit amount of bytes: no more than the specified
     * amount will be served on the given band for the acid.
     *
     * @param nodeId The node ID
     * @param cw The codeword used to serve the acid process
     * @param bands A vector of logical bands
     * @param acid The ACID
     * @return The allocated bytes. 0 if retransmission was not possible
     */
    virtual unsigned int schedulePerAcidRtx(MacNodeId nodeId, GHz carrierFrequency, Codeword cw, unsigned char acid,
            std::vector<BandLimit> *bandLim = nullptr, Remote antenna = MACRO, bool limitBl = false) = 0;

    /// Shared UL/DL implementation; the UL-interference bookkeeping inside is
    /// active only for the uplink scheduler (direction_ == UL).
    virtual unsigned int scheduleBgRtx(MacNodeId bgUeId, GHz carrierFrequency, Codeword cw, std::vector<BandLimit> *bandLim = nullptr,
            Remote antenna = MACRO, bool limitBl = false);

    /**
     * Schedules capacity for a given connection without effectively performing the operation on the
     * real downlink/uplink buffer: instead, it performs the operation on a virtual buffer,
     * which is used during the finalize() operation in order to commit the decision performed
     * by the grant function.
     * Each band also has an assigned limit amount of bytes: no more than the
     * specified amount will be served on the given band for the cid
     *
     * @param cid Identifier of the connection that is granted capacity
     * @param antenna the DAS remote antenna
     * @param bands The set of logical bands with their related limits
     * @param bytes Grant size, in bytes
     * @param terminate Set to true if scheduling has to stop now
     * @param active Set to false if the current queue becomes inactive
     * @param eligible Set to false if the current queue becomes ineligible
     * @param limitBl if true bandLim vector express the limit of allocation for each band in blocks
     * @return The number of bytes that have been actually granted.
     */
    virtual unsigned int scheduleGrant(MacCid cid, unsigned int bytes, bool& terminate, bool& active, bool& eligible, GHz carrierFrequency,
            BandLimitVector *bandLim = nullptr, Remote antenna = MACRO, bool limitBl = false);

    virtual unsigned int scheduleGrantBackground(MacCid bgCid, unsigned int bytes, bool& terminate, bool& active, bool& eligible, GHz carrierFrequency,
            BandLimitVector *bandLim = nullptr, Remote antenna = MACRO, bool limitBl = false);
    /*
     * Getter for active connection set
     */
    virtual ActiveSet *readActiveConnections();

    virtual void removeActiveConnections(MacNodeId nodeId);

  protected:

    /**
     * Checks Harq Descriptors and returns the first free codeword.
     *
     * @param id
     * @param cw
     * @return
     */
    virtual bool checkEligibility(MacNodeId id, Codeword& cw, GHz carrierFrequency) = 0;

    /*
     * Schedule and related methods.
     *
     * The methods in the following section must be implemented in the UL and DL class of the
     * LteSchedulerEnb: these are common functions used inside the schedule method, which is
     * the main function of the class
     */

    /**
     * Updates the current schedule list with RAC requests (only for UL).
     * @return TRUE if OFDM space is exhausted.
     */
    virtual bool racschedule(GHz carrierFrequency, BandLimitVector *bandLim = nullptr) = 0;

    /**
     * Updates the current schedule list with HARQ retransmissions.
     * @return TRUE if OFDM space is exhausted.
     */
    virtual bool rtxschedule(GHz carrierFrequency, BandLimitVector *bandLim = nullptr) = 0;

    /**
     * Schedule retransmissions for background UEs
     * @return TRUE if OFDM space is exhausted.
     */
    virtual bool rtxscheduleBackground(GHz carrierFrequency, BandLimitVector *bandLim = nullptr);

    /*
     * OFDMA frame management
     */

    /**
     * Records assigned resource blocks statistics.
     */
    virtual void resourceBlockStatistics(bool sleep = false);

    /**
     * Initializes the blocks-related structures allocation
     */
    virtual void initializeAllocator();

    /**
     * Resets the blocks-related structures allocation
     */
    virtual void resetAllocator();

    /**
     * Returns the available space for a given user, antenna, logical band, and codeword, in bytes.
     *
     * @param id MAC node Id
     * @param antenna antenna
     * @param b band
     * @param cw codeword
     * @return available space in bytes
     */
    virtual unsigned int availableBytes(const MacNodeId id, const Remote antenna, Band b, Codeword cw, Direction dir, GHz carrierFrequency, int limit = -1);

    /// Applies the user's allowed-band restriction to a band-limit vector:
    /// builds a fresh vector in 'storage' when bandLim is null (limit -1 for
    /// allowed bands, -2 otherwise), or masks the provided vector in place
    /// (already-excluded (-2) entries stay excluded). Returns the effective
    /// vector. Shared by the RAC and rtx allocation paths.
    BandLimitVector *applyAllowedBandLimits(MacNodeId nodeId, Direction dir, GHz carrierFrequency, BandLimitVector *bandLim, BandLimitVector& storage);

    /// Fills 'storage' with one BandLimit per band, all codeword limits set
    /// to 'limit'. Shared by the background RAC / rtx paths.
    static void makeUniformBandLimits(BandLimitVector& storage, unsigned int numBands, int limit);

    /// Serves 'bytes' bytes of a retransmission for nodeId on the given
    /// codeword across the (prepared) band-limit vector, records the
    /// allocation (allocator blocks, schedule list entry keyed by bsrLcid,
    /// allocated-codeword count) and returns the number of served bytes.
    /// 'served' reports whether the request fit the available bands (the
    /// allocation is recorded only when it did). Shared by the UL and D2D
    /// per-ACID rtx paths.
    unsigned int allocateRtxBytes(MacNodeId nodeId, Direction dir, LogicalCid bsrLcid, unsigned int bytes, GHz carrierFrequency, Codeword cw, unsigned char acid, BandLimitVector *bandLim, Remote antenna, bool& served);
    /**
     * Returns the available space for a given (background) user, antenna, logical band, and codeword, in bytes.
     *
     * @param id MAC node Id
     * @param antenna antenna
     * @param b band
     * @param cw codeword
     * @return available space in bytes
     */
    virtual unsigned int availableBytesBackgroundUe(const MacNodeId id, const Remote antenna, Band b, Direction dir, GHz carrierFrequency, int limit = -1);

    unsigned int allocatedCws(MacNodeId nodeId)
    {
        return allocatedCws_[nodeId];
    }

    // Get the bands already allocated
    virtual std::set<Band> getOccupiedBands();

    virtual void storeAllocationEnb(std::vector<std::vector<AllocatedRbsPerBandMapA>> allocatedRbsPerBand, std::set<Band> *untouchableBands = nullptr);

    // store an element in the schedule list
    virtual void storeScListId(GHz carrierFrequency, std::pair<MacCid, Codeword> scList, unsigned int num_blocks);

    /*****************
    * UTILITIES
    *****************/

    /**
     * Returns a particular LteScheduler subclass, implementing the given discipline.
     * Overridable seam: D2D-only disciplines (e.g. ALLOCATOR_BESTFIT, used by the
     * frequency-reuse configs) are handled by the D2D uplink scheduler subclasses.
     * @param discipline scheduler discipline
     */
    virtual LteScheduler *getScheduler(SchedDiscipline discipline);
};

} //namespace

#endif // _LTE_LTESCHEDULERENB_H_

