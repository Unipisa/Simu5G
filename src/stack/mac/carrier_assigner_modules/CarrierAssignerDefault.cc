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

#include "stack/mac/carrier_assigner_modules/CarrierAssignerDefault.h"

using namespace omnetpp;

void CarrierAssignerDefault::assign()
{

    if (binder_ == NULL)
         binder_ = getBinder();

    const ActiveSet* activeConnectionSet = schedulerEnb_->readActiveConnections();

    // clear previous assignment
    std::map<double, ActiveSet>::iterator it = carrierActiveConnectionSet_.begin();
    for (; it != carrierActiveConnectionSet_.end(); ++it)
       it->second.clear();

    // for each active connection, add the id to all the carriers it can use
    ActiveSet::const_iterator ait = activeConnectionSet->begin();
    for (; ait != activeConnectionSet->end(); ++ait)
    {
        for (it = carrierActiveConnectionSet_.begin(); it != carrierActiveConnectionSet_.end(); ++it)
        {
            double carrierFrequency = it->first;

            // check if this UE can use this carrier
            const UeSet& carrierUeSet = binder_->getCarrierUeSet(carrierFrequency);
            if (carrierUeSet.find(MacCidToNodeId(*ait)) == carrierUeSet.end())
                continue;

            carrierActiveConnectionSet_[carrierFrequency].insert(*ait);
        }
    }
}
