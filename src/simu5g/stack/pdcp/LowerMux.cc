#include "simu5g/stack/pdcp/LowerMux.h"
#include "simu5g/stack/pdcp/UpperMux.h"
#include "simu5g/stack/pdcp/PdcpOutputRoutingTag_m.h"
#include "simu5g/common/LteControlInfoTags_m.h"
#include "simu5g/stack/d2dModeSelection/D2DModeSwitchNotification_m.h"
#include "simu5g/x2/packet/X2ControlInfo_m.h"
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"
#include <inet/networklayer/common/NetworkInterface.h>

namespace simu5g {

Define_Module(LowerMux);

using namespace omnetpp;
using namespace inet;

void LowerMux::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        rlcInGate_ = gate("rlcIn");
        rlcOutGate_ = gate("rlcOut");

        binder_.reference(this, "binderModule", true);
        nodeId_ = MacNodeId(getContainingNode(this)->par("macNodeId").intValue());

        upperMux_ = check_and_cast<UpperMux *>(getParentModule()->getSubmodule("upperMux"));

        isNR_ = par("isNR").boolValue();
        hasD2DSupport_ = par("hasD2DSupport").boolValue();

        const char *rxEntityModuleTypeName = getParentModule()->par("rxEntityModuleType").stringValue();
        rxEntityModuleType_ = cModuleType::get(rxEntityModuleTypeName);

        const char *bypassRxEntityModuleTypeName = getParentModule()->par("bypassRxEntityModuleType").stringValue();
        bypassRxEntityModuleType_ = cModuleType::get(bypassRxEntityModuleTypeName);

        const char *bypassTxEntityModuleTypeName = getParentModule()->par("bypassTxEntityModuleType").stringValue();
        bypassTxEntityModuleType_ = cModuleType::get(bypassTxEntityModuleTypeName);

        if (isNR_) {
            inet::NetworkInterface *nic = inet::getContainingNicModule(this);
            dualConnectivityEnabled_ = nic->par("dualConnectivityEnabled").boolValue();

            if (getNodeTypeById(nodeId_) == UE) {
                nrRlcOutGate_ = gate("nrRlcOut");
            }
        }

        WATCH(nodeId_);
    }
}

void LowerMux::handleMessage(cMessage *msg)
{
    cPacket *pkt = check_and_cast<cPacket *>(msg);
    EV << "LowerMux : Received packet " << pkt->getName() << " from port "
       << pkt->getArrivalGate()->getName() << endl;

    // NrPdcpEnb: incoming data from DualConnectivityManager via X2
    if (isNR_ && getNodeTypeById(nodeId_) == NODEB && msg->getArrivalGate()->isName("dcManagerIn")) {
        EV << "LowerMux::handleMessage - Received packet from DualConnectivityManager" << endl;
        auto datagram = check_and_cast<Packet*>(pkt);
        auto tag = datagram->removeTag<X2SourceNodeInd>();
        MacNodeId sourceNode = tag->getSourceNode();
        receiveDataFromSourceNode(datagram, sourceNode);
        return;
    }

    // D2D mode switch notification
    if (hasD2DSupport_) {
        auto inet_pkt = check_and_cast<inet::Packet *>(pkt);
        auto chunk = inet_pkt->peekAtFront<Chunk>();
        if (inet::dynamicPtrCast<const D2DModeSwitchNotification>(chunk) != nullptr) {
            EV << "LowerMux::handleMessage - Received packet " << pkt->getName() << " from port " << pkt->getArrivalGate()->getName() << endl;
            auto switchPkt = inet_pkt->peekAtFront<D2DModeSwitchNotification>();
            pdcpHandleD2DModeSwitch(switchPkt->getPeerId(), switchPkt->getNewMode());
            delete pkt;
            return;
        }
    }

    cGate *incoming = pkt->getArrivalGate();
    if (incoming == rlcInGate_ || incoming->isName("nrRlcIn")) {
        fromLowerLayer(pkt);
    }
    else if (incoming->isName("fromTxEntity")) {
        // Packet from a TX entity — route based on PdcpOutputRoutingTag
        auto inetPkt = check_and_cast<Packet *>(pkt);
        auto routeTag = inetPkt->removeTag<PdcpOutputRoutingTag>();
        switch (routeTag->getRoute()) {
            case PDCP_OUT_RLC:
                send(pkt, rlcOutGate_);
                break;
            case PDCP_OUT_NR_RLC:
                send(pkt, nrRlcOutGate_);
                break;
            case PDCP_OUT_X2:
                send(pkt, "dcManagerOut");
                break;
            default:
                throw cRuntimeError("LowerMux: unexpected route %d from TX entity", (int)routeTag->getRoute());
        }
    }
    else if (incoming->isName("fromBypassRxEntity")) {
        // Packet from a bypass RX entity — route to DC manager
        auto inetPkt = check_and_cast<Packet *>(pkt);
        auto routeTag = inetPkt->removeTag<PdcpOutputRoutingTag>();
        if (routeTag->getRoute() != PDCP_OUT_X2)
            throw cRuntimeError("LowerMux: unexpected route %d from bypass RX entity", (int)routeTag->getRoute());
        send(pkt, "dcManagerOut");
    }
    else {
        throw cRuntimeError("LowerMux: unexpected message from gate %s", incoming->getFullName());
    }
}

