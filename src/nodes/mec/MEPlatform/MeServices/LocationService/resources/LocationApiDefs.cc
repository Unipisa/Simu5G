
#include "nodes/mec/MEPlatform/MeServices/LocationService/resources/LocationApiDefs.h"
using namespace omnetpp;
namespace LocationUtils
{


    inet::Coord getCoordinates(const MacNodeId id)
    {
        LteBinder* binder = getBinder();
        OmnetId omnetId = binder->getOmnetId(id);
        if(omnetId == 0)
            return inet::Coord::ZERO; // or throw exception?
        cModule* module = getSimulation()->getModule(omnetId);
        inet::IMobility *mobility_ = check_and_cast<inet::IMobility *>(module->getSubmodule("mobility"));
        return mobility_->getCurrentPosition();
    }

    inet::Coord getSpeed(const MacNodeId id)
    {
        LteBinder* binder = getBinder();
        OmnetId omnetId = binder->getOmnetId(id);
        if(omnetId == 0)
            return inet::Coord::ZERO; // or throw exception?
        cModule* module = getSimulation()->getModule(omnetId);
        inet::IMobility *mobility_ = check_and_cast<inet::IMobility *>(module->getSubmodule("mobility"));
        return mobility_->getCurrentVelocity();
    }
}
