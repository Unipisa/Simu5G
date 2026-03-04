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

#include <inet/networklayer/common/NetworkInterface.h>

#include "simu5g/stack/rlc/RlcEntityManager.h"
#include "simu5g/stack/rlc/RlcMux.h"
#include "simu5g/stack/rlc/um/UmTxEntity.h"

namespace simu5g {

Define_Module(RlcEntityManager);

using namespace omnetpp;

void RlcEntityManager::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        lowerMux_ = check_and_cast<RlcMux *>(getModuleByPath(par("lowerMuxModule").stringValue()));
        hasD2DSupport_ = inet::getContainingNicModule(this)->par("d2dCapable").boolValue();
    }
}

void RlcEntityManager::registerD2DPeerTxEntity(MacNodeId peerId, UmTxEntity *umTxEnt)
{
    if (peerId != NODEID_NONE)
        perPeerTxEntities_[peerId].insert(umTxEnt);

    // if other Tx buffers for this peer are already holding, the new one should hold too
    if (isEmptyingTxBuffer(peerId))
        umTxEnt->startHoldingDownstreamInPackets();
}

void RlcEntityManager::resumeDownstreamInPackets(MacNodeId peerId)
{
    if (!hasD2DSupport_)
        return;

    if (peerId == NODEID_NONE || (perPeerTxEntities_.find(peerId) == perPeerTxEntities_.end()))
        return;

    for (auto& txEntity : perPeerTxEntities_.at(peerId)) {
        if (txEntity->isHoldingDownstreamInPackets())
            txEntity->resumeDownstreamInPackets();
    }
}

bool RlcEntityManager::isEmptyingTxBuffer(MacNodeId peerId)
{
    if (!hasD2DSupport_)
        return false;

    EV << NOW << " RlcEntityManager::isEmptyingTxBuffer - peerId " << peerId << endl;

    if (peerId == NODEID_NONE || (perPeerTxEntities_.find(peerId) == perPeerTxEntities_.end()))
        return false;

    for (auto& entity : perPeerTxEntities_.at(peerId)) {
        if (entity->isEmptyingBuffer()) {
            EV << NOW << " RlcEntityManager::isEmptyingTxBuffer - found " << endl;
            return true;
        }
    }
    return false;
}

void RlcEntityManager::activeUeUL(std::set<MacNodeId> *ueSet)
{
    lowerMux_->activeUeUL(ueSet);
}

void RlcEntityManager::addUeThroughput(MacNodeId nodeId, Throughput throughput)
{
    auto& nodeUlThroughput = ulThroughput_[nodeId];
    nodeUlThroughput.pktSizeCount += throughput.pktSizeCount;
    nodeUlThroughput.time += throughput.time;
}

double RlcEntityManager::getUeThroughput(MacNodeId nodeId)
{
    ULThroughputPerUE::iterator it = ulThroughput_.find(nodeId);
    return it == ulThroughput_.end() ? 0 : it->second.pktSizeCount / it->second.time.dbl();
}

void RlcEntityManager::resetThroughputStats(MacNodeId nodeId)
{
    auto& nodeUlThroughput = ulThroughput_[nodeId];
    nodeUlThroughput.pktSizeCount = 0;
    nodeUlThroughput.time = 0;
}

} //namespace