void LowerMux::fromLowerLayer(cPacket *pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);

    // NrPdcpEnb: trim packet for NR gNBs
    if (isNR_ && getNodeTypeById(nodeId_) == NODEB) {
        pkt->trim();
    }

    ASSERT(pkt->findTag<PdcpTrackingTag>() == nullptr);

    auto lteInfo = pkt->getTag<FlowControlInfo>();
    DrbKey id = DrbKey(lteInfo->getSourceId(), lteInfo->getDrbId());

    // UE with DC: both RLC legs must reach the same reordering entity
    if (isDualConnectivityEnabled() && getNodeTypeById(nodeId_) == UE
            && lteInfo->getMulticastGroupId() == NODEID_NONE) {
        id = DrbKey(binder_->getServingNode(nodeId_), lteInfo->getDrbId());
    }

    PdcpRxEntityBase *entity = lookupRxEntity(id);

    EV << "fromLowerLayer in " << getFullPath() << " event #" << getSimulation()->getEventNumber()
       <<  ": Processing packet " << pkt->getName() << " src=" << lteInfo->getSourceId() << " dest=" << lteInfo->getDestId()
       << " multicast=" << lteInfo->getMulticastGroupId() << " direction=" << dirToA((Direction)lteInfo->getDirection())
       << " ---> " << id << (entity == nullptr ? " (NEW)" : " (existing)") << std::endl;

    ASSERT(entity != nullptr);

    send(pkt, entity->gate("in")->getPreviousGate());
}

void LowerMux::receiveDataFromSourceNode(inet::Packet *pkt, MacNodeId sourceNode)
{
    auto ctrlInfo = pkt->getTag<FlowControlInfo>();
    if (ctrlInfo->getDirection() == DL) {
        MacNodeId destId = ctrlInfo->getDestId();
        EV << NOW << " LowerMux::receiveDataFromSourceNode - Received PDCP PDU from master node with id " << sourceNode << " - destination node[" << destId << "]" << endl;

        DrbKey id = DrbKey(destId, ctrlInfo->getDrbId());
        PdcpTxEntityBase *entity = lookupBypassTxEntity(id);
        ASSERT(entity != nullptr);
        send(pkt, entity->gate("in")->getPreviousGate());
    }
    else { // UL
        EV << NOW << " LowerMux::receiveDataFromSourceNode - Received PDCP PDU from secondary node with id " << sourceNode << endl;
        fromLowerLayer(pkt);
    }
}

void LowerMux::pdcpHandleD2DModeSwitch(MacNodeId peerId, LteD2DMode newMode)
{
    EV << NOW << " LowerMux::pdcpHandleD2DModeSwitch - peering with UE " << peerId << " set to " << d2dModeToA(newMode) << endl;
}

PdcpRxEntityBase *LowerMux::lookupRxEntity(DrbKey id)
{
    auto it = rxEntities_.find(id);
    return it != rxEntities_.end() ? it->second : nullptr;
}

PdcpRxEntityBase *LowerMux::createRxEntity(DrbKey id)
{
    std::stringstream buf;
    buf << "rx-" << id.getNodeId() << "-" << id.getDrbId();
    auto *module = rxEntityModuleType_->create(buf.str().c_str(), getParentModule());
    module->par("headerCompressedSize") = getParentModule()->par("headerCompressedSize");
    module->finalizeParameters();
    module->buildInside();

    // Wire LowerMux output gate to entity input gate
    int idx = gateSize("toRxEntity");
    setGateSize("toRxEntity", idx + 1);
    gate("toRxEntity", idx)->connectTo(module->gate("in"));

    // Wire entity output gate to UpperMux input gate
    int fromIdx = upperMux_->gateSize("fromRxEntity");
    upperMux_->setGateSize("fromRxEntity", fromIdx + 1);
    module->gate("out")->connectTo(upperMux_->gate("fromRxEntity", fromIdx));

    module->scheduleStart(simTime());
    module->callInitialize();
    PdcpRxEntityBase *rxEnt = check_and_cast<PdcpRxEntityBase *>(module);
    rxEntities_[id] = rxEnt;

    EV << "LowerMux::createRxEntity - Added new RxPdcpEntity for " << id << "\n";

    return rxEnt;
}

