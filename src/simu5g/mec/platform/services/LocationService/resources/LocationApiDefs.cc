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

#include "simu5g/mec/platform/services/LocationService/resources/LocationApiDefs.h"

namespace simu5g {

using namespace omnetpp;
namespace LocationUtils {

inet::Coord getCoordinates(Binder *binder, const MacNodeId id)
{
    cModule *module = binder->getNodeModule(id);
    if (module == nullptr)
        return inet::Coord::NIL; // or throw exception?
    inet::IMobility *mobility_ = check_and_cast<inet::IMobility *>(module->getSubmodule("mobility"));
    return mobility_->getCurrentPosition();
}

inet::Coord getSpeed(Binder *binder, const MacNodeId id)
{
    cModule *module = binder->getNodeModule(id);
    if (module == nullptr)
        return inet::Coord::NIL; // or throw exception?
    inet::IMobility *mobility_ = check_and_cast<inet::IMobility *>(module->getSubmodule("mobility"));
    return mobility_->getCurrentVelocity();
}

} // namespace LocationUtils

} //namespace
