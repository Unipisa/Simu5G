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

#include "nodes/mec/MECOrchestrator/mecHostSelectionPolicies/MecServiceSelectionBased.h"
#include "nodes/mec/MECPlatformManager/MecPlatformManager.h"
#include "nodes/mec/VirtualisationInfrastructureManager/VirtualisationInfrastructureManager.h"

namespace simu5g {

cModule *MecServiceSelectionBased::findBestMecHost(const ApplicationDescriptor& appDesc)
{
    EV << "MecServiceSelectionBased::findBestMecHost - finding best MecHost..." << endl;
    cModule *bestHost = nullptr;
    bool found = false;

    for (auto mecHost : mecOrchestrator_->mecHosts) {
        EV << "MecServiceSelectionBased::findBestMecHost - MEC host [" << mecHost->getName() << "] size of mecHost " << mecOrchestrator_->mecHosts.size() << endl;
        VirtualisationInfrastructureManager *vim = check_and_cast<VirtualisationInfrastructureManager *>(mecHost->getSubmodule("vim"));
        ResourceDescriptor resources = appDesc.getVirtualResources();
        bool res = vim->isAllocable(resources.ram, resources.disk, resources.cpu);
        if (!res) {
            EV << "MecServiceSelectionBased::findBestMecHost - MEC host [" << mecHost->getName() << "] does not have enough resources. Searching again..." << endl;
            continue;
        }

        // Temporarily select this mec host as the best
        EV << "MecServiceSelectionBased::findBestMecHost - MEC host [" << mecHost->getName() << "] temporarily chosen as the best MEC host, checking for the required MEC services.." << endl;
        bestHost = mecHost;

        MecPlatformManager *mecpm = check_and_cast<MecPlatformManager *>(mecHost->getSubmodule("mecPlatformManager"));
        auto mecServices = mecpm->getAvailableMecServices();
        std::string serviceName;

        // I assume the app requires only one mec service
        if (appDesc.getAppServicesRequired().size() > 0) {
            serviceName = appDesc.getAppServicesRequired()[0];
            EV << "MecServiceSelectionBased::findBestMecHost - required Mec Service: " << serviceName << endl;
        }
        else {
            EV << "MecServiceSelectionBased::findBestMecHost - the Mec App does not require any MEC service. Choosing the temporary Mec Host as the best one" << endl;
            found = true;
            break;
        }
        for (const auto& service : *mecServices) {
            if (serviceName == service.getName() && service.getMecHost() == bestHost->getName()) {
                EV << "MecServiceSelectionBased::findBestMecHost - The temporary Mec Host has the MEC service " << service.getName() << " required by the Mec App. It has been chosen as the best one" << endl;
                bestHost = mecHost;
                found = true;
                break;
            }
        }
        if (found)
            break;
    }

    if (bestHost != nullptr && !found)
        EV << "MecServiceSelectionBased::findBestMecHost - The best Mec Host does not have the required service. Best MEC host: " << bestHost << endl;
    else if (bestHost == nullptr)
        EV << "MecServiceSelectionBased::findBestMecHost - no MEC host found" << endl;

    return bestHost;
}

} //namespace

