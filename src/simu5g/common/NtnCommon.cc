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

#include "simu5g/common/NtnCommon.h"

#include <inet/common/ModuleAccess.h>

#include "simu5g/common/binder/Binder.h"
#include "simu5g/stack/mac/LteMacBase.h"

namespace simu5g {

using namespace omnetpp;

simtime_t ntnCellRoundTripDelay(cSimpleModule *entity)
{
    auto *mac = inet::getModuleFromPar<LteMacBase>(entity->par("macModule"), entity);
    auto *binder = inet::getModuleFromPar<Binder>(entity->par("binderModule"), entity);

    // getMacCellId() is the serving gNodeB at a UE and the node itself at a gNodeB, which is the
    // identifier the Binder keys its NTN associations by in both cases.
    return binder->getNtnCellRoundTripDelay(mac->getMacCellId());
}

int ntnHarqTransmissions(cSimpleModule *entity)
{
    auto *mac = inet::getModuleFromPar<LteMacBase>(entity->par("macModule"), entity);
    return mac->par("maxHarqRtx").intValue() + 1;
}

} // namespace simu5g
