#include "simu5g/stack/rlc/RlcUpperMux.h"
#include "simu5g/stack/rlc/RlcLowerMux.h"
#include "simu5g/common/LteControlInfoTags_m.h"
#include <inet/networklayer/common/NetworkInterface.h>

namespace simu5g {

Define_Module(RlcUpperMux);

simsignal_t RlcUpperMux::receivedPacketFromUpperLayerSignal_ = registerSignal("receivedPacketFromUpperLayer");
simsignal_t RlcUpperMux::sentPacketToUpperLayerSignal_ = registerSignal("sentPacketToUpperLayer");

void RlcUpperMux::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        upperLayerInGate_ = gate("upperLayerIn");
        upperLayerOutGate_ = gate("upperLayerOut");

        binder_.reference(this, "binderModule", true);
        lowerMux_ = check_and_cast<RlcLowerMux *>(getModuleByPath(par("lowerMuxModule").stringValue()));

        hasD2DSupport_ = inet::getContainingNicModule(this)->par("d2dCapable").boolValue();

        cModule *em = getModuleByPath(par("entityManagerModule").stringValue());
        nodeType_ = aToNodeType(em->par("nodeType").stdstringValue());

        WATCH_MAP(txEntities_);
    }
}

void RlcUpperMux::handleMessage(cMessage *msg)
{
    cGate *incoming = msg->getArrivalGate();
    if (incoming == upperLayerInGate_) {
        fromUpperLayer(check_and_cast<cPacket *>(msg));
    }
    else if (incoming->isName("fromRxEntity")) {
        // Packet from an RX entity — forward to upper layer
        emit(sentPacketToUpperLayerSignal_, msg);
        send(msg, upperLayerOutGate_);
    }
    else if (incoming->isName("fromLowerMux")) {
        // D2D switch notification forwarded from LowerMux to PDCP
        send(msg, upperLayerOutGate_);
    }
    else {
        throw cRuntimeError("RlcUpperMux: unexpected message from gate %s", incoming->getFullName());
    }
}

void RlcUpperMux::fromUpperLayer(cPacket *pktAux)
{
    emit(receivedPacketFromUpperLayerSignal_, pktAux);

    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();

    EV << "RlcUpperMux::fromUpperLayer - Received packet from upper layer, size " << pktAux->getByteLength() << "\n";

    DrbKey id = ctrlInfoToTxDrbKey(lteInfo.get());
    RlcTxEntityBase *txbuf = lookupTxBuffer(id);
    ASSERT(txbuf != nullptr);

    send(pkt, txbuf->gate("in")->getPreviousGate());
}

RlcTxEntityBase *RlcUpperMux::lookupTxBuffer(DrbKey id)
{
    auto it = txEntities_.find(id);
    return (it != txEntities_.end()) ? it->second : nullptr;
}

void RlcUpperMux::registerTxBuffer(DrbKey id, RlcTxEntityBase *txEnt)
{
    if (txEntities_.find(id) != txEntities_.end())
        throw cRuntimeError("RLC TX entity for %s already exists", id.str().c_str());
    txEntities_[id] = txEnt;
    EV << "RlcUpperMux::registerTxBuffer - Registered TX entity: " << txEnt->getId() << " for " << id << "\n";
}

void RlcUpperMux::unregisterTxBuffer(DrbKey id)
{
    txEntities_.erase(id);
}

void RlcUpperMux::registerD2DPeerTxEntity(MacNodeId peerId, UmTxEntity *umTxEnt)
{
    if (peerId != NODEID_NONE)
        perPeerTxEntities_[peerId].insert(umTxEnt);

    // if other Tx buffers for this peer are already holding, the new one should hold too
    if (isEmptyingTxBuffer(peerId))
        umTxEnt->startHoldingDownstreamInPackets();
}

void RlcUpperMux::deleteTxEntities(MacNodeId nodeId)
{
    Enter_Method_Silent();

    for (auto tit = txEntities_.begin(); tit != txEntities_.end();) {
        // D2D: if the entity refers to a D2D_MULTI connection, do not erase it
        if (hasD2DSupport_) {
            UmTxEntity *umEnt = dynamic_cast<UmTxEntity *>(tit->second);
            if (umEnt != nullptr && umEnt->isD2DMultiConnection()) {
                ++tit;
                continue;
            }
        }

        if (nodeType_ == UE || (nodeType_ == NODEB && tit->first.getNodeId() == nodeId)) {
            tit->second->deleteModule();
            tit = txEntities_.erase(tit);
        }
        else {
            ++tit;
        }
    }

    // D2D: clear per-peer tracking
    if (hasD2DSupport_)
        perPeerTxEntities_.clear();
}

void RlcUpperMux::resumeDownstreamInPackets(MacNodeId peerId)
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

bool RlcUpperMux::isEmptyingTxBuffer(MacNodeId peerId)
{
    if (!hasD2DSupport_)
        return false;

    EV << NOW << " RlcUpperMux::isEmptyingTxBuffer - peerId " << peerId << endl;

    if (peerId == NODEID_NONE || (perPeerTxEntities_.find(peerId) == perPeerTxEntities_.end()))
        return false;

    for (auto& entity : perPeerTxEntities_.at(peerId)) {
        if (entity->isEmptyingBuffer()) {
            EV << NOW << " RlcUpperMux::isEmptyingTxBuffer - found " << endl;
            return true;
        }
    }
    return false;
}

} // namespace simu5g