PdcpTxEntityBase *LowerMux::lookupBypassTxEntity(DrbKey id)
{
    auto it = bypassTxEntities_.find(id);
    return it != bypassTxEntities_.end() ? it->second : nullptr;
}

PdcpTxEntityBase *LowerMux::createBypassTxEntity(DrbKey id)
{
    std::stringstream buf;
    buf << "bypass-tx-" << id.getNodeId() << "-" << id.getDrbId();
    auto *module = bypassTxEntityModuleType_->create(buf.str().c_str(), getParentModule());
    module->finalizeParameters();
    module->buildInside();

    // Wire LowerMux output gate to entity input gate
    int idx = gateSize("toBypassTxEntity");
    setGateSize("toBypassTxEntity", idx + 1);
    gate("toBypassTxEntity", idx)->connectTo(module->gate("in"));

    // Wire entity output gate back to LowerMux input gate (same collection as normal TX)
    int fromIdx = gateSize("fromTxEntity");
    setGateSize("fromTxEntity", fromIdx + 1);
    module->gate("out")->connectTo(gate("fromTxEntity", fromIdx));

    module->scheduleStart(simTime());
    module->callInitialize();
    PdcpTxEntityBase *txEnt = check_and_cast<PdcpTxEntityBase *>(module);
    bypassTxEntities_[id] = txEnt;

    EV << "LowerMux::createBypassTxEntity - Added new BypassTxPdcpEntity for " << id << "\n";

    return txEnt;
}

PdcpRxEntityBase *LowerMux::createBypassRxEntity(DrbKey id)
{
    std::stringstream buf;
    buf << "bypass-rx-" << id.getNodeId() << "-" << id.getDrbId();
    auto *module = bypassRxEntityModuleType_->create(buf.str().c_str(), getParentModule());
    module->finalizeParameters();
    module->buildInside();

    // Wire LowerMux output gate to entity input gate (same dispatch as normal RX)
    int idx = gateSize("toRxEntity");
    setGateSize("toRxEntity", idx + 1);
    gate("toRxEntity", idx)->connectTo(module->gate("in"));

    // Wire entity output gate to LowerMux bypass RX input (routes to dcManagerOut)
    int fromIdx = gateSize("fromBypassRxEntity");
    setGateSize("fromBypassRxEntity", fromIdx + 1);
    module->gate("out")->connectTo(gate("fromBypassRxEntity", fromIdx));

    module->scheduleStart(simTime());
    module->callInitialize();
    PdcpRxEntityBase *rxEnt = check_and_cast<PdcpRxEntityBase *>(module);
    rxEntities_[id] = rxEnt;

    EV << "LowerMux::createBypassRxEntity - Added new BypassRxPdcpEntity for " << id << "\n";

    return rxEnt;
}

void LowerMux::deleteRxAndBypassEntities(MacNodeId nodeId)
{
    bool isEnb = (getNodeTypeById(nodeId_) == NODEB);

    if (isEnb || isNR_) {
        for (auto rit = rxEntities_.begin(); rit != rxEntities_.end(); ) {
            auto& [id, rxEntity] = *rit;
            if (id.getNodeId() == nodeId) {
                rxEntity->deleteModule();
                rit = rxEntities_.erase(rit);
            }
            else {
                ++rit;
            }
        }
        for (auto tit = bypassTxEntities_.begin(); tit != bypassTxEntities_.end(); ) {
            auto& [id, txEntity] = *tit;
            if (id.getNodeId() == nodeId) {
                txEntity->deleteModule();
                tit = bypassTxEntities_.erase(tit);
            }
            else {
                ++tit;
            }
        }
    }
    else {
        for (auto& [rxId, rxEntity] : rxEntities_)
            rxEntity->deleteModule();
        rxEntities_.clear();

        for (auto& [txId, txEntity] : bypassTxEntities_)
            txEntity->deleteModule();
        bypassTxEntities_.clear();
    }
}

void LowerMux::activeUeUL(std::set<MacNodeId> *ueSet)
{
    for (const auto& [id, rxEntity] : rxEntities_) {
        MacNodeId nodeId = id.getNodeId();
        if (!(rxEntity->isEmpty()))
            ueSet->insert(nodeId);
    }
}

} // namespace simu5g
