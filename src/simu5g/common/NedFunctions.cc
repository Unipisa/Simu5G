//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include <inet/common/INETDefs.h>
#include "simu5g/common/LteDefs.h"

namespace simu5g {

using namespace omnetpp;

using namespace inet;

// This is a copy of the similar function in development version of INET -- remove once INET is released.

static cNEDValue nedf_seq(cComponent *context, cNEDValue argv[], int argc)
{
    static int handle = cSimulationOrSharedDataManager::registerSharedVariableName("simu5g::NedFunctions::seq");
    auto& seqs = getSimulationOrSharedDataManager()->getSharedVariable<std::map<std::string, int>>(handle);
    auto key = argv[0].stringValue();
    auto it = seqs.find(key);
    if (it == seqs.end())
        seqs[key] = 0;
    else
        seqs[key]++;
    return cNEDValue(seqs[key]);
}

Define_NED_Function2(nedf_seq,
        "int simu5g_seq(string id)",
        "misc",
        "Returns the next integer (starting from 0) from the sequence identified by the first argument as name."
        );

} //namespace
