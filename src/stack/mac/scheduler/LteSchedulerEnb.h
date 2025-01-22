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

#ifndef _LTE_LTESCHEDULERENB_H_
#define _LTE_LTESCHEDULERENB_H_

#include "common/LteCommon.h"
#include "stack/mac/buffer/harq/LteHarqBufferTx.h"
#include "stack/mac/allocator/LteAllocatorUtils.h"
#include "stack/mac/LteMacEnb.h"

namespace simu5g {

using namespace omnetpp;

/// forward declarations
class LteScheduler;
class LteAllocationModule;
class LteMacEnb;

/**
 * @class LteSchedulerEnb
 *
 */
class LteSchedulerEnb
{
    /******************
    * Friend classes
    ******************/

    // Lte Scheduler Modules access grants
    friend class LteScheduler;
    friend class LteDrr;
    friend class LtePf;
    friend class LteMaxCi;
    friend class LteMaxCiMultiband;
    friend class LteMaxCiOptMB;
    friend class LteMaxCiComp;
    friend class LteAllocatorBestFit;

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
    std::map<double, LteMacScheduleList> scheduleList_;

    // Codeword list
    LteMacAllocatedCws allocatedCws_;

    // Pointer to downlink virtual buffers (that are in LteMacBase)
    LteMacBufferMap *vbuf_ = nullptr;

    // Pointer to uplink virtual buffers (that are in LteMacBase)
    LteMacBufferMap *bsrbuf_ = nullptr;

    // Pointer to Harq Tx Buffers (that are in LteMacBase)
    std::map<double, HarqTxBuffers> *harqTxBuffers_ = nullptr;

    // Pointer to Harq Rx Buffers (that are in LteMacBase)
    std::map<double, HarqRxBuffers> *harqRxBuffers_ = nullptr;

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
    LteSchedulerEnb();

    /**
     * Copy constructor and operator=
     */
    LteSchedulerEnb(const LteSchedulerEnb& other)
    {
        operator=(other);
    }

    LteSchedulerEnb& operator=(const LteSchedulerEnb& other);

    /**
     * Destructor.
     */
    virtual ~LteSchedulerEnb();

    /**
     * Set Direction and bind the internal pointers to the MAC objects.
     * @param dir link direction
     * @param mac pointer to MAC module
     * @param binder pointer to Binder module
     */
    void initialize(Direction dir, LteMacEnb *mac, Binder *binder);

    /*
     * Initialize counters for schedulers
     */
    void initializeSchedulerPeriodCounter(NumerologyIndex maxNumerologyIndex);

    /**
     * Schedule data. Returns one schedule list per carrier
     * @param list
     */
    virtual std::map<double, LteMacScheduleList> *schedule();

    /**
     * Adds an entry (if not already in) to scheduling list.
     * The function calls the LteScheduler notify().
     * @param cid connection identifier
     */
    void backlog(MacCid cid);

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
    unsigned int readPerUeAllocatedBlocks(const MacNodeId nodeId, const Remote antenna, const Band b);

    /*
     * Returns the amount of blocks allocated on a logical band
     */
    unsigned int readPerBandAllocatedBlocks(Plane plane, const Remote antenna, const Band band);

    /*
     * Returns the amount of blocks allocated on a logical band
     */
    unsigned int getInterferingBlocks(Plane plane, const Remote antenna, const Band band);

    /**
     * Returns the number of available blocks for the UE on the given antenna
     * in the logical band provided.
     */
    unsigned int readAvailableRbs(const MacNodeId id, const Remote antenna, const Band b);

    /**
     * Returns the number of available blocks.
     */
    unsigned int readTotalAvailableRbs();

    /**
     * Resource Block IDs computation function.
     */
    unsigned int readRbOccupation(const MacNodeId id, double carrierFrequency, RbMap& rbMap);

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
    virtual unsigned int schedulePerAcidRtx(MacNodeId nodeId, double carrierFrequency, Codeword cw, unsigned char acid,
            std::vector<BandLimit> *bandLim = nullptr, Remote antenna = MACRO, bool limitBl = false) = 0;

