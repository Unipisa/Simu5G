#include "simu5g/stack/pdcp/UpperMux.h"
#include "simu5g/stack/pdcp/LowerMux.h"
#include "simu5g/stack/pdcp/PdcpOutputRoutingTag_m.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

Define_Module(UpperMux);

using namespace omnetpp;
using namespace inet;

void UpperMux::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        upperLayerInGate_ = gate("upperLayerIn");
        upperLayerOutGate_ = gate("upperLayerOut");

        binder_.reference(this, "binderModule", true);
        nodeId_ = MacNodeId(getContainingNode(this)->par("macNodeId").intValue());

        lowerMux_ = check_and_cast<LowerMux *>(getParentModule()->getSubmodule("lowerMux"));

        const char *txEntityModuleTypeName = getParentModule()->par("txEntityModuleType").stringValue();
        txEntityModuleType_ = cModuleType::get(txEntityModuleTypeName);

        WATCH(nodeId_);
    }
}

void UpperMux::handleMessage(cMessage *msg)
{
    cPacket *pkt = check_and_cast<cPacket *>(msg);
    cGate *incoming = pkt->getArrivalGate();

    if (incoming == upperLayerInGate_) {
        fromDataPort(pkt);
    }
    else if (incoming->isName("fromRxEntity")) {
        // Packet from an RX entity — route to upper layer
        auto inetPkt = check_and_cast<Packet *>(pkt);
        auto routeTag = inetPkt->removeTag<PdcpOutputRoutingTag>();
        if (routeTag->getRoute() != PDCP_OUT_UPPER)
            throw cRuntimeError("UpperMux: unexpected route %d from RX entity", (int)routeTag->getRoute());
        send(pkt, upperLayerOutGate_);
    }
    else {
        throw cRuntimeError("UpperMux: unexpected message from gate %s", incoming->getFullName());
    }
}

void UpperMux::fromDataPort(cPacket *pktAux)
{
    auto pkt = check_and_cast<inet::Packet *>(pktAux);

    auto lteInfo = pkt->getTag<FlowControlInfo>();
    verifyControlInfo(lteInfo.get());

    DrbKey id = DrbKey(lteInfo->getDestId(), lteInfo->getDrbId());
    PdcpTxEntityBase *entity = lookupTxEntity(id);

    EV << "fromDataPort in " << getFullPath() << " event #" << getSimulation()->getEventNumber()
       << ": Processing packet " << pkt->getName() << " src=" << lteInfo->getSourceId() << " dest=" << lteInfo->getDestId()
       << " multicast=" << lteInfo->getMulticastGroupId() << " direction=" << dirToA((Direction)lteInfo->getDirection())
       << " ---> " << id << (entity == nullptr ? " (NEW)" : " (existing)") << std::endl;

    if (entity == nullptr) {
        binder_->establishUnidirectionalDataConnection((FlowControlInfo *)lteInfo.get());
        entity = lookupTxEntity(id);
        ASSERT(entity != nullptr);
    }

    send(pkt, entity->gate("in")->getPreviousGate());
}

PdcpTxEntityBase *UpperMux::lookupTxEntity(DrbKey id)
{
    auto it = txEntities_.find(id);
    return it != txEntities_.end() ? it->second : nullptr;
}

PdcpTxEntityBase *UpperMux::createTxEntity(DrbKey id)
{
    std::stringstream buf;
    buf << "tx-" << id.getNodeId() << "-" << id.getDrbId();
    auto *module = txEntityModuleType_->create(buf.str().c_str(), getParentModule());
    module->par("headerCompressedSize") = getParentModule()->par("headerCompressedSize");
    module->finalizeParameters();
    module->buildInside();

    // Wire UpperMux output gate to entity input gate
    int idx = gateSize("toTxEntity");
    setGateSize("toTxEntity", idx + 1);
    gate("toTxEntity", idx)->connectTo(module->gate("in"));

    // Wire entity output gate to LowerMux input gate
    int fromIdx = lowerMux_->gateSize("fromTxEntity");
    lowerMux_->setGateSize("fromTxEntity", fromIdx + 1);
    module->gate("out")->connectTo(lowerMux_->gate("fromTxEntity", fromIdx));

    module->scheduleStart(simTime());
    module->callInitialize();
    PdcpTxEntityBase *txEnt = check_and_cast<PdcpTxEntityBase *>(module);
    txEntities_[id] = txEnt;

    EV << "UpperMux::createTxEntity - Added new TxPdcpEntity for " << id << "\n";

    return txEnt;
}

void UpperMux::deleteTxEntities(MacNodeId nodeId)
{
    bool isEnb = (getNodeTypeById(nodeId_) == NODEB);
    bool isNR = getParentModule()->par("isNR").boolValue();

    if (isEnb || isNR) {
        for (auto tit = txEntities_.begin(); tit != txEntities_.end(); ) {
            auto& [id, txEntity] = *tit;
            if (id.getNodeId() == nodeId) {
                txEntity->deleteModule();
                tit = txEntities_.erase(tit);
            }
            else {
                ++tit;
            }
        }
    }
    else {
        for (auto& [txId, txEntity] : txEntities_)
            txEntity->deleteModule();
        txEntities_.clear();
    }
}

} // namespace simu5g
