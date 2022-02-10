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

#include "stack/mac/scheduler/CarrierAssigner.h"

using namespace omnetpp;

void CarrierAssigner::initializeCarrierActiveConnectionSet(double carrierFrequency)
{
    ActiveSet aSet;
    carrierActiveConnectionSet_[carrierFrequency] = aSet;
}

ActiveSet* CarrierAssigner::getCarrierActiveConnectionSet(double carrierFrequency)
{
    if (carrierActiveConnectionSet_.find(carrierFrequency) == carrierActiveConnectionSet_.end())
        throw cRuntimeError("CarrierAssigner::getCarrierActiveConnectionSet - Carrier frequency %fGHz not found.", carrierFrequency);
    return &(carrierActiveConnectionSet_[carrierFrequency]);
}
