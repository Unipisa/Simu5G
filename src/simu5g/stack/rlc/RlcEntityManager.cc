//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/rlc/RlcEntityManager.h"
#include "simu5g/stack/rlc/RlcMux.h"

namespace simu5g {

Define_Module(RlcEntityManager);

using namespace omnetpp;

void RlcEntityManager::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        lowerMux_ = check_and_cast<RlcMux *>(getModuleByPath(par("lowerMuxModule").stringValue()));
    }
}

} //namespace
