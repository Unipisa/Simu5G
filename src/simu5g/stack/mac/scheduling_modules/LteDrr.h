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

#ifndef _LTE_LTEDRR_H_
#define _LTE_LTEDRR_H_

#include <map>
#include "stack/mac/scheduler/LteScheduler.h"
#include "common/Circular.h"

namespace simu5g {

class LteSchedulerEnb;

class LteDrr : public LteScheduler
{
  private:

    //! DRR descriptor.
    struct DrrDesc
    {
        //! DRR quantum, in bytes.
        unsigned int quantum_ = 0;
        //! Deficit, in bytes.
        unsigned int deficit_ = 0;
        //! Flag indicating whether the connection consumed all the previous quantum and needs another one.
        bool addQuantum_ = true;
        //! True if this descriptor is in the active list.
        bool active_ = false;
        //! True if this connection is eligible for service.
        bool eligible_ = false;

        //! Create an inactive DRR descriptor.
        DrrDesc()
        {
        }
    };

    typedef std::map<MacCid, DrrDesc> DrrDescMap;
    typedef CircularList<MacCid> ActiveList;

    //! Deficit round-robin Active List.
    ActiveList activeList_;

    //! Deficit round-robin Active List. Temporary variable used in the two-phase scheduling operations.
    ActiveList activeTempList_;

    //! Deficit round-robin descriptor per-connection map.
    DrrDescMap drrMap_;

    //! Deficit round-robin descriptor per-connection map. Temporary variable used in the two-phase scheduling operations.
    DrrDescMap drrTempMap_;

  public:
    LteDrr(Binder *binder) : LteScheduler(binder) {}

    // Scheduling functions ********************************************************************

    //virtual void schedule ();

    void prepareSchedule() override;

    void commitSchedule() override;

    // *****************************************************************************************

    void notifyActiveConnection(MacCid cid) override;

    void updateSchedulingInfo() override;
};

} //namespace

#endif

