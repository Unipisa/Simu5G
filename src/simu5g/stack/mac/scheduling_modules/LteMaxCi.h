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

#ifndef _LTE_LTEMAXCI_H_
#define _LTE_LTEMAXCI_H_

#include "stack/mac/scheduler/LteScheduler.h"

namespace simu5g {

class LteMaxCi : public virtual LteScheduler
{
  protected:

    typedef SortedDesc<MacCid, unsigned int> ScoreDesc;
    typedef std::priority_queue<ScoreDesc> ScoreList;

  public:
    LteMaxCi(Binder *binder) : LteScheduler(binder) {}

    void prepareSchedule() override;

    void commitSchedule() override;

};

} //namespace

#endif // _LTE_LTEMAXCI_H_

