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

#include "simu5g/stack/pdcp/LteRxPdcpEntity.h"
#include "simu5g/stack/packetFlowManager/PacketFlowManagerBase.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/transportlayer/tcp_common/TcpHeader.h>
#include <inet/transportlayer/udp/UdpHeader_m.h>
#include "simu5g/stack/packetFlowManager/PacketFlowManagerBase.h"
#include "simu5g/stack/pdcp/packet/LteRohcPdu_m.h"
#include "simu5g/stack/pdcp/packet/LtePdcpPdu_m.h"

namespace simu5g {

Define_Module(LteRxPdcpEntity);


void LteRxPdcpEntity::initialize()
{
    pdcp_ = check_and_cast<LtePdcpBase *>(getParentModule());

    headerCompressionEnabled_ = pdcp_->par("headerCompressedSize").intValue() > 0;  // TODO a.k.a. LTE_PDCP_HEADER_COMPRESSION_DISABLED
}

void LteRxPdcpEntity::handlePacketFromLowerLayer(Packet *pkt)
{
    take(pkt);
    EV << NOW << " LteRxPdcpEntity::handlePacketFromLowerLayer - LCID[" << lcid_ << "] - processing packet from RLC layer" << endl;

    // pop PDCP header
    pkt->popAtFront<LtePdcpHeader>();

    // TODO NRRxEntity could delete this packet in handlePdcpSdu()...
    auto lteInfo = pkt->getTag<FlowControlInfo>();
    if (lteInfo->getDirection() != D2D_MULTI && lteInfo->getDirection() != D2D) {
        PacketFlowManagerBase *flowManager = pdcp_->getPacketFlowManager();
        if (flowManager != nullptr)
            flowManager->receivedPdcpSdu(pkt);
    }

    // perform PDCP operations
    decompressHeader(pkt); // Decompress packet header

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

void LteRxPdcpEntity::decompressHeader(Packet *pkt)
{
    if (isCompressionEnabled()) {
        pkt->trim();
        auto rohcHeader = pkt->removeAtFront<LteRohcPdu>();
        auto ipHeader = pkt->removeAtFront<Ipv4Header>();
        int transportProtocol = ipHeader->getProtocolId();

        if (IP_PROT_TCP == transportProtocol) {
            auto tcpHeader = pkt->removeAtFront<tcp::TcpHeader>();
            tcpHeader->setChunkLength(rohcHeader->getOrigSizeTransportHeader());
            pkt->insertAtFront(tcpHeader);
        }
        else if (IP_PROT_UDP == transportProtocol) {
            auto udpHeader = pkt->removeAtFront<UdpHeader>();
            udpHeader->setChunkLength(rohcHeader->getOrigSizeTransportHeader());
            pkt->insertAtFront(udpHeader);
        }
        else {
            EV_WARN << "LtePdcp : unknown transport header - cannot perform transport header decompression";
        }

        ipHeader->setChunkLength(rohcHeader->getOrigSizeIpHeader());
        pkt->insertAtFront(ipHeader);

        EV << "LtePdcp : Header decompression performed\n";
    }
}


} //namespace

