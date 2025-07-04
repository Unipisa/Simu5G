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

#ifndef NODES_MEC_MECORCHESTRATOR_MECHOSTSELECTIONPOLICIES_AVAILABLERESOURCESELECTION_H_
#define NODES_MEC_MECORCHESTRATOR_MECHOSTSELECTIONPOLICIES_AVAILABLERESOURCESELECTION_H_

#include "simu5g/mec/orchestrator/policies/SelectionPolicyBase.h"

namespace simu5g {

//class MecOrchestrator;

class AvailableResourcesBasedSelectionPolicy : public SelectionPolicyBase
{
  protected:
    cModule *findBestMecHost(const ApplicationDescriptor&) override;

  public:
    AvailableResourcesBasedSelectionPolicy(MecOrchestrator *mecOrchestrator) : SelectionPolicyBase(mecOrchestrator) {}
};

} //namespace

#endif /* NODES_MEC_MECORCHESTRATOR_MECHOSTSELECTIONPOLICIES_AVAILABLERESOURCESELECTION_H_ */

