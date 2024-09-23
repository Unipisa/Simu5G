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

#ifndef NODES_MEC_MECORCHESTRATOR_SELECTIONPOLICYBASE_H_
#define NODES_MEC_MECORCHESTRATOR_SELECTIONPOLICYBASE_H_

#include "nodes/mec/MECOrchestrator/MecOrchestrator.h"
#include "nodes/mec/MECOrchestrator/ApplicationDescriptor/ApplicationDescriptor.h"

namespace simu5g {

class MecOrchestrator;

class SelectionPolicyBase
{
    friend class MecOrchestrator;

  protected:
    MecOrchestrator *mecOrchestrator_ = nullptr;
    virtual cModule *findBestMecHost(const ApplicationDescriptor&) = 0;

  public:
    SelectionPolicyBase(MecOrchestrator *mecOrchestrator) : mecOrchestrator_(mecOrchestrator) {}
};

} //namespace

#endif /* NODES_MEC_MECORCHESTRATOR_SELECTIONPOLICYBASE_H_ */

