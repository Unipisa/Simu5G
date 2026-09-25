//
//                  Simu5G
//
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef STACK_PHY_MEDIUM_CELLULARRADIOMEDIUM_H_
#define STACK_PHY_MEDIUM_CELLULARRADIOMEDIUM_H_

#include <omnetpp.h>

namespace simu5g {

using namespace omnetpp;

/**
 * The radio medium of a cellular network (see the NED documentation). It
 * holds no state and handles no messages yet.
 */
class CellularRadioMedium : public cSimpleModule
{
};

} // namespace simu5g

#endif
