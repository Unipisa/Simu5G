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

#include "simu5g/stack/pdcp/LteTxPdcpEntity.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/stack/pdcp/packet/LtePdcpPdu_m.h"
#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/transportlayer/tcp_common/TcpHeader.h>
#include <inet/transportlayer/udp/UdpHeader_m.h>
#include "simu5g/stack/packetFlowManager/PacketFlowManagerBase.h"
#include "simu5g/stack/pdcp/packet/LteRohcPdu_m.h"
#include <inet/common/ProtocolTag_m.h>
#include "simu5g/stack/sdap/packet/NrSdapHeader_m.h"
#include "simu5g/stack/pdcp/packet/RohcHeader.h"
#include "simu5g/stack/pdcp/packet/LtePdcpPdu_m.h"

// We require a minimum length of 1 Byte for each header even in compressed state
// (transport, network and ROHC header, i.e. minimum is 3 Bytes)
#define MIN_COMPRESSED_HEADER_SIZE    B(3)


namespace simu5g {

Define_Module(LteTxPdcpEntity);

void LteTxPdcpEntity::initialize()
{
    pdcp_ = check_and_cast<LtePdcpBase *>(getParentModule());

    headerCompressedSize_ = B(pdcp_->par("headerCompressedSize"));
    if (headerCompressedSize_ != LTE_PDCP_HEADER_COMPRESSION_DISABLED && headerCompressedSize_ < MIN_COMPRESSED_HEADER_SIZE)
        throw cRuntimeError("Size of compressed header must not be less than %" PRId64 "B.", MIN_COMPRESSED_HEADER_SIZE.get());
}

void LteTxPdcpEntity::handlePacketFromUpperLayer(Packet *pkt)
{
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    EV << NOW << " LteTxPdcpEntity::handlePacketFromUpperLayer - processing packet " << pkt->getName() << " from IP layer" << endl;

    // perform PDCP operations
    compressHeader(pkt); // header compression

    // Write information into the ControlInfo object
    lteInfo->setSequenceNumber(sno_++); // set sequence number for this PDCP SDU.

    // PDCP Packet creation
    auto pdcpHeader = makeShared<LtePdcpHeader>();

    unsigned int headerLength;
    switch (lteInfo->getRlcType()) {
        case UM:
            headerLength = PDCP_HEADER_UM;
            break;
        case AM:
            headerLength = PDCP_HEADER_AM;
            break;
        case TM:
            headerLength = 0;
            break;
        default:
            throw cRuntimeError("LtePdcpBase::handlePacketFromUpperLayer(): invalid RlcType %d", lteInfo->getRlcType());
    }
    pdcpHeader->setChunkLength(B(headerLength));
    pkt->trim();
    pkt->insertAtFront(pdcpHeader);
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&LteProtocol::pdcp);

    EV << "LteTxPdcpEntity::handlePacketFromUpperLayer: Packet trafficClass="
       << lteTrafficClassToA((LteTrafficClass)lteInfo->getTraffic())
       << " size=" << pkt->getByteLength() << "B\n";

    EV << NOW << " LteTxPdcpEntity::handlePacketFromUpperLayer: sending PDCP PDU to the RLC layer" << endl;
    deliverPdcpPdu(pkt);
}

void LteTxPdcpEntity::compressHeader(Packet *pkt)
{
    if (isCompressionEnabled()) {
        // Check PacketProtocolTag to determine packet type
        auto protocolTag = pkt->findTag<PacketProtocolTag>();
        inet::Ptr<inet::Chunk> sdapHeader = nullptr;

        // If packet has SDAP protocol tag, remove SDAP header first
        if (protocolTag && protocolTag->getProtocol() == &LteProtocol::sdap) {
            sdapHeader = pkt->removeAtFront<NrSdapHeader>();
            EV << "LtePdcp : Removed SDAP header before compression\n";
        }

        // Extract IP and transport headers to be compressed
        auto ipHeader = pkt->removeAtFront<Ipv4Header>();
        int transportProtocol = ipHeader->getProtocolId();

        inet::Ptr<inet::Chunk> transportHeader;
        if (transportProtocol == IP_PROT_TCP) {
            transportHeader = pkt->removeAtFront<tcp::TcpHeader>();
        }
        else if (transportProtocol == IP_PROT_UDP) {
            transportHeader = pkt->removeAtFront<UdpHeader>();
        }
        else {
            transportHeader = nullptr;  // cannot compress
        }

        // Create a sequence chunk containing the original headers
        auto originalHeaders = inet::makeShared<inet::SequenceChunk>();

        // Make headers immutable before inserting into SequenceChunk
        ipHeader->markImmutable();
        originalHeaders->insertAtBack(ipHeader);
        if (transportHeader) {
            transportHeader->markImmutable();
            originalHeaders->insertAtBack(transportHeader);
        }

        // Make originalHeaders immutable before passing to RohcHeader
        originalHeaders->markImmutable();

        // Create ROHC header with original headers and compressed size
        auto rohcHeader = makeShared<RohcHeader>(originalHeaders, headerCompressedSize_);
        rohcHeader->markImmutable();
        pkt->insertAtFront(rohcHeader);

        // If we had an SDAP header, add it back on top of the ROHC header
        if (sdapHeader) {
            sdapHeader->markImmutable();
            pkt->insertAtFront(sdapHeader);
            EV << "LtePdcp : Added SDAP header back on top of ROHC header\n";
        }

        EV << "LtePdcp : Header compression performed\n";
    }
}


void LteTxPdcpEntity::deliverPdcpPdu(Packet *pdcpPkt)
{
    pdcp_->sendToLowerLayer(pdcpPkt);
}


} //namespace

