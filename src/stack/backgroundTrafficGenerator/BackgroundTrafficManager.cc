//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/backgroundTrafficGenerator/BackgroundTrafficManager.h"
#include "stack/mac/layer/LteMacEnb.h"

Define_Module(BackgroundTrafficManager);

void BackgroundTrafficManager::initialize()
{
    numBgUEs_ = getAncestorPar("numBgUes");

    // get the reference to the MAC layer
    mac_ = check_and_cast<LteMacEnb*>(getParentModule()->getParentModule()->getSubmodule("mac"));

    // create vector of BackgroundUEs
    for (int i=0; i < numBgUEs_; i++)
        bgUe_.push_back(check_and_cast<TrafficGeneratorBase*>(getParentModule()->getSubmodule("bgUe", i)->getSubmodule("generator")));
}

void BackgroundTrafficManager::notifyBacklog(int index, Direction dir)
{
    if (dir != DL && dir != UL)
        throw cRuntimeError("TrafficGeneratorBase::consumeBytes - unrecognized direction: %d" , dir);

    backloggedBgUes_[dir].push_back(index);
}
