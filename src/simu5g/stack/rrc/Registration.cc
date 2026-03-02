//
//                  Simu5G
//
// Authors: Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/rrc/Registration.h"
#include "simu5g/common/InitStages.h"

namespace simu5g {

Define_Module(Registration);

void Registration::initialize(int stage)
{
}

int Registration::numInitStages() const
{
    return inet::NUM_INIT_STAGES;
}

void Registration::handleMessage(cMessage *msg)
{
    throw cRuntimeError("This module does not process messages");
}

} // namespace simu5g
