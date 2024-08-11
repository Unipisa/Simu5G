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

#include "nodes/mec/MECOrchestrator/mecHostSelectionPolicies/AvailableResourcesSelectionBased.h"
#include "nodes/mec/MECPlatformManager/MecPlatformManager.h"
#include "nodes/mec/VirtualisationInfrastructureManager/VirtualisationInfrastructureManager.h"

namespace simu5g {

cModule *AvailableResourcesSelectionBased::findBestMecHost(const ApplicationDescriptor& appDesc)
{
    EV << "AvailableResourcesSelectionBased::findBestMecHost - finding best MecHost..." << endl;
    cModule *bestHost = nullptr;
    double maxCpuSpeed = -1;

    for (auto mecHost : mecOrchestrator_->mecHosts) {
        VirtualisationInfrastructureManager *vim = check_and_cast<VirtualisationInfrastructureManager *>(mecHost->getSubmodule("vim"));
        ResourceDescriptor resources = appDesc.getVirtualResources();
        bool res = vim->isAllocable(resources.ram, resources.disk, resources.cpu);
        if (!res) {
            EV << "AvailableResourcesSelectionBased::findBestMecHost - MEC host [" << mecHost->getName() << "] does not have enough resources. Searching again..." << endl;
            continue;
        }
        if (vim->getAvailableResources().cpu > maxCpuSpeed) {
            // Temporarily select this mec host as the best
            EV << "AvailableResourcesSelectionBased::findBestMecHost - MEC host [" << mecHost->getName() << "] temporarily chosen as best MEC host. Available resources: " << endl;
            vim->printResources();
            bestHost = mecHost;
            maxCpuSpeed = vim->getAvailableResources().cpu;
        }
    }
    if (bestHost != nullptr)
        EV << "AvailableResourcesSelectionBased::findBestMecHost - MEC host [" << bestHost->getName() << "] has been chosen as the best Mec Host" << endl;
    else
        EV << "AvailableResourcesSelectionBased::findBestMecHost - No Mec Host found" << endl;
    return bestHost;
}

} //namespace

