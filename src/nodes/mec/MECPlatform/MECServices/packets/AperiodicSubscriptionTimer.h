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

#ifndef NODES_MEC_MEPLATFORM_MESERVICES_PACKETS_APERIODICSUBSCRIPTIONTIMER_H_
#define NODES_MEC_MEPLATFORM_MESERVICES_PACKETS_APERIODICSUBSCRIPTIONTIMER_H_

#include "AperiodicSubscriptionTimer_m.h"
#include <set>

class AperiodicSubscriptionTimer: public AperiodicSubscriptionTimer_m {
public:
    AperiodicSubscriptionTimer();
    AperiodicSubscriptionTimer(const char *name=nullptr, const double& period = 0);
    AperiodicSubscriptionTimer(const char *name=nullptr);
    virtual ~AperiodicSubscriptionTimer();

    void insertSubId(int subId)
    {
        subIdSet_.insert(subId);
    }
    void removeSubId(int subId)
    {
        subIdSet_.erase(subId);
    }
    const std::set<int> getSubIdSet() const
    {
        return subIdSet_;
    }
    const int getSubIdSetSize() const
    {
        return subIdSet_.size();
    }

private:
    std::set<int> subIdSet_;
};

#endif /* NODES_MEC_MEPLATFORM_MESERVICES_PACKETS_APERIODICSUBSCRIPTIONTIMER_H_ */
