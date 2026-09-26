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

#include <inet/common/stlutils.h>
#include <inet/networklayer/common/NetworkInterface.h>

#include "simu5g/stack/d2d/binder/D2dBinder.h"
#include "simu5g/stack/d2d/mac/ID2dMacUe.h"
#include "simu5g/stack/mac/LteMacBase.h"
#include "simu5g/stack/phy/PhyBase.h"

namespace simu5g {

using namespace omnetpp;
using namespace inet;

Define_Module(D2dBinder);

void D2dBinder::initialize()
{
    binder_.reference(this, "binderModule", true);
    binder_->subscribe(Binder::nodeUnregisteredSignal_, this);
    WATCH(multicastTransmitterSet_);
    WATCH(multicastFlows_);
    WATCH(d2dPeeringMap_);
}

void D2dBinder::receiveSignal(cComponent *source, simsignal_t signalID, long nodeId, cObject *details)
{
    ASSERT(signalID == Binder::nodeUnregisteredSignal_);
    MacNodeId id = MacNodeId(nodeId);

    // as the subject of an entry, and as the peer inside every other UE's map
    d2dPeeringMap_.erase(id);
    for (auto& [peer, peerMap] : d2dPeeringMap_)
        peerMap.erase(id);

    multicastTransmitterSet_.erase(id);

    // the pools are keyed by node pair, so the departed id sits in every pair it took part in
    for (auto it = sidelinkDrbIds_.begin(); it != sidelinkDrbIds_.end(); ) {
        if (it->first.first == id || it->first.second == id)
            it = sidelinkDrbIds_.erase(it);
        else
            ++it;
    }

    // The remembered multicast flows are keyed by group but owned by their sender: drop the
    // ones this node established, or multicastGroupJoined() would keep handing later joiners
    // an RX leg keyed to a sender that no longer transmits -- and, since the RX descriptor's
    // MacCid carries that sender's id, the PDUs of whichever node took over the group would
    // then arrive on a connection the joiner has no descriptor for. A replacement sender's
    // createMulticastConnection() stores a fresh flow, so the group keeps working.
    for (auto it = multicastFlows_.begin(); it != multicastFlows_.end(); ) {
        if (it->first.second == id)
            it = multicastFlows_.erase(it);
        else
            ++it;
    }
}

std::set<DrbId>& D2dBinder::sidelinkDrbIdPool(const std::pair<MacNodeId, MacNodeId>& pair)
{
    ASSERT(pair.first <= pair.second);
    return sidelinkDrbIds_[pair];
}

void D2dBinder::rememberMulticastFlow(MacNodeId groupId, MacNodeId senderId, const FlowId& flow, const BearerRequest& req, bool withPdcp)
{
    auto flowKey = std::make_pair(groupId, senderId);
    if (multicastFlows_.find(flowKey) == multicastFlows_.end())
        multicastFlows_[flowKey] = { flow, req, withPdcp };
}

LteD2DMode D2dBinder::computeD2DCapability(MacNodeId src, MacNodeId dst)
{
    LteMacBase *dstMac = binder_->getMacFromMacNodeId(dst);
    if (dynamic_cast<ID2dMacUe *>(dstMac) != nullptr) {
        // set the initial mode
        if (binder_->getServingNode(src) == binder_->getServingNode(dst)) {
            // if served by the same cell, then the mode is selected according to the corresponding parameter
            LteMacBase *srcMac = binder_->getMacFromMacNodeId(src);
            inet::NetworkInterface *srcNic = getContainingNicModule(srcMac);
            bool d2dInitialMode = srcNic->hasPar("d2dInitialMode") ? srcNic->par("d2dInitialMode").boolValue() : false;
            return d2dInitialMode ? DM : IM;
        }
        else {
            // if served by different cells, then the mode can be IM only
            return IM;
        }
    }
    else {
        // this is not a D2D-capable flow
        return NONE;
    }
}

bool D2dBinder::checkD2DCapability(MacNodeId src, MacNodeId dst)
{
    ASSERT(getNodeTypeById(src) == UE && binder_->nodeExists(src));
    ASSERT(getNodeTypeById(dst) == UE && binder_->nodeExists(dst));

    // if the entry is missing, check if the receiver is D2D capable and update the map
    if (!containsKey(d2dPeeringMap_, src) || !containsKey(d2dPeeringMap_[src], dst)) {
        LteD2DMode mode = computeD2DCapability(src, dst);
        if (mode == NONE) {
            EV << "D2dBinder::checkD2DCapability - UE " << src << " may not transmit to UE " << dst << " using D2D (UE " << dst << " is not D2D capable)" << endl;
        }
        else {
            EV << "D2dBinder::checkD2DCapability - UE " << src << " may transmit to UE " << dst << " using D2D (current mode " << (mode == DM ? "DM)" : "IM)") << endl;
        }
        d2dPeeringMap_[src][dst] = mode;
        return mode != NONE;
    }

    // if an entry is present, and it is not NONE, this is a D2D-capable flow
    return d2dPeeringMap_[src][dst] != NONE;
}

bool D2dBinder::getD2DCapability(MacNodeId src, MacNodeId dst)
{
    ASSERT(getNodeTypeById(src) == UE && binder_->nodeExists(src));
    ASSERT(getNodeTypeById(dst) == UE && binder_->nodeExists(dst));

    // return true if the entry exists and it is not NONE, no matter if it is DM or IM
    return containsKey(d2dPeeringMap_, src) && containsKey(d2dPeeringMap_[src], dst) && d2dPeeringMap_[src][dst] != NONE;
}

LteD2DMode D2dBinder::getD2DMode(MacNodeId src, MacNodeId dst)
{
    if (!getD2DCapability(src, dst))
        throw cRuntimeError("D2dBinder::getD2DMode - Node Id not valid. Src %hu Dst %hu", num(src), num(dst));

    return d2dPeeringMap_[src][dst];
}

void D2dBinder::registerD2dPhy(MacNodeId nodeId, PhyBase *phy)
{
    d2dPhys_[nodeId] = phy;
}

PhyBase *D2dBinder::getD2dPhy(MacNodeId nodeId)
{
    auto it = d2dPhys_.find(nodeId);
    return it == d2dPhys_.end() ? nullptr : it->second.get();
}

void D2dBinder::setD2DMode(MacNodeId src, MacNodeId dst, LteD2DMode mode)
{
    d2dPeeringMap_[src][dst] = mode;
}

void D2dBinder::addD2DMulticastTransmitter(MacNodeId nodeId)
{
    multicastTransmitterSet_.insert(nodeId);
}

std::set<MacNodeId>& D2dBinder::getD2DMulticastTransmitters()
{
    return multicastTransmitterSet_;
}

} //namespace
