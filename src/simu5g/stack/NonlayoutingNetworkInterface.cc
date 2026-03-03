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

#include "simu5g/stack/NonlayoutingNetworkInterface.h"

namespace simu5g {

Define_Module(NonlayoutingNetworkInterface);

void NonlayoutingNetworkInterface::initialize(int stage)
{
    // Skip INITSTAGE_LAST to prevent INET's layoutSubmodulesWithoutGates()
    // from overwriting NED display string positions of gateless submodules.
    if (stage == inet::INITSTAGE_LAST)
        return;
    inet::NetworkInterface::initialize(stage);
}

} // namespace simu5g
