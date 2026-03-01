#include "simu5g/stack/rlc/RlcUpperMux.h"
#include "simu5g/stack/rlc/RlcLowerMux.h"
#include "simu5g/common/LteControlInfoTags_m.h"

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
        lowerMux_ = check_and_cast<RlcLowerMux *>(getParentModule()->getSubmodule("lowerMux"));

        hasD2DSupport_ = getParentModule()->par("d2dCapable").boolValue();

        // get TX entity module type and nodeType from the entity manager
        cModule *um = getParentModule()->getSubmodule("entityManager");
        txEntityModuleType_ = cModuleType::get(um->par("txEntityModuleType").stringValue());
        nodeType_ = aToNodeType(um->par("nodeType").stdstringValue());

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
    UmTxEntity *txbuf = lookupTxBuffer(id);
    ASSERT(txbuf != nullptr);

    send(pkt, txbuf->gate("in")->getPreviousGate());
}

UmTxEntity *RlcUpperMux::lookupTxBuffer(DrbKey id)
{
    auto it = txEntities_.find(id);
    return (it != txEntities_.end()) ? it->second : nullptr;
}

UmTxEntity *RlcUpperMux::createTxBuffer(DrbKey id, FlowControlInfo *lteInfo)
{
    if (txEntities_.find(id) != txEntities_.end())
        throw cRuntimeError("RLC-UM connection TX entity for %s already exists", id.str().c_str());

    std::stringstream buf;
    buf << "UmTxEntity Lcid: " << id.getDrbId() << " cid: " << id.asPackedInt();
    auto *module = txEntityModuleType_->create(buf.str().c_str(), getParentModule());
    module->finalizeParameters();
    module->buildInside();

    // Wire UpperMux → entity in gate
    int idx = gateSize("toTxEntity");
    setGateSize("toTxEntity", idx + 1);
    gate("toTxEntity", idx)->connectTo(module->gate("in"));

    // Wire entity out gate → LowerMux fromTxEntity
    int fromIdx = lowerMux_->gateSize("fromTxEntity");
    lowerMux_->setGateSize("fromTxEntity", fromIdx + 1);
    module->gate("out")->connectTo(lowerMux_->gate("fromTxEntity", fromIdx));

    // Wire LowerMux macToTxEntity → entity macIn gate
    int macIdx = lowerMux_->gateSize("macToTxEntity");
    lowerMux_->setGateSize("macToTxEntity", macIdx + 1);
    lowerMux_->gate("macToTxEntity", macIdx)->connectTo(module->gate("macIn"));

    module->scheduleStart(simTime());
    module->callInitialize();

    UmTxEntity *txEnt = check_and_cast<UmTxEntity *>(module);
    txEntities_[id] = txEnt;

    txEnt->setFlowControlInfo(lteInfo);

    // D2D: store per-peer map
    if (hasD2DSupport_) {
        MacNodeId d2dPeer = lteInfo->getD2dRxPeerId();
        if (d2dPeer != NODEID_NONE)
            perPeerTxEntities_[d2dPeer].insert(txEnt);

        // if other Tx buffers for this peer are already holding, the new one should hold too
        if (isEmptyingTxBuffer(d2dPeer))
            txEnt->startHoldingDownstreamInPackets();
    }

    EV << "RlcUpperMux::createTxBuffer - Added new UmTxEntity: " << txEnt->getId() << " for " << id << "\n";

    return txEnt;
}

void RlcUpperMux::deleteTxEntities(MacNodeId nodeId)
{
    Enter_Method_Silent();

    for (auto tit = txEntities_.begin(); tit != txEntities_.end();) {
        // D2D: if the entity refers to a D2D_MULTI connection, do not erase it
        if (hasD2DSupport_ && tit->second->isD2DMultiConnection()) {
            ++tit;
            continue;
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
