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

#ifndef _LTE_LTEPF_H_
#define _LTE_LTEPF_H_

#include "stack/mac/scheduler/LteScheduler.h"

namespace simu5g {

class LtePf : public LteScheduler
{
  protected:

    typedef std::map<MacCid, double> PfRate;
    typedef SortedDesc<MacCid, double> ScoreDesc;
    typedef std::priority_queue<ScoreDesc> ScoreList;

    //! Long-term rates, used by PF scheduling.
    PfRate pfRate_;

    //! Granted bytes
    std::map<MacCid, unsigned int> grantedBytes_;

    //! Smoothing factor for proportional fair scheduler.
    double pfAlpha_;

    //! Small number to slightly blur scores.
    const double scoreEpsilon_ = 0.000001;

  public:

    double& pfAlpha()
    {
        return pfAlpha_;
    }

    // Scheduling functions ********************************************************************

    //virtual void schedule ();

    void prepareSchedule() override;

    void commitSchedule() override;

    // *****************************************************************************************

    LtePf(Binder *binder, double pfAlpha) :
        LteScheduler(binder),
        pfAlpha_(pfAlpha)
    {
    }

};

} //namespace

#endif // _LTE_LTEPF_H_

