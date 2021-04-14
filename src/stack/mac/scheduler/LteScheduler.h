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

#ifndef _LTE_LTESCHEDULER_H_
#define _LTE_LTESCHEDULER_H_

#include "common/LteCommon.h"
#include "stack/mac/layer/LteMacEnb.h"

/// forward declarations
class LteSchedulerEnb;

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
        if (score_ < y.score_)
            return true;
        if (score_ == y.score_)
            return uniform(omnetpp::getEnvir()->getRNG(0),0,1) < 0.5;
        return false;
    }

  public:
    SortedDesc()
    {

    }
    SortedDesc(const T x, const S score)
    {
        x_ = x;
        score_ = score;
    }
};

/**
 * @class LteScheduler
 */
class LteScheduler
{
  protected:

    /// MAC module, used to get parameters from NED
    LteMacEnb *mac_;

    /// Reference to the LTE binder
    Binder *binder_;

    /// Associated LteSchedulerEnb (it is the one who creates the LteScheduler)
    LteSchedulerEnb* eNbScheduler_;

    /// Link Direction (DL/UL)
    Direction direction_;

    //! Set of active connections.
    ActiveSet* activeConnectionSet_;

    //! General Active set. Temporary variable used in the two phase scheduling operations
    ActiveSet activeConnectionTempSet_;

    //! Per-carrier Active set. Temporary variable used for storing the set of connections allowed in this carrier
    ActiveSet carrierActiveConnectionSet_;

    //! Frequency of the carrier handled by this scheduler
    double carrierFrequency_;

    //! Set of bands available for this carrier
    BandLimitVector* bandLimit_;

    /// Cid List
    typedef std::list<MacCid> CidList;

    /**
     * Grant type per class of service.
     * FIXED has size, URGENT has max size, FITALL always has 4294967295U size.
     */
    std::map<LteTrafficClass, GrantType> grantTypeMap_;

    /**
     * Grant size per class of service.
     * FIXED has size, URGENT has max size, FITALL always has 4294967295U size.
     */
    std::map<LteTrafficClass, int> grantSizeMap_;

    //! numerology index of the component carrier it has to schedule
    unsigned int numerologyIndex_;

    //! timers for handling scheduling period of this scheduler
    unsigned int maxSchedulingPeriodCounter_;
    unsigned int currentSchedulingPeriodCounter_;

  public:

    /**
     * Default constructor.
     */
    LteScheduler()
    {
        //    WATCH(activeSet_);
        activeConnectionSet_ = nullptr;
        binder_ = nullptr;
    }
    /**
     * Destructor.
     */
    virtual ~LteScheduler()
    {
    }
    /**
     * Initializes the LteScheduler.
     * @param eNbScheduler eNb scheduler
     */
    virtual void setEnbScheduler(LteSchedulerEnb* eNbScheduler);

    /**
     * Initializes the carrier frequency for this LteScheduler.
     * @param carrierFrequency carrier frequency
     */
    void setCarrierFrequency(double carrierFrequency);

    /*
     * Set the period counter
     */
    void initializeSchedulerPeriodCounter(NumerologyIndex maxNumerologyIndex);

    /*
     * Handle the period counter
     */
    unsigned int decreaseSchedulerPeriodCounter();

    /**
     * Returns the carrier frequency for this LteScheduler.
     */
    double getCarrierFrequency() { return carrierFrequency_; };

    /**
     * Set the numerology index for this scheduler
     */
    void setNumerologyIndex(unsigned int numerologyIndex) { numerologyIndex_ = numerologyIndex; }

    // Scheduling functions ********************************************************************

    /**
     * The schedule function is splitted in two phases
     *  - in the first phase, carried out by the prepareSchedule(),
     *    the computation of the algorithm on temporary structures is performed
     *  - in the second phase, carried out by the storeSchedule(),
     *    the commitment to the permanent variables is performed
     *
     * In this way, if in the environment there's a special module which wants to execute
     * more schedulers, compare them and pick a single one, the execSchedule() of each
     * scheduler is executed, but only the storeSchedule() of the picked one will be executed.
     * The schedule() simply call the sequence of execSchedule() and storeSchedule().
     */

    virtual void schedule();

    virtual void prepareSchedule()
    {
    }
    virtual void commitSchedule()
    {
    }

    // *****************************************************************************************

    /// performs request of grant to the eNbScheduler
    virtual unsigned int requestGrant(MacCid cid, unsigned int bytes, bool& terminate, bool& active, bool& eligible , std::vector<BandLimit>* bandLim = nullptr);

    /// calls eNbScheduler rtxschedule()
    virtual bool scheduleRetransmissions();

    /// calls LteSchedulerEnbUl serveRacs()
    virtual void scheduleRacRequests();

    /// calls LteSchedulerEnbUl racGrantEnb()
    virtual void requestRacGrant(MacNodeId nodeId);

    virtual void notifyActiveConnection(MacCid activeCid)
    {
    }
    virtual void updateSchedulingInfo()
    {
    }

  protected:

    /*
     * prepare the set of active connections on this carrier
     * used by scheduling modules
     */
    void buildCarrierActiveConnectionSet();

  private:

    /**
     * Utility function.
     * Initializes grantType_ and grantSize_ maps using mac NED parameters.
     * Note: mac_ amd direction_ should be initialized.
     */
    void initializeGrants();

};

#endif // _LTE_LTESCHEDULER_H_
