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

/**
 * Extends the LTE/NR MAC layer scheduler to support QoS-aware scheduling
 * decisions based on QFI (QoS Flow Identifier) contexts. QfiContextManager
 * maintains mappings between QFIs, CIDs, and their associated QoS parameters.
 *
 * Key Features:
 * - Implements a PF-based scheduler that dynamically scores active CIDs using
 *   QoS weights derived from QFI context (e.g., 5QI, GBR, delay budget, PER,
 *   priority).
 * - Supports per-CID registration of QFIs
 * - QfiContextManager can load QFI-DRB configurations from file
 * - Provides flexible QoS weight computation based on service criticality.
 * - Enables more realistic traffic differentiation for scenarios involving
 *   conversational voice, URLLC, video streaming, etc.
 *
 * Notes:
 * - Scheduler gracefully defaults to best-effort when no QFI context is found.
 */
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