    virtual unsigned int scheduleBgRtx(MacNodeId bgUeId, double carrierFrequency, Codeword cw, std::vector<BandLimit> *bandLim = nullptr,
            Remote antenna = MACRO, bool limitBl = false) = 0;

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
    virtual unsigned int scheduleGrant(MacCid cid, unsigned int bytes, bool& terminate, bool& active, bool& eligible, double carrierFrequency,
            BandLimitVector *bandLim = nullptr, Remote antenna = MACRO, bool limitBl = false);

    virtual unsigned int scheduleGrantBackground(MacCid bgCid, unsigned int bytes, bool& terminate, bool& active, bool& eligible, double carrierFrequency,
            BandLimitVector *bandLim = nullptr, Remote antenna = MACRO, bool limitBl = false);
    /*
     * Getter for active connection set
     */
    ActiveSet *readActiveConnections();

    void removeActiveConnections(MacNodeId nodeId);

  protected:

    /**
     * Checks Harq Descriptors and returns the first free codeword.
     *
     * @param id
     * @param cw
     * @return
     */
    virtual bool checkEligibility(MacNodeId id, Codeword& cw, double carrierFrequency) = 0;

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
    virtual bool racschedule(double carrierFrequency, BandLimitVector *bandLim = nullptr) = 0;

    /**
     * Updates the current schedule list with HARQ retransmissions.
     * @return TRUE if OFDM space is exhausted.
     */
    virtual bool rtxschedule(double carrierFrequency, BandLimitVector *bandLim = nullptr) = 0;

    /**
     * Schedule retransmissions for background UEs
     * @return TRUE if OFDM space is exhausted.
     */
    virtual bool rtxscheduleBackground(double carrierFrequency, BandLimitVector *bandLim = nullptr) = 0;

    /*
     * OFDMA frame management
     */

    /**
     * Records assigned resource blocks statistics.
     */
    void resourceBlockStatistics(bool sleep = false);

    /**
     * Initializes the blocks-related structures allocation
     */
    void initializeAllocator();

    /**
     * Resets the blocks-related structures allocation
     */
    void resetAllocator();

    /**
     * Returns the available space for a given user, antenna, logical band, and codeword, in bytes.
     *
     * @param id MAC node Id
     * @param antenna antenna
     * @param b band
     * @param cw codeword
     * @return available space in bytes
     */
    unsigned int availableBytes(const MacNodeId id, const Remote antenna, Band b, Codeword cw, Direction dir, double carrierFrequency, int limit = -1);
    /**
     * Returns the available space for a given (background) user, antenna, logical band, and codeword, in bytes.
     *
     * @param id MAC node Id
     * @param antenna antenna
     * @param b band
     * @param cw codeword
     * @return available space in bytes
     */
    unsigned int availableBytesBackgroundUe(const MacNodeId id, const Remote antenna, Band b, Direction dir, double carrierFrequency, int limit = -1);

    unsigned int allocatedCws(MacNodeId nodeId)
    {
        return allocatedCws_[nodeId];
    }

    // Get the bands already allocated
    std::set<Band> getOccupiedBands();

    void storeAllocationEnb(std::vector<std::vector<AllocatedRbsPerBandMapA>> allocatedRbsPerBand, std::set<Band> *untouchableBands = nullptr);

    // store an element in the schedule list
    void storeScListId(double carrierFrequency, std::pair<unsigned int, Codeword> scList, unsigned int num_blocks);

  private:

    /*****************
    * UTILITIES
    *****************/

    /**
     * Returns a particular LteScheduler subclass,
     * implementing the given discipline.
     * @param discipline scheduler discipline
     */
    LteScheduler *getScheduler(SchedDiscipline discipline);
};

} //namespace

#endif // _LTE_LTESCHEDULERENB_H_

