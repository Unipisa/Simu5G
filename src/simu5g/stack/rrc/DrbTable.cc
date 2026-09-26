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

#include "simu5g/stack/rrc/DrbTable.h"

namespace simu5g {

Define_Module(DrbTable);

void DrbTable::initialize()
{
    WATCH(drbs_);
    WATCH(configuredDrbs_);
}

void DrbTable::handleMessage(cMessage *msg)
{
    throw cRuntimeError("This module does not process messages");
}

void DrbTable::refreshDisplay() const
{
    getDisplayString().setTagArg("t", 0, (std::to_string(drbs_.size()) + " DRBs").c_str());
}

const DrbDesc *DrbTable::findDrb(DrbKey key) const
{
    auto it = drbs_.find(key);
    return it != drbs_.end() ? &it->second : nullptr;
}

const DrbDesc *DrbTable::findConfiguredDrb(DrbKey key) const
{
    auto it = configuredDrbs_.find(key);
    return it != configuredDrbs_.end() ? &it->second : nullptr;
}

DrbDesc& DrbTable::getOrCreateDrb(DrbKey key)
{
    auto it = drbs_.find(key);
    if (it != drbs_.end())
        return it->second;
    DrbDesc& drb = drbs_[key];
    drb.key = key;
    return drb;
}

void DrbTable::removeDrb(DrbKey key)
{
    drbs_.erase(key);
}

void DrbTable::removeDrbsOfPeer(MacNodeId peerId)
{
    for (auto it = drbs_.begin(); it != drbs_.end(); ) {
        if (it->first.getNodeId() == peerId)
            it = drbs_.erase(it);
        else
            ++it;
    }
}

void DrbTable::removeAllDrbs()
{
    drbs_.clear();
}

} // namespace simu5g
