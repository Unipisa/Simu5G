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

#ifndef NODES_MEC_MECORCHESTRATOR_MECHOSTSELECTIONPOLICIES_MECSERVICESELECTIONBASED_H_
#define NODES_MEC_MECORCHESTRATOR_MECHOSTSELECTIONPOLICIES_MECSERVICESELECTIONBASED_H_

#include "simu5g/mec/orchestrator/policies/SelectionPolicyBase.h"

namespace simu5g {

//class MecOrchestrator;

class MecServiceBasedSelectionPolicy : public SelectionPolicyBase
{
  protected:
    cModule *findBestMecHost(const ApplicationDescriptor&) override;

  public:
    MecServiceBasedSelectionPolicy(MecOrchestrator *mecOrchestrator) : SelectionPolicyBase(mecOrchestrator) {}
};

} //namespace

#endif /* NODES_MEC_MECORCHESTRATOR_MECHOSTSELECTIONPOLICIES_MECSERVICESELECTIONBASED_H_ */

