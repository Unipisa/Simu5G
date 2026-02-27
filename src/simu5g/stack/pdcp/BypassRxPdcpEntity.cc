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

#include "simu5g/stack/pdcp/BypassRxPdcpEntity.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

Define_Module(BypassRxPdcpEntity);

void BypassRxPdcpEntity::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        pdcp_ = dynamic_cast<IPdcpGateway *>(getParentModule()->getSubmodule("mux"));
        ASSERT(pdcp_ != nullptr);
        binder_.reference(this, "binderModule", true);
        nodeId_ = MacNodeId(inet::getContainingNode(this)->par("macNodeId").intValue());
    }
}

void BypassRxPdcpEntity::handlePacketFromLowerLayer(inet::Packet *pkt)
{
    take(pkt);

    // DC bypass: forward PDCP PDU to the master node via X2
    // without any PDCP processing (no header removal, no decompression).
    pkt->trim();

    MacNodeId masterId = binder_->getMasterNodeOrSelf(nodeId_);
    EV << NOW << " BypassRxPdcpEntity::handlePacketFromLowerLayer - forwarding packet " << pkt->getName()
       << " to master node " << masterId << " via X2 (DC bypass)" << endl;

    // Translate sourceId from NR UE ID to LTE UE ID so the master's
    // RX entity (keyed by LTE UE ID) can find it without normalization
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    MacNodeId lteSourceId = binder_->getUeNodeId(lteInfo->getSourceId(), false);
    ASSERT(lteSourceId != NODEID_NONE);
    lteInfo->setSourceId(lteSourceId);

    auto tag = pkt->addTagIfAbsent<X2TargetReq>();
    tag->setTargetNode(masterId);

    pdcp_->sendToX2(pkt);
}

} // namespace simu5g
