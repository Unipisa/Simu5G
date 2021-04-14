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

#ifndef _NRSCHEDULER_GNB_UL_H_
#define _NRSCHEDULER_GNB_UL_H_

#include "stack/mac/scheduler/LteSchedulerEnbUl.h"

/**
 * @class NRSchedulerGnbUl
 *
 * NR gNB uplink scheduler.
 */
class NRSchedulerGnbUl : public LteSchedulerEnbUl
{
  protected:


  public:

    // does nothing with asynchronous H-ARQ
    virtual void updateHarqDescs() { }

    virtual bool checkEligibility(MacNodeId id, Codeword& cw, double carrierFrequency);

    /**
     * Updates current schedule list with HARQ retransmissions.
     * @return TRUE if OFDM space is exhausted.
     */
    virtual bool rtxschedule(double carrierFrequency, BandLimitVector* bandLim = NULL);
};

#endif // _NRSCHEDULER_GNB_UL_H_
