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
#include "simu5g/stack/mac/DrbQosProfile.h"
#include <map>
#include <queue>

namespace simu5g {

/**
 * QoS-aware proportional-fair scheduler: scores active CIDs with QoS weights
 * derived from the per-DRB QoS profile (GBR flag, packet delay budget, packet
 * error rate, priority level). The profiles come from the MAC's DRB QoS map
 * (see LteMacEnb::getDrbQosMap()), which RRC fills from the bearer configuration
 * delivered to it (the staticDrbs parameter of the ~BearerConfigurator module).
 *
 * A CID whose bearer has no QoS profile is scheduled with a neutral weight
 * (plain proportional fair).
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

    const std::map<DrbKey, DrbQosProfile> *drbQosMap_ = nullptr;

    // QoS weight parameters  TODO initialize from NED parameters
    double gbrMultiplier_ = 2.0;
    double priorityBase_ = 10.0;       // weight contribution = priorityBase_ / priorityLevel (scale-free: only priority ratios matter)
    double delayUrgentMs_ = 10.0;      // delay budget thresholds (ms)
    double delayTightMs_ = 50.0;
    double delayLooseMs_ = 100.0;
    double delayUrgentMultiplier_ = 5.0;  // multipliers for each tier
    double delayTightMultiplier_ = 3.0;
    double delayLooseMultiplier_ = 1.5;

    // Helpers
    virtual double computeQosWeight(const DrbQosProfile& e);
    virtual const DrbQosProfile *getDrbQosForCid(MacCid cid);

  public:
    double& pfAlpha() { return pfAlpha_; }

    QoSAwareScheduler(Binder* binder, double pfAlpha);
    void setDrbQosMap(const std::map<DrbKey, DrbQosProfile> *m) { drbQosMap_ = m; }
    void prepareSchedule() override;
    void commitSchedule() override;
};

} // namespace simu5g

#endif /* STACK_MAC_SCHEDULING_MODULES_QOSAWARESCHEDULER_H_ */

