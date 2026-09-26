#include "simu5g/stack/d2d/rrc/D2DModeController.h"
#include "simu5g/stack/d2d/rlc/ID2dRlcUmTxEntity.h"
#include "simu5g/common/InitStages.h"

namespace simu5g {

Define_Module(D2DModeController);

void D2DModeController::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
        WATCH(perPeerTxEntities_);
}

void D2DModeController::registerD2DPeerTxEntity(MacNodeId peerId, ID2dRlcUmTxEntity *umTxEnt)
{
    if (peerId != NODEID_NONE)
        perPeerTxEntities_[peerId].insert(umTxEnt);

    // if other Tx buffers for this peer are already holding, the new one should hold too
    if (isEmptyingTxBuffer(peerId))
        umTxEnt->startHoldingDownstreamInPackets();
}

void D2DModeController::unregisterD2DPeerTxEntity(MacNodeId peerId, ID2dRlcUmTxEntity *umTxEnt)
{
    auto it = perPeerTxEntities_.find(peerId);
    if (it == perPeerTxEntities_.end())
        return;

    it->second.erase(umTxEnt);
    if (it->second.empty())
        perPeerTxEntities_.erase(it);
}

void D2DModeController::resumeDownstreamInPackets(MacNodeId peerId)
{
    if (peerId == NODEID_NONE || (perPeerTxEntities_.find(peerId) == perPeerTxEntities_.end()))
        return;

    for (auto& txEntity : perPeerTxEntities_.at(peerId)) {
        if (txEntity->isHoldingDownstreamInPackets())
            txEntity->resumeDownstreamInPackets();
    }
}

bool D2DModeController::isEmptyingTxBuffer(MacNodeId peerId)
{
    EV << NOW << " D2DModeController::isEmptyingTxBuffer - peerId " << peerId << endl;

    if (peerId == NODEID_NONE || (perPeerTxEntities_.find(peerId) == perPeerTxEntities_.end()))
        return false;

    for (auto& entity : perPeerTxEntities_.at(peerId)) {
        if (entity->isEmptyingBuffer()) {
            EV << NOW << " D2DModeController::isEmptyingTxBuffer - found " << endl;
            return true;
        }
    }
    return false;
}

} // namespace simu5g
