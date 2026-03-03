#include "simu5g/stack/rlc/RlcLowerMux.h"
#include "simu5g/stack/rlc/um/UmTxEntity.h"
#include "simu5g/stack/rrc/BearerManagement.h"
#include "simu5g/common/LteControlInfoTags_m.h"
#include <inet/networklayer/common/NetworkInterface.h>
#include "simu5g/stack/d2dModeSelection/D2DModeSwitchNotification_m.h"
#include "simu5g/stack/mac/packet/LteMacSduRequest.h"
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"

namespace simu5g {

Define_Module(RlcLowerMux);

simsignal_t RlcLowerMux::receivedPacketFromLowerLayerSignal_ = registerSignal("receivedPacketFromLowerLayer");
simsignal_t RlcLowerMux::sentPacketToLowerLayerSignal_ = registerSignal("sentPacketToLowerLayer");

void RlcLowerMux::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        macInGate_ = gate("macIn");
        macOutGate_ = gate("macOut");

        bearerManagement_ = check_and_cast<BearerManagement *>(inet::getContainingNicModule(this)->getSubmodule("rrc")->getSubmodule("bearerManagement"));

        hasD2DSupport_ = inet::getContainingNicModule(this)->par("d2dCapable").boolValue();

        cModule *em = getModuleByPath(par("entityManagerModule").stringValue());
        nodeType_ = aToNodeType(em->par("nodeType").stdstringValue());

        WATCH_MAP(rxEntities_);
    }
}

void RlcLowerMux::handleMessage(cMessage *msg)
{
    cGate *incoming = msg->getArrivalGate();
    if (incoming == macInGate_) {
        fromMacLayer(check_and_cast<cPacket *>(msg));
    }
    else if (incoming->isName("fromTxEntity")) {
        // Packet from a TX entity — forward to MAC
        emit(sentPacketToLowerLayerSignal_, msg);
        send(msg, macOutGate_);
    }
    else {
        throw cRuntimeError("RlcLowerMux: unexpected message from gate %s", incoming->getFullName());
    }
}

void RlcLowerMux::fromMacLayer(cPacket *pktAux)
{
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    EV << "RlcLowerMux::fromMacLayer - Received packet " << pkt->getName() << " from lower layer\n";
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    auto chunk = pkt->peekAtFront<inet::Chunk>();

    ASSERT(pkt->findTag<PdcpTrackingTag>() == nullptr);

    // D2D: handle mode switch notification
    if (hasD2DSupport_ && inet::dynamicPtrCast<const D2DModeSwitchNotification>(chunk) != nullptr) {
        auto switchPkt = pkt->peekAtFront<D2DModeSwitchNotification>();

        if (switchPkt->getTxSide()) {
            // get the corresponding Tx buffer & call handler
            DrbKey id = ctrlInfoToTxDrbKey(lteInfo.get());
            RlcTxEntityBase *txbuf = bearerManagement_->lookupRlcTxBuffer(id);
            if (txbuf == nullptr)
                txbuf = bearerManagement_->createRlcTxBuffer(id, lteInfo.get());
            UmTxEntity *umTxbuf = check_and_cast<UmTxEntity *>(txbuf);
            umTxbuf->rlcHandleD2DModeSwitch(switchPkt->getOldConnection(), switchPkt->getClearRlcBuffer());

            // forward packet to PDCP via UpperMux
            EV << "RlcLowerMux::fromMacLayer - Forwarding D2D switch to PDCP via toUpperMux\n";
            send(pkt, "toUpperMux");
        }
        else { // rx side
            DrbKey id = ctrlInfoToRxDrbKey(lteInfo.get());
            RlcRxEntityBase *rxbuf = lookupRxBuffer(id);
            if (rxbuf == nullptr)
                rxbuf = bearerManagement_->createRlcRxBuffer(id, lteInfo.get());
            UmRxEntity *umRxbuf = check_and_cast<UmRxEntity *>(rxbuf);
            umRxbuf->rlcHandleD2DModeSwitch(switchPkt->getOldConnection(), switchPkt->getOldMode(), switchPkt->getClearRlcBuffer());

            delete pkt;
        }
        return;
    }

    if (inet::dynamicPtrCast<const LteMacSduRequest>(chunk) != nullptr) {
        // MAC SDU request — dispatch to TX entity via macToTxEntity gate
        DrbKey id = ctrlInfoToTxDrbKey(lteInfo.get());
        RlcTxEntityBase *txbuf = bearerManagement_->lookupRlcTxBuffer(id);
        ASSERT(txbuf != nullptr);

        send(pkt, txbuf->gate("macIn")->getPreviousGate());
    }
    else {
        // RLC PDU — dispatch to RX entity via toRxEntity gate
        emit(receivedPacketFromLowerLayerSignal_, pkt);

        DrbKey id = ctrlInfoToRxDrbKey(lteInfo.get());
        RlcRxEntityBase *rxbuf = lookupRxBuffer(id);
        ASSERT(rxbuf != nullptr);

        EV << "RlcLowerMux::fromMacLayer - Enqueue packet " << pkt->getName() << " into RX entity\n";
        send(pkt, rxbuf->gate("in")->getPreviousGate());
    }
}

RlcRxEntityBase *RlcLowerMux::lookupRxBuffer(DrbKey id)
{
    auto it = rxEntities_.find(id);
    return (it != rxEntities_.end()) ? it->second : nullptr;
}

void RlcLowerMux::registerRxBuffer(DrbKey id, RlcRxEntityBase *rxEnt)
{
    if (rxEntities_.find(id) != rxEntities_.end())
        throw cRuntimeError("RLC RX entity for %s already exists", id.str().c_str());
    rxEntities_[id] = rxEnt;
    EV << "RlcLowerMux::registerRxBuffer - Registered RX entity: " << rxEnt->getId() << " for " << id << "\n";
}

void RlcLowerMux::unregisterRxBuffer(DrbKey id)
{
    rxEntities_.erase(id);
}

void RlcLowerMux::activeUeUL(std::set<MacNodeId> *ueSet)
{
    for (const auto& [id, entity] : rxEntities_) {
        MacNodeId nodeId = id.getNodeId();
        UmRxEntity *umEnt = dynamic_cast<UmRxEntity *>(entity);
        if (umEnt != nullptr && (ueSet->find(nodeId) == ueSet->end()) && !umEnt->isEmpty())
            ueSet->insert(nodeId);
    }
}

} // namespace simu5g
