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

#ifndef _LTE_LTE_SCHEDULER_UE_UL_H_
#define _LTE_LTE_SCHEDULER_UE_UL_H_

#include "common/LteCommon.h"

#include "stack/mac/LteMacUe.h"
#include "stack/mac/scheduler/LcgScheduler.h"

namespace simu5g {

using namespace omnetpp;

class LteMacUe;
class LcgScheduler;

/**
 * LTE Ue uplink scheduler.
 */
class LteSchedulerUeUl
{
  protected:

    // MAC module, queried for parameters
    opp_component_ptr<LteMacUe> mac_;

    // Schedule List
    LteMacScheduleList scheduleList_;

    // Scheduled Bytes List
    LteMacScheduleList scheduledBytesList_;

    // Inner Scheduler - defaults to Standard LCG
    LcgScheduler lcgScheduler_;

    // Carrier frequency handled by this scheduler
    double carrierFrequency_;

  public:

    /* Performs the standard LCG scheduling algorithm
     * @returns reference to scheduling list
     */
    LteMacScheduleList *schedule();

    /* After the scheduling, returns the amount of bytes
     * scheduled for each connection
     */
    LteMacScheduleList *getScheduledBytesList();

    /*
     * Constructor
     */
    LteSchedulerUeUl(LteMacUe *mac, double carrierFrequency);

    /**
     * Copy constructor and operator=
     */
    LteSchedulerUeUl(const LteSchedulerUeUl& other) :
        mac_(other.mac_),
        scheduleList_(other.scheduleList_),
        scheduledBytesList_(other.scheduledBytesList_),
        lcgScheduler_(other.lcgScheduler_),
        carrierFrequency_(other.carrierFrequency_)
    {
    }

    LteSchedulerUeUl& operator=(const LteSchedulerUeUl& other);

    /*
     * Destructor
     */
    ~LteSchedulerUeUl();
};

} //namespace

#endif // _LTE_LTE_SCHEDULER_UE_UL_H_

