#include "simu5g/stack/rlc/RlcLowerMux.h"
#include "simu5g/stack/rlc/RlcUpperMux.h"
#include "simu5g/common/LteControlInfoTags_m.h"
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

        upperMux_ = check_and_cast<RlcUpperMux *>(getParentModule()->getSubmodule("upperMux"));

        hasD2DSupport_ = getParentModule()->par("d2dCapable").boolValue();

        // get RX entity module type and nodeType from the UM module
        cModule *um = getParentModule()->getSubmodule("um");
        rxEntityModuleType_ = cModuleType::get(um->par("rxEntityModuleType").stringValue());
        nodeType_ = aToNodeType(um->par("nodeType").stdstringValue());

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
            UmTxEntity *txbuf = upperMux_->lookupTxBuffer(id);
            if (txbuf == nullptr)
                txbuf = upperMux_->createTxBuffer(id, lteInfo.get());
            txbuf->rlcHandleD2DModeSwitch(switchPkt->getOldConnection(), switchPkt->getClearRlcBuffer());

            // forward packet to PDCP via UpperMux
            EV << "RlcLowerMux::fromMacLayer - Forwarding D2D switch to PDCP via toUpperMux\n";
            send(pkt, "toUpperMux");
        }
        else { // rx side
            DrbKey id = ctrlInfoToRxDrbKey(lteInfo.get());
            UmRxEntity *rxbuf = lookupRxBuffer(id);
            if (rxbuf == nullptr)
                rxbuf = createRxBuffer(id, lteInfo.get());
            rxbuf->rlcHandleD2DModeSwitch(switchPkt->getOldConnection(), switchPkt->getOldMode(), switchPkt->getClearRlcBuffer());

            delete pkt;
        }
        return;
    }

    if (inet::dynamicPtrCast<const LteMacSduRequest>(chunk) != nullptr) {
        // MAC SDU request — dispatch to TX entity via macToTxEntity gate
        DrbKey id = ctrlInfoToTxDrbKey(lteInfo.get());
        UmTxEntity *txbuf = upperMux_->lookupTxBuffer(id);
        ASSERT(txbuf != nullptr);

        send(pkt, txbuf->gate("macIn")->getPreviousGate());
    }
    else {
        // RLC PDU — dispatch to RX entity via toRxEntity gate
        emit(receivedPacketFromLowerLayerSignal_, pkt);

        DrbKey id = ctrlInfoToRxDrbKey(lteInfo.get());
        UmRxEntity *rxbuf = lookupRxBuffer(id);
        ASSERT(rxbuf != nullptr);

        EV << "RlcLowerMux::fromMacLayer - Enqueue packet " << pkt->getName() << " into RX entity\n";
        send(pkt, rxbuf->gate("in")->getPreviousGate());
    }
}

UmRxEntity *RlcLowerMux::lookupRxBuffer(DrbKey id)
{
    auto it = rxEntities_.find(id);
    return (it != rxEntities_.end()) ? it->second : nullptr;
}

UmRxEntity *RlcLowerMux::createRxBuffer(DrbKey id, FlowControlInfo *lteInfo)
{
    if (rxEntities_.find(id) != rxEntities_.end())
        throw cRuntimeError("RLC-UM connection RX entity for %s already exists", id.str().c_str());

    std::stringstream buf;
    buf << "UmRxEntity Lcid: " << id.getDrbId() << " cid: " << id.asPackedInt();
    auto *module = rxEntityModuleType_->create(buf.str().c_str(), getParentModule());
    module->finalizeParameters();
    module->buildInside();

    // Wire LowerMux → entity in gate
    int idx = gateSize("toRxEntity");
    setGateSize("toRxEntity", idx + 1);
    gate("toRxEntity", idx)->connectTo(module->gate("in"));

    // Wire entity out gate → UpperMux fromRxEntity
    int fromIdx = upperMux_->gateSize("fromRxEntity");
    upperMux_->setGateSize("fromRxEntity", fromIdx + 1);
    module->gate("out")->connectTo(upperMux_->gate("fromRxEntity", fromIdx));

    module->scheduleStart(simTime());
    module->callInitialize();

    UmRxEntity *rxEnt = check_and_cast<UmRxEntity *>(module);
    rxEntities_[id] = rxEnt;

    // configure entity
    rxEnt->setFlowControlInfo(lteInfo);

    EV << "RlcLowerMux::createRxBuffer - Added new UmRxEntity: " << rxEnt->getId() << " for " << id << "\n";

    return rxEnt;
}

void RlcLowerMux::deleteRxEntities(MacNodeId nodeId)
{
    Enter_Method_Silent();

    for (auto rit = rxEntities_.begin(); rit != rxEntities_.end();) {
        // D2D: if the entity refers to a D2D_MULTI connection, do not erase it
        if (hasD2DSupport_ && rit->second->isD2DMultiConnection()) {
            ++rit;
            continue;
        }

        if (nodeType_ == UE || (nodeType_ == NODEB && rit->first.getNodeId() == nodeId)) {
            rit->second->deleteModule();
            rit = rxEntities_.erase(rit);
        }
        else {
            ++rit;
        }
    }
}

void RlcLowerMux::activeUeUL(std::set<MacNodeId> *ueSet)
{
    for (const auto& [id, entity] : rxEntities_) {
        MacNodeId nodeId = id.getNodeId();
        if ((ueSet->find(nodeId) == ueSet->end()) && !entity->isEmpty())
            ueSet->insert(nodeId);
    }
}

} // namespace simu5g
