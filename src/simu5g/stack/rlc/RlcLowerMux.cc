#include "simu5g/stack/rlc/RlcLowerMux.h"
#include "simu5g/stack/rlc/RlcUpperMux.h"

namespace simu5g {

Define_Module(RlcLowerMux);

simsignal_t RlcLowerMux::sentPacketToLowerLayerSignal_ = registerSignal("sentPacketToLowerLayer");

void RlcLowerMux::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        macInGate_ = gate("macIn");
        macOutGate_ = gate("macOut");
        toUmGate_ = gate("toUm");
        fromUmGate_ = gate("fromUm");

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
        send(msg, toUmGate_);
    }
    else if (incoming == fromUmGate_) {
        send(msg, macOutGate_);
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
