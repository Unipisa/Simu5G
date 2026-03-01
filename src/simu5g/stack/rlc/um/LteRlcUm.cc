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

#include "simu5g/stack/rlc/um/LteRlcUm.h"
#include "simu5g/stack/rlc/RlcUpperMux.h"
#include "simu5g/stack/rlc/RlcLowerMux.h"

namespace simu5g {

Define_Module(LteRlcUm);

using namespace omnetpp;

UmTxEntity *LteRlcUm::lookupTxBuffer(DrbKey id)
{
    return upperMux_->lookupTxBuffer(id);
}

UmTxEntity *LteRlcUm::createTxBuffer(DrbKey id, FlowControlInfo *lteInfo)
{
    return upperMux_->createTxBuffer(id, lteInfo);
}


UmRxEntity *LteRlcUm::lookupRxBuffer(DrbKey id)
{
    return lowerMux_->lookupRxBuffer(id);
}

UmRxEntity *LteRlcUm::createRxBuffer(DrbKey id, FlowControlInfo *lteInfo)
{
    return lowerMux_->createRxBuffer(id, lteInfo);
}


void LteRlcUm::deleteQueues(MacNodeId nodeId)
{
    Enter_Method_Silent();

    upperMux_->deleteTxEntities(nodeId);
    lowerMux_->deleteRxEntities(nodeId);
}

/*
 * Main functions
 */

void LteRlcUm::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        upperMux_ = check_and_cast<RlcUpperMux *>(getParentModule()->getSubmodule("upperMux"));
        lowerMux_ = check_and_cast<RlcLowerMux *>(getParentModule()->getSubmodule("lowerMux"));
    }
}

void LteRlcUm::resumeDownstreamInPackets(MacNodeId peerId)
{
    upperMux_->resumeDownstreamInPackets(peerId);
}

bool LteRlcUm::isEmptyingTxBuffer(MacNodeId peerId)
{
    return upperMux_->isEmptyingTxBuffer(peerId);
}

void LteRlcUm::activeUeUL(std::set<MacNodeId> *ueSet)
{
    lowerMux_->activeUeUL(ueSet);
}

void LteRlcUm::addUeThroughput(MacNodeId nodeId, Throughput throughput)
{
    auto& nodeUlThroughput = ulThroughput_[nodeId];
    nodeUlThroughput.pktSizeCount += throughput.pktSizeCount;
    nodeUlThroughput.time += throughput.time;
}

double LteRlcUm::getUeThroughput(MacNodeId nodeId)
{
    ULThroughputPerUE::iterator it = ulThroughput_.find(nodeId);
    return it == ulThroughput_.end() ? 0 : it->second.pktSizeCount / it->second.time.dbl();
}

void LteRlcUm::resetThroughputStats(MacNodeId nodeId)
{
    auto& nodeUlThroughput = ulThroughput_[nodeId];
    nodeUlThroughput.pktSizeCount = 0;
    nodeUlThroughput.time = 0;
}

} //namespace
