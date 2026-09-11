//
//                  Simu5G
//
// Copyright (C) 2026 Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/mac/UlBacklogRegistry.h"

#include "simu5g/stack/mac/buffer/LteMacBuffer.h"

namespace simu5g {

using namespace omnetpp;

UlBacklogRegistry::~UlBacklogRegistry()
{
    for (auto& [key, mirror] : mirrors_)
        delete mirror;
}

void UlBacklogRegistry::update(MacCid key, int64_t size, simtime_t timestamp)
{
    auto it = mirrors_.find(key);
    LteMacBuffer *mirror;
    if (it == mirrors_.end()) {
        mirror = new LteMacBuffer();
        mirrors_[key] = mirror;
        EV << "UlBacklogRegistry: new mirror for " << key << "\n";
    }
    else {
        mirror = it->second;
    }

    if (size > 0) {
        // replace the mirror's content with the reported figure
        PacketInfo report;
        if (!mirror->isEmpty())
            report = mirror->popFront();
        report.first = size;
        report.second = timestamp;
        mirror->pushBack(report);

        EV << "UlBacklogRegistry: " << key << " reports " << size << " bytes\n";

        if (backlogCallback_)
            backlogCallback_(key);
    }
    else {
        // the report said the queues behind this key are drained
        if (!mirror->isEmpty())
            mirror->popFront();

        EV << "UlBacklogRegistry: " << key << " reports empty\n";
    }
}

LteMacBuffer *UlBacklogRegistry::mirror(MacCid key) const
{
    auto it = mirrors_.find(key);
    if (it == mirrors_.end())
        throw cRuntimeError("UlBacklogRegistry::mirror - no mirror for key %s", key.str().c_str());
    return it->second;
}

std::vector<MacCid> UlBacklogRegistry::keys() const
{
    std::vector<MacCid> result;
    result.reserve(mirrors_.size());
    for (const auto& [key, mirror] : mirrors_)
        result.push_back(key);
    return result;
}

void UlBacklogRegistry::clearForNode(MacNodeId nodeId)
{
    for (auto& [key, mirror] : mirrors_) {
        if (key.getNodeId() != nodeId)
            continue;
        while (!mirror->isEmpty())
            mirror->popFront();
        EV << "UlBacklogRegistry: cleared mirror of " << key << "\n";
    }
}

void UlBacklogRegistry::eraseForNode(MacNodeId nodeId)
{
    for (auto it = mirrors_.begin(); it != mirrors_.end(); ) {
        if (it->first.getNodeId() == nodeId) {
            delete it->second;
            it = mirrors_.erase(it);
        }
        else {
            ++it;
        }
    }
}

} //namespace
