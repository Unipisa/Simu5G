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

#include "MecHostBasedSelectionPolicy.h"
#include "simu5g/mec/mepm/MecPlatformManager.h"
#include "simu5g/mec/vim/VirtualisationInfrastructureManager.h"

namespace simu5g {

MecHostBasedSelectionPolicy::MecHostBasedSelectionPolicy(MecOrchestrator *mecOrchestrator, int index):SelectionPolicyBase(mecOrchestrator), mecHostIndex_(index)
{
}

cModule *MecHostBasedSelectionPolicy::findBestMecHost(const ApplicationDescriptor& appDesc)
{
    EV << "MecHostBasedSelectionPolicy::findBestMecHost - finding best MecHost..." << endl;
    cModule *bestHost = nullptr;

    int size = mecOrchestrator_->mecHosts.size();
    if (size <= mecHostIndex_) {
        EV << "MecHostBasedSelectionPolicy::findBestMecHost - No Mec Host with index [" << mecHostIndex_ << "] found" << endl;
    }
    else {
        bestHost = mecOrchestrator_->mecHosts.at(mecHostIndex_);
        EV << "MecHostBasedSelectionPolicy::findBestMecHost - MEC host [" << bestHost->getName() << "] has been chosen as the best Mec Host" << endl;
    }
    return bestHost;
}

} //namespace

