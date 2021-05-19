//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "nodes/mec/MECPlatformManager/MecPlatformManager.h"

Define_Module(MecPlatformManager);

MecPlatformManager::MecPlatformManager()
{
    vim = nullptr;
    serviceRegistry = nullptr;
}

void MecPlatformManager::initialize(int stage)
{
    EV << "VirtualisationInfrastructureManager::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;
    vim = check_and_cast<VirtualisationInfrastructureManager*>(getParentModule()->getSubmodule("vim"));
    cModule* mecPlatform = getParentModule()->getSubmodule("mecPlatform");
    if(mecPlatform->findSubmodule("ServiceRegistry") != -1)
    {
        serviceRegistry = check_and_cast<ServiceRegistry*>(mecPlatform->getSubmodule("serviceRegistry"));
    }
}


// instancing the requested MEApp (called by handleResource)
MecAppInstanceInfo MecPlatformManager::instantiateMEApp(CreateAppMessage* msg)
{
    return vim->instantiateMEApp(msg);
}

bool MecPlatformManager::instantiateEmulatedMEApp(CreateAppMessage* msg)
{
    return vim->instantiateEmulatedMEApp(msg);
}

bool MecPlatformManager::terminateEmulatedMEApp(DeleteAppMessage* msg)
{
    return vim->terminateEmulatedMEApp(msg);
}


// terminating the correspondent MEApp (called by handleResource)
bool MecPlatformManager::terminateMEApp(DeleteAppMessage* msg)
{
    return vim->terminateMEApp(msg);
}

const MecServicesMap* MecPlatformManager::getAvailableServices(){
    if(serviceRegistry == nullptr)
        return nullptr;
    else
    {
       return serviceRegistry->getAvailableServices();
    }
}



