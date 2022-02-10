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

#include "stack/mac/carrier_assigner_modules/CarrierAssignerBestChannel.h"

using namespace omnetpp;

void CarrierAssignerBestChannel::assign()
{

    if (binder_ == NULL)
         binder_ = getBinder();

    const ActiveSet* activeConnectionSet = schedulerEnb_->readActiveConnections();

    // clear previous assignment
    std::map<double, ActiveSet>::iterator it = carrierActiveConnectionSet_.begin();
    for (; it != carrierActiveConnectionSet_.end(); ++it)
       it->second.clear();

    // for each active connection, select the carrier with the best CQI
    ActiveSet::const_iterator ait = activeConnectionSet->begin();
    for (; ait != activeConnectionSet->end(); ++ait)
    {
        Cqi maxCqi = 0;
        double bestCarrierFrequency = 0;

        for (it = carrierActiveConnectionSet_.begin(); it != carrierActiveConnectionSet_.end(); ++it)
        {
            double carrierFrequency = it->first;

            // check if this UE can use this carrier
            const UeSet& carrierUeSet = binder_->getCarrierUeSet(carrierFrequency);
            if (carrierUeSet.find(MacCidToNodeId(*ait)) == carrierUeSet.end())
                continue;

            const UserTxParams& txParams = mac_->getAmc()->computeTxParams(MacCidToNodeId(*ait), direction_, carrierFrequency);
            Cqi cqi = txParams.readCqiVector().at(0);
            bestCarrierFrequency = (cqi > maxCqi) ? carrierFrequency : bestCarrierFrequency;
            maxCqi = (cqi > maxCqi) ? cqi : maxCqi;
        }
        if (bestCarrierFrequency > 0.0)
            carrierActiveConnectionSet_[bestCarrierFrequency].insert(*ait);
    }
}
