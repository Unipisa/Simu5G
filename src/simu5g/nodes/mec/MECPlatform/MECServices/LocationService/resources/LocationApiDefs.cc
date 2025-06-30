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

#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/LocationApiDefs.h"

namespace simu5g {

using namespace omnetpp;
namespace LocationUtils {

inet::Coord getCoordinates(Binder *binder, const MacNodeId id)
{
    OmnetId omnetId = binder->getOmnetId(id);
    if (omnetId == 0)
        return inet::Coord::NIL; // or throw exception?
    cModule *module = getSimulation()->getModule(omnetId);
    inet::IMobility *mobility_ = check_and_cast<inet::IMobility *>(module->getSubmodule("mobility"));
    return mobility_->getCurrentPosition();
}

inet::Coord getSpeed(Binder *binder, const MacNodeId id)
{
    OmnetId omnetId = binder->getOmnetId(id);
    if (omnetId == 0)
        return inet::Coord::NIL; // or throw exception?
    cModule *module = getSimulation()->getModule(omnetId);
    inet::IMobility *mobility_ = check_and_cast<inet::IMobility *>(module->getSubmodule("mobility"));
    return mobility_->getCurrentVelocity();
}

} // namespace LocationUtils

} //namespace

