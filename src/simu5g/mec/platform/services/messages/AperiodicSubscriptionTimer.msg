//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

cplusplus {{
    #include <set>
}}

namespace simu5g;

message AperiodicSubscriptionTimer
{
	double period;
}

cplusplus(AperiodicSubscriptionTimer) {{
  private:
    std::set<int> subIdSet_;

  public:
    void insertSubId(int subId) { subIdSet_.insert(subId); }
    void removeSubId(int subId) { subIdSet_.erase(subId); }
    const std::set<int> getSubIdSet() const { return subIdSet_; }
    const int getSubIdSetSize() const { return subIdSet_.size(); }
}}

cplusplus(AperiodicSubscriptionTimer::copy) {{
    this->subIdSet_ = other.subIdSet_;
}}

cplusplus(AperiodicSubscriptionTimer::parsimPack) {{
    doParsimPacking(b, this->subIdSet_);
}}

cplusplus(AperiodicSubscriptionTimer::parsimUnpack) {{
    doParsimUnpacking(b, this->subIdSet_);
}}
