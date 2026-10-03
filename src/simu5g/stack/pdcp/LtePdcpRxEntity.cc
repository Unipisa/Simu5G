//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/pdcp/LtePdcpRxEntity.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/transportlayer/tcp_common/TcpHeader.h>
#include <inet/transportlayer/udp/UdpHeader_m.h>
#include "simu5g/stack/pdcp/packet/LtePdcpPdu_m.h"
#include "simu5g/stack/sdap/packet/NrSdapHeader_m.h"
#include "simu5g/stack/pdcp/packet/RohcHeader.h"

namespace simu5g {

Define_Module(LtePdcpRxEntity);

simsignal_t LtePdcpRxEntity::receivedPacketFromLowerLayerSignal_ = registerSignal("receivedPacketFromLowerLayer");
simsignal_t LtePdcpRxEntity::pdcpSduReceivedSignal_ = registerSignal("pdcpSduReceived");
simsignal_t LtePdcpRxEntity::sentPacketToUpperLayerSignal_ = registerSignal("sentPacketToUpperLayer");


void LtePdcpRxEntity::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        binder_.reference(this, "binderModule", true);
        nodeId_ = MacNodeId(getContainingNode(this)->par("macNodeId").intValue());

        headerCompressionEnabled_ = !opp_isblank(par("rohcProfiles").stringValue());
    }
}

void LtePdcpRxEntity::handlePacketFromLowerLayer(Packet *pkt)
{
    emit(receivedPacketFromLowerLayerSignal_, pkt);
    EV << NOW << " LtePdcpRxEntity::handlePacketFromLowerLayer - DRB ID[" << drbId_ << "] - processing packet from RLC layer" << endl;

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

void LtePdcpRxEntity::handlePdcpSdu(Packet *pkt, unsigned int sequenceNumber)
{
    Enter_Method("LtePdcpRxEntity::handlePdcpSdu");

    EV << NOW << " LtePdcpRxEntity::handlePdcpSdu - processing PDCP SDU with SN[" << sequenceNumber << "]" << endl;

    // deliver to IP layer
    deliverSduToUpperLayer(pkt);
}

void LtePdcpRxEntity::deliverSduToUpperLayer(Packet *pkt)
{
    emit(sentPacketToUpperLayerSignal_, pkt);
    send(pkt, "out");
}

void LtePdcpRxEntity::decompressHeader(Packet *pkt)
{
    if (isCompressionEnabled()) {
        pkt->trim();

        // Check if there's an SDAP header on top
        inet::Ptr<inet::Chunk> sdapHeader = nullptr;
        if (dynamicPtrCast<const NrSdapHeader>(pkt->peekAtFront())) {
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

