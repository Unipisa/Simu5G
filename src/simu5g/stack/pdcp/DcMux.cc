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
#include "simu5g/stack/pdcp/PdcpRxEntityBase.h"
#include "simu5g/stack/rrc/BearerManagement.h"
#include "simu5g/common/binder/Binder.h"
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

        auto *rrc = getParentModule()->getSubmodule("rrc");
        ASSERT(rrc != nullptr);
        bearerManagement_ = check_and_cast<BearerManagement *>(rrc->getSubmodule("bearerManagement"));

        dcManagerInGate_ = gate("dcManagerIn");
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
            // The sourceId may be the NR UE ID; translate to LTE UE ID for RX entity lookup
            MacNodeId sourceId = lteInfo->getSourceId();
            if (isNrUe(sourceId))
                sourceId = binder_->getUeNodeId(sourceId, false);
            DrbKey id = DrbKey(sourceId, lteInfo->getDrbId());
            PdcpRxEntityBase *entity = bearerManagement_->lookupPdcpRxEntity(id);
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

void DcMux::registerBypassTxEntity(DrbKey id, PdcpTxEntityBase *txEnt)
{
    if (bypassTxEntities_.find(id) != bypassTxEntities_.end())
        throw cRuntimeError("PDCP bypass TX entity for %s already exists", id.str().c_str());
    bypassTxEntities_[id] = txEnt;
    EV << "DcMux::registerBypassTxEntity - Registered BypassTxPdcpEntity for " << id << "\n";
}

void DcMux::unregisterBypassTxEntity(DrbKey id)
{
    bypassTxEntities_.erase(id);
}

} // namespace simu5g
