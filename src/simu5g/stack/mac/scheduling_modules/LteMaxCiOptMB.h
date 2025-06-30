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

#ifndef LTEMAXCIOPTMB_H_
#define LTEMAXCIOPTMB_H_

#include "stack/mac/scheduler/LteScheduler.h"
#include <string>
#include "stack/mac/amc/AmcPilot.h"

namespace simu5g {

typedef std::map<MacNodeId, std::vector<BandLimit>> SchedulingDecision;
typedef std::map<MacNodeId, UsableBands> UsableBandList;

class LteMaxCiOptMB : public virtual LteScheduler
{

    std::string problemFile_;
    std::string solutionFile_;

    std::vector<MacNodeId> ueList_;
    std::vector<MacCid> cidList_;
    SchedulingDecision schedulingDecision_;

    UsableBandList usableBands_;

    // read the CQIs and queue information for each user and build an optimization problem
    void generateProblem();

    // call the interactive solver
    void launchProblem();

    // parse the solution
    void readSolution();

    // apply the scheduling decision in the allocator (occupies the Resource blocks)
    void applyScheduling();

  public:
    LteMaxCiOptMB(Binder *binder);

    void prepareSchedule() override;

    void commitSchedule() override;

};

} //namespace

#endif /* LTEMAXCIOPTMB_H_ */

