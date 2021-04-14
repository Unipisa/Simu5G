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

class LteMacUe;
class LcgScheduler;

/**
 * LTE Ue uplink scheduler.
 */
class LteSchedulerUeUl
{
  protected:

    // MAC module, queried for parameters
    LteMacUe *mac_;

    // Schedule List
    LteMacScheduleList scheduleList_;

    // Scheduled Bytes List
    LteMacScheduleList scheduledBytesList_;

    // Inner Scheduler - default to Standard LCG
    LcgScheduler* lcgScheduler_;

    // carrier frequency handled by this scheduler
    double carrierFrequency_;

  public:

    /* Performs the standard LCG scheduling algorithm
     * @returns reference to scheduling list
     */

    LteMacScheduleList* schedule();

    /* After the scheduling, returns the amount of bytes
     * scheduled for each connection
     */
    LteMacScheduleList* getScheduledBytesList();

    /*
     * constructor
     */
    LteSchedulerUeUl(LteMacUe * mac, double carrierFrequency);

    /**
     * Copy constructor and operator=
     */
    LteSchedulerUeUl(const LteSchedulerUeUl& other)
    {
        operator=(other);
    }
    LteSchedulerUeUl& operator=(const LteSchedulerUeUl& other);

    /*
     * destructor
     */
    ~LteSchedulerUeUl();
};

#endif // _LTE_LTE_SCHEDULER_UE_UL_H_
