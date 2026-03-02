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

#include "simu5g/stack/pdcp/DcMux.h"
#include "simu5g/stack/pdcp/LowerMux.h"
#include "simu5g/common/LteControlInfoTags_m.h"
#include "simu5g/x2/packet/X2ControlInfo_m.h"
#include <inet/common/ModuleAccess.h>

namespace simu5g {

Define_Module(DcMux);

void DcMux::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        binder_.reference(this, "binderModule", true);
        nodeId_ = MacNodeId(inet::getContainingNode(this)->par("macNodeId").intValue());

        lowerMux_ = check_and_cast<LowerMux *>(getParentModule()->getSubmodule("lowerMux"));

        dcManagerInGate_ = gate("dcManagerIn");

        const char *bypassTxEntityModuleTypeName = getParentModule()->par("bypassTxEntityModuleType").stringValue();
        bypassTxEntityModuleType_ = cModuleType::get(bypassTxEntityModuleTypeName);
    }
}

void DcMux::handleMessage(cMessage *msg)
{
    cGate *incoming = msg->getArrivalGate();
    if (incoming == dcManagerInGate_) {
        // Incoming from DC manager via X2
        auto pkt = check_and_cast<inet::Packet *>(msg);
        auto tag = pkt->removeTag<X2SourceNodeInd>();
        MacNodeId sourceNode = tag->getSourceNode();

        auto ctrlInfo = pkt->getTag<FlowControlInfo>();
        if (ctrlInfo->getDirection() == DL) {
            // DL: master sent data for a UE — dispatch to bypass TX entity
            MacNodeId destId = ctrlInfo->getDestId();
            DrbKey id = DrbKey(destId, ctrlInfo->getDrbId());
            PdcpTxEntityBase *entity = lookupBypassTxEntity(id);
            ASSERT(entity != nullptr);

            EV << NOW << " DcMux::handleMessage - Received DL PDCP PDU from master node " << sourceNode
               << " for UE " << destId << " - dispatching to bypass TX entity" << endl;
            send(pkt, entity->gate("in")->getPreviousGate());
        }
        else {
            // UL: secondary forwarded UL data — dispatch directly to RX entity
            auto lteInfo = pkt->getTag<FlowControlInfo>();
            DrbKey id = DrbKey(lteInfo->getSourceId(), lteInfo->getDrbId());
            PdcpRxEntityBase *entity = lowerMux_->lookupRxEntity(id);
            ASSERT(entity != nullptr);

            EV << NOW << " DcMux::handleMessage - Received UL PDCP PDU from secondary node " << sourceNode
               << " for " << id << " - dispatching to RX entity" << endl;
            send(pkt, entity->gate("dcIn")->getPreviousGate());
        }
    }
    else if (incoming->isName("fromEntity")) {
        // Outgoing from PDCP entity: forward to DC manager
        send(msg, "dcManagerOut");
    }
    else {
        throw cRuntimeError("DcMux: unexpected message from gate %s", incoming->getFullName());
    }
}

PdcpTxEntityBase *DcMux::lookupBypassTxEntity(DrbKey id)
{
    auto it = bypassTxEntities_.find(id);
    return it != bypassTxEntities_.end() ? it->second : nullptr;
}

PdcpTxEntityBase *DcMux::createBypassTxEntity(DrbKey id)
{
    std::stringstream buf;
    buf << "bypass-tx-" << id.getNodeId() << "-" << id.getDrbId();
    auto *module = bypassTxEntityModuleType_->create(buf.str().c_str(), getParentModule());
    module->finalizeParameters();
    module->buildInside();

    // Wire DcMux output gate to entity input gate (DcMux dispatches incoming DL X2)
    int idx = gateSize("toBypassTxEntity");
    setGateSize("toBypassTxEntity", idx + 1);
    gate("toBypassTxEntity", idx)->connectTo(module->gate("in"));

    // Wire entity output gate to LowerMux input gate (entity sends to RLC via LowerMux)
    int fromIdx = lowerMux_->gateSize("fromTxEntity");
    lowerMux_->setGateSize("fromTxEntity", fromIdx + 1);
    module->gate("out")->connectTo(lowerMux_->gate("fromTxEntity", fromIdx));

    module->scheduleStart(simTime());
    module->callInitialize();
    PdcpTxEntityBase *txEnt = check_and_cast<PdcpTxEntityBase *>(module);
    bypassTxEntities_[id] = txEnt;

    EV << "DcMux::createBypassTxEntity - Added new BypassTxPdcpEntity for " << id << "\n";

    return txEnt;
}

void DcMux::deleteBypassTxEntities(MacNodeId nodeId)
{
    bool isEnb = (getNodeTypeById(nodeId_) == NODEB);

    if (isEnb) {
        for (auto it = bypassTxEntities_.begin(); it != bypassTxEntities_.end(); ) {
            auto& [id, txEntity] = *it;
            if (id.getNodeId() == nodeId) {
                txEntity->deleteModule();
                it = bypassTxEntities_.erase(it);
            }
            else {
                ++it;
            }
        }
    }
    else {
        for (auto& [id, txEntity] : bypassTxEntities_)
            txEntity->deleteModule();
        bypassTxEntities_.clear();
    }
}

} // namespace simu5g
