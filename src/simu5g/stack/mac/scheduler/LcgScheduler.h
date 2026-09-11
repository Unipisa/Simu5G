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

#ifndef _LTE_LCGSCHEDULER_H_
#define _LTE_LCGSCHEDULER_H_

#include "simu5g/common/LteCommon.h"
#include "simu5g/stack/mac/LteMacUe.h"

namespace simu5g {

using namespace omnetpp;

/// forward declarations
class LteSchedulerUeUl;
class LteMacPdu;
/**
 */
typedef std::map<MacCid, unsigned int> ScheduleList;

/**
 * UE-side logical channel prioritization (LCP): divides one TTI's UL grant
 * among the UE's outgoing connections, at the granularity Simu5G models --
 * the logical channel group stands in for the per-channel priority of
 * TS 36.321/38.321 sec 5.4.3.1. Groups are served in increasing LCG-index
 * order (strict priority; the PBR token bucket, the full LCP's protection
 * of lower priorities, is not modeled). Within a group the backlogged
 * connections are served round-robin, one SDU (LTE FI) or one PDU carve
 * (NR SO) per turn, so equal-priority connections are served equally and a
 * connection's share does not depend on where bearer-establishment order
 * placed it in the registration map.
 */
class LcgScheduler
{

  protected:
    /**
     * Score-based schedulers descriptor.
     */
    template<typename T, typename S>
    struct SortedDesc
    {
        /// Connection identifier.
        T x_;
        /// Score value.
        S score_;

        /// Comparison operator to enable sorting.
        bool operator<(const SortedDesc& y) const
        {
            return score_ < y.score_;
        }

      public:
        SortedDesc(const T x, const S score) : x_(x), score_(score)
        {
        }
    };

    struct StatusElem
    {
        unsigned int occupancy_;
        unsigned int sentData_;
        unsigned int sentSdus_;
    };

    /// MAC module, used to get parameters from NED
    opp_component_ptr<LteMacUe> mac_;

    /// Associated LteSchedulerUeUl (it is the one who creates the LteScheduler)
    LteSchedulerUeUl *ueScheduler_ = nullptr;

    // schedule List - returned by reference on scheduler invocation
    ScheduleList scheduleList_;

    // scheduled bytes list
    ScheduleList scheduledBytesList_;

    // For NR-SO (no RLC concatenation) connections: the per-PDU payload sizes that
    // fill the grant, so the MAC issues one SDU request per planned PDU and the MAC
    // PDU multiplexes them. Empty/absent for LTE-FI connections (one concatenated PDU).
    std::map<MacCid, std::vector<unsigned int>> scheduledSoPduSizes_;

    /// Cid List
    typedef std::list<MacCid> CidList;

    // scheduling status map
    std::map<MacCid, StatusElem> statusMap_;

  public:

    /**
     * Default constructor.
     */
    LcgScheduler(LteMacUe *mac);
    LcgScheduler(const LcgScheduler& other) { operator=(other); }
    LcgScheduler& operator=(const LcgScheduler& other);
    /**
     * Destructor.
     */
    virtual ~LcgScheduler() {}

    /**
     * Initializes the LteScheduler.
     * @param ueScheduler UE scheduler
     */
    inline virtual void setUeUlScheduler(LteSchedulerUeUl *ueScheduler)
    {
        ueScheduler_ = ueScheduler;
    }

    /* Executes the LCG scheduling algorithm
     * @param availableBytes
     * @return # of scheduled sdus per cid
     */
    virtual ScheduleList& schedule(unsigned int availableBytes, Direction grantDir = UL);

    /* After the scheduling, returns the amount of bytes
     * scheduled for each connection
     */
    virtual ScheduleList& getScheduledBytesList();

    /* After scheduling, returns the planned per-PDU payload sizes for an NR-SO
     * connection (one SDU request per entry), or nullptr for LTE-FI connections.
     */
    virtual const std::vector<unsigned int> *getScheduledSoPduSizes(MacCid cid) const;

    protected:

    /**
     * Hook, called once per LCG during schedule(): returns true if the
     * current UL grant must be withheld so that it can carry the BSR of an active
     * D2D connection instead. Default implementation never withholds (no-op);
     * overridden by LcgSchedulerD2D.
     */
    virtual bool checkForPendingAdditionalBsr(Direction grantDir, Lcg lcg) { return false; }
};

} //namespace simu5g

#endif

