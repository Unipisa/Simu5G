#include "nodes/mec/MecCommon.h"

ResourceManager* mec::getResourceManager()
{
    return check_and_cast<ResourceManager*>(getSimulation()->getModuleByPath("meHost.virtualisationInfrastructure.resourceManager"));
}
