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

#include "simu5g/stack/pdcp/BypassTxPdcpEntity.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

Define_Module(BypassTxPdcpEntity);

simsignal_t BypassTxPdcpEntity::sentPacketToLowerLayerSignal_ = registerSignal("sentPacketToLowerLayer");
simsignal_t BypassTxPdcpEntity::pdcpSduSentSignal_ = registerSignal("pdcpSduSent");

void BypassTxPdcpEntity::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        pdcp_ = dynamic_cast<IPdcpGateway *>(getParentModule()->getSubmodule("mux"));
        ASSERT(pdcp_ != nullptr);
    }
}

void BypassTxPdcpEntity::handlePacketFromUpperLayer(inet::Packet *pkt)
{
    // DC bypass: packet is already a PDCP PDU from the master node.
    // Forward directly to RLC without any PDCP processing.
    EV << NOW << " BypassTxPdcpEntity::handlePacketFromUpperLayer - forwarding packet " << pkt->getName() << " to RLC (DC bypass)" << endl;

    auto lteInfo = pkt->getTag<FlowControlInfo>();
    if (hasListeners(pdcpSduSentSignal_) && lteInfo->getDirection() != D2D_MULTI && lteInfo->getDirection() != D2D) {
        emit(pdcpSduSentSignal_, pkt);
    }
    emit(sentPacketToLowerLayerSignal_, pkt);

    pdcp_->sendToRlc(pkt);
}

} // namespace simu5g
