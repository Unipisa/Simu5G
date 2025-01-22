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

#include "stack/pdcp_rrc/LteRxPdcpEntity.h"
#include "stack/packetFlowManager/PacketFlowManagerBase.h"

namespace simu5g {

Define_Module(LteRxPdcpEntity);


void LteRxPdcpEntity::initialize()
{
    pdcp_ = check_and_cast<LtePdcpRrcBase *>(getParentModule());
}

void LteRxPdcpEntity::handlePacketFromLowerLayer(Packet *pkt)
{
    EV << NOW << " LteRxPdcpEntity::handlePacketFromLowerLayer - LCID[" << lcid_ << "] - processing packet from RLC layer" << endl;

    // pop PDCP header
    pkt->popAtFront<LtePdcpPdu>();

    // TODO NRRxEntity could delete this packet in handlePdcpSdu()...
    auto lteInfo = pkt->getTag<FlowControlInfo>();
    if (lteInfo->getDirection() != D2D_MULTI && lteInfo->getDirection() != D2D) {
        PacketFlowManagerBase *flowManager = pdcp_->getPacketFlowManager();
        if (flowManager != nullptr)
            flowManager->receivedPdcpSdu(pkt);
    }

    // perform PDCP operations
    pdcp_->headerDecompress(pkt); // Decompress packet header

    // handle PDCP SDU
    handlePdcpSdu(pkt);
}

void LteRxPdcpEntity::handlePdcpSdu(Packet *pkt)
{
    Enter_Method("LteRxPdcpEntity::handlePdcpSdu");

    auto controlInfo = pkt->getTag<FlowControlInfo>();

    EV << NOW << " LteRxPdcpEntity::handlePdcpSdu - processing PDCP SDU with SN[" << controlInfo->getSequenceNumber() << "]" << endl;

    // deliver to IP layer
    pdcp_->toDataPort(pkt);
}


} //namespace

