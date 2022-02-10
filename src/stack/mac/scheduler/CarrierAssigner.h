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

#ifndef _CARRIERASSIGNER_H_
#define _CARRIERASSIGNER_H_

#include "common/LteCommon.h"
#include "stack/mac/layer/LteMacEnb.h"
#include "stack/mac/scheduler/LteSchedulerEnb.h"

/**
 * @class CarrierAssigner
 */
class CarrierAssigner
{
  protected:

    /// Reference to the owning MAC module
    LteMacEnb *mac_;

    /// Reference to the scheduler using this carrier assigner
    LteSchedulerEnb *schedulerEnb_;

    /// Reference to the binder
    Binder *binder_;

    /// Direction
    Direction direction_;

    //! Per-carrier Active set. Variable used for storing the set of connections allowed per carrier
    std::map<double, ActiveSet> carrierActiveConnectionSet_;

  public:

    /**
     * Default constructor.
     */
    CarrierAssigner(LteSchedulerEnb* schedEnb, LteMacEnb* mac, Direction dir)
    {
        //    WATCH(activeSet_);
        schedulerEnb_ = schedEnb;
        mac_ = mac;
        direction_ = dir;
    }
    /**
     * Destructor.
     */
    virtual ~CarrierAssigner()
    {
    }

    // initialize the set of active connections for the given carrier
    void initializeCarrierActiveConnectionSet(double carrierFrequency);

    // return the current set of active connection for the given carrier
    ActiveSet* getCarrierActiveConnectionSet(double carrierFrequency);

    // run the carrier assignment algorithm, to be redefined
    virtual void assign() = 0;
};

#endif // _CARRIERASSIGNER_H_
