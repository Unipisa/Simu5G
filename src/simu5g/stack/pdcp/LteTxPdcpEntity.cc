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
    EV << NOW << " LteTxPdcpEntity::handlePacketFromUpperLayer - LCID[" << lteInfo->getLcid() << "] - processing packet from IP layer" << endl;

    // perform PDCP operations
    compressHeader(pkt); // header compression

    // Write information into the ControlInfo object
    lteInfo->setSequenceNumber(sno_++); // set sequence number for this PDCP SDU.

    // set source and destination IDs
    setIds(lteInfo);

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
            throw cRuntimeError("LtePdcpBase::fromDataport(): invalid RlcType %d", lteInfo->getRlcType());
    }
    pdcpHeader->setChunkLength(B(headerLength));
    pkt->trim();
    pkt->insertAtFront(pdcpHeader);
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&LteProtocol::pdcp);

    EV << "LtePdcp : Preparing to send "
       << lteTrafficClassToA((LteTrafficClass)lteInfo->getTraffic())
       << " traffic\n";
    EV << "LtePdcp : Packet size " << pkt->getByteLength() << " Bytes\n";

    EV << NOW << " LteTxPdcpEntity::handlePacketFromUpperLayer - LCID[" << lteInfo->getLcid() << "] - sending PDCP PDU to the RLC layer" << endl;
    deliverPdcpPdu(pkt);
}

void LteTxPdcpEntity::compressHeader(Packet *pkt)
{
    if (isCompressionEnabled()) {
        auto ipHeader = pkt->removeAtFront<Ipv4Header>();

        int transportProtocol = ipHeader->getProtocolId();
        B transportHeaderCompressedSize = B(0);

        auto rohcHeader = makeShared<LteRohcPdu>();
        rohcHeader->setOrigSizeIpHeader(ipHeader->getHeaderLength());

        if (IP_PROT_TCP == transportProtocol) {
            auto tcpHeader = pkt->removeAtFront<tcp::TcpHeader>();
            rohcHeader->setOrigSizeTransportHeader(tcpHeader->getHeaderLength());
            tcpHeader->setChunkLength(B(1));
            transportHeaderCompressedSize = B(1);
            pkt->insertAtFront(tcpHeader);
        }
        else if (IP_PROT_UDP == transportProtocol) {
            auto udpHeader = pkt->removeAtFront<UdpHeader>();
            rohcHeader->setOrigSizeTransportHeader(inet::UDP_HEADER_LENGTH);
            udpHeader->setChunkLength(B(1));
            transportHeaderCompressedSize = B(1);
            pkt->insertAtFront(udpHeader);
        }
        else {
            EV_WARN << "LtePdcp : unknown transport header - cannot perform transport header compression";
            rohcHeader->setOrigSizeTransportHeader(B(0));
        }

        ipHeader->setChunkLength(B(1));
        pkt->insertAtFront(ipHeader);

        rohcHeader->setChunkLength(headerCompressedSize_ - transportHeaderCompressedSize - B(1));
        pkt->insertAtFront(rohcHeader);

        EV << "LtePdcp : Header compression performed\n";
    }
}


void LteTxPdcpEntity::deliverPdcpPdu(Packet *pdcpPkt)
{
    pdcp_->sendToLowerLayer(pdcpPkt);
}

void LteTxPdcpEntity::setIds(inet::Ptr<FlowControlInfo> lteInfo)
{
    lteInfo->setSourceId(pdcp_->getNodeId());   // TODO CHANGE HERE!!! Must be the NR node ID if this is an NR connection

    if (lteInfo->getMulticastGroupId() > 0)                                               // destId is meaningless for multicast D2D (we use the id of the source for statistic purposes at lower levels)
        lteInfo->setDestId(pdcp_->getNodeId());
    else
        lteInfo->setDestId(pdcp_->getDestId(lteInfo));
}


} //namespace

