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
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include <inet/common/ProtocolTag_m.h>
#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/transportlayer/tcp_common/TcpHeader.h>
#include <inet/transportlayer/udp/UdpHeader_m.h>
#include "simu5g/stack/pdcp/packet/LteRohcPdu_m.h"
#include "simu5g/stack/pdcp/packet/LtePdcpPdu_m.h"
#include "simu5g/stack/sdap/packet/NrSdapHeader_m.h"
#include "simu5g/stack/pdcp/packet/RohcHeader.h"

namespace simu5g {

Define_Module(LteRxPdcpEntity);

simsignal_t LteRxPdcpEntity::receivedPacketFromLowerLayerSignal_ = registerSignal("receivedPacketFromLowerLayer");
simsignal_t LteRxPdcpEntity::pdcpSduReceivedSignal_ = registerSignal("pdcpSduReceived");
simsignal_t LteRxPdcpEntity::sentPacketToUpperLayerSignal_ = registerSignal("sentPacketToUpperLayer");


void LteRxPdcpEntity::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        binder_.reference(this, "binderModule", true);
        nodeId_ = MacNodeId(getContainingNode(this)->par("macNodeId").intValue());

        headerCompressionEnabled_ = par("headerCompressedSize").intValue() > 0;
    }
}

void LteRxPdcpEntity::handlePacketFromLowerLayer(Packet *pkt)
{
    emit(receivedPacketFromLowerLayerSignal_, pkt);
    EV << NOW << " LteRxPdcpEntity::handlePacketFromLowerLayer - DRB ID[" << drbId_ << "] - processing packet from RLC layer" << endl;

    // Extract sequence number from PDCP header before popping it
    auto pdcpHeader = pkt->peekAtFront<LtePdcpHeader>();
    unsigned int sequenceNumber = pdcpHeader->getSequenceNumber();

    // TODO NRRxEntity could delete this packet in handlePdcpSdu()...
    auto lteInfo = pkt->getTag<FlowControlInfo>();
    if (hasListeners(pdcpSduReceivedSignal_) && lteInfo->getDirection() != D2D_MULTI && lteInfo->getDirection() != D2D) {
        emit(pdcpSduReceivedSignal_, pkt);
    }

    // pop PDCP header
    pkt->popAtFront<LtePdcpHeader>();

    // perform PDCP operations
    decompressHeader(pkt); // Decompress packet header

    // handle PDCP SDU
    handlePdcpSdu(pkt, sequenceNumber);
}

void LteRxPdcpEntity::handlePdcpSdu(Packet *pkt, unsigned int sequenceNumber)
{
    Enter_Method("LteRxPdcpEntity::handlePdcpSdu");

    EV << NOW << " LteRxPdcpEntity::handlePdcpSdu - processing PDCP SDU with SN[" << sequenceNumber << "]" << endl;

    // deliver to IP layer
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);
    deliverSduToUpperLayer(pkt);
}

void LteRxPdcpEntity::deliverSduToUpperLayer(Packet *pkt)
{
    emit(sentPacketToUpperLayerSignal_, pkt);
    send(pkt, "out");
}

void LteRxPdcpEntity::decompressHeader(Packet *pkt)
{
    if (isCompressionEnabled()) {
        pkt->trim();

        // Check if there's an SDAP header on top
        inet::Ptr<inet::Chunk> sdapHeader = nullptr;
        if (pkt->peekAtFront<NrSdapHeader>()) {
            sdapHeader = pkt->removeAtFront<NrSdapHeader>();
            EV << "LtePdcp : Removed SDAP header before decompression\n";
        }

        auto rohcHeader = pkt->removeAtFront<RohcHeader>();

        // Get the original headers from the ROHC header
        auto originalHeaders = rohcHeader->getChunk();

        // Insert the original headers back into the packet
        pkt->insertAtFront(originalHeaders);

        // If we had an SDAP header, add it back on top
        if (sdapHeader) {
            pkt->insertAtFront(sdapHeader);
            EV << "LtePdcp : Added SDAP header back on top after decompression\n";
        }

        EV << "LtePdcp : Header decompression performed\n";
    }

}


} //namespace

