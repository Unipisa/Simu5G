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

#include "simu5g/stack/rlc/RlcEntityManager.h"
#include "simu5g/stack/rlc/RlcUpperMux.h"
#include "simu5g/stack/rlc/RlcLowerMux.h"

namespace simu5g {

Define_Module(RlcEntityManager);

using namespace omnetpp;

UmTxEntity *RlcEntityManager::lookupTxBuffer(DrbKey id)
{
    return upperMux_->lookupTxBuffer(id);
}

UmTxEntity *RlcEntityManager::createTxBuffer(DrbKey id, FlowControlInfo *lteInfo)
{
    return upperMux_->createTxBuffer(id, lteInfo);
}


UmRxEntity *RlcEntityManager::lookupRxBuffer(DrbKey id)
{
    return lowerMux_->lookupRxBuffer(id);
}

UmRxEntity *RlcEntityManager::createRxBuffer(DrbKey id, FlowControlInfo *lteInfo)
{
    return lowerMux_->createRxBuffer(id, lteInfo);
}


void RlcEntityManager::deleteQueues(MacNodeId nodeId)
{
    Enter_Method_Silent();

    upperMux_->deleteTxEntities(nodeId);
    lowerMux_->deleteRxEntities(nodeId);
}

/*
 * Main functions
 */

void RlcEntityManager::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        upperMux_ = check_and_cast<RlcUpperMux *>(getParentModule()->getSubmodule("upperMux"));
        lowerMux_ = check_and_cast<RlcLowerMux *>(getParentModule()->getSubmodule("lowerMux"));
    }
}

void RlcEntityManager::resumeDownstreamInPackets(MacNodeId peerId)
{
    upperMux_->resumeDownstreamInPackets(peerId);
}

bool RlcEntityManager::isEmptyingTxBuffer(MacNodeId peerId)
{
    return upperMux_->isEmptyingTxBuffer(peerId);
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
