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
#include <vector>

namespace simu5g {

class LteMacBase;

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

    // scratch for the aggregated profile getQosForUlGroup() returns a pointer to
    DrbQosProfile groupQos_;

    // Helpers
    virtual double computeQosWeight(const DrbQosProfile& e);
    virtual const DrbQosProfile *getDrbQosForCid(MacCid cid);
    virtual const DrbQosProfile *getQosForUlGroup(MacNodeId ueId, Lcg lcg);

  public:
    /**
     * The QoS profiles of the DRBs behind one uplink logical channel group: the
     * join of the MAC's configured-DRB profiles with its established channels'
     * group assignment. A configured-but-unestablished DRB has no channel config
     * yet and is skipped, so the result is empty when no established DRB of the UE
     * belongs to the group.
     */
    static std::vector<const DrbQosProfile *> collectUlGroupMembers(
            const std::map<DrbKey, DrbQosProfile>& drbQosMap, LteMacBase *mac, MacNodeId ueId, Lcg lcg);

    /**
     * The weight-relevant profile of a group of bearers: the most demanding
     * member on each axis -- the lowest (= most important) priority level, GBR
     * if any member is GBR, the tightest delay budget and error target. Used to
     * weigh an uplink (UE, LCG) pseudo-connection by the DRBs behind it.
     */
    static DrbQosProfile aggregateQosProfiles(const std::vector<const DrbQosProfile *>& members);

    double& pfAlpha() { return pfAlpha_; }

    QoSAwareScheduler(Binder* binder, double pfAlpha);
    void setDrbQosMap(const std::map<DrbKey, DrbQosProfile> *m) { drbQosMap_ = m; }
    void prepareSchedule() override;
    void commitSchedule() override;
};

} // namespace simu5g

#endif /* STACK_MAC_SCHEDULING_MODULES_QOSAWARESCHEDULER_H_ */

