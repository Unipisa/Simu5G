//
//                  Simu5G
//
// Authors: Mohamed Seliem (University College Cork)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//


#ifndef STACK_MAC_SCHEDULING_MODULES_QOSAWARESCHEDULER_H_
#define STACK_MAC_SCHEDULING_MODULES_QOSAWARESCHEDULER_H_

#include "simu5g/stack/mac/scheduler/LteScheduler.h"
#include "simu5g/stack/sdap/common/QfiContextManager.h"
#include <map>
#include <queue>

namespace simu5g {

class QoSAwareScheduler : public LteScheduler
{
  protected:
    typedef std::map<MacCid, double> PfRate;
    typedef std::pair<MacCid, double> ScoredCid;

    PfRate pfRate_;
    std::map<MacCid, unsigned int> grantedBytes_;
    double pfAlpha_;
    const double scoreEpsilon_ = 1e-6;

    QfiContextManager* qfiContextMgr_ = nullptr;
    bool contextLoaded_ = false;

    // Helpers
    void loadContextIfNeeded();
    double computeQosWeightFromContext(const QfiContext& ctx);
    const QfiContext* getQfiContextForCid(MacCid cid);

  public:
    double& pfAlpha() { return pfAlpha_; }

    QoSAwareScheduler(Binder* binder, double pfAlpha);
    void prepareSchedule() override;
    void commitSchedule() override;
};

} // namespace simu5g

#endif /* STACK_MAC_SCHEDULING_MODULES_QOSAWARESCHEDULER_H_ */

