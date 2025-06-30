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

#include "stack/pdcp_rrc/LteTxPdcpEntity.h"

#include <inet/common/ProtocolTag_m.h>

namespace simu5g {

Define_Module(LteTxPdcpEntity);

void LteTxPdcpEntity::initialize()
{
    pdcp_ = check_and_cast<LtePdcpRrcBase *>(getParentModule());
}

void LteTxPdcpEntity::handlePacketFromUpperLayer(Packet *pkt)
{
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    EV << NOW << " LteTxPdcpEntity::handlePacketFromUpperLayer - LCID[" << lteInfo->getLcid() << "] - processing packet from IP layer" << endl;

    // perform PDCP operations
    pdcp_->headerCompress(pkt); // header compression

    // Write information into the ControlInfo object
    lteInfo->setSequenceNumber(sno_++); // set sequence number for this PDCP SDU.

    // set source and destination IDs
    setIds(lteInfo);

    // PDCP Packet creation
    auto pdcpPkt = makeShared<LtePdcpPdu>();

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
            throw cRuntimeError("LtePdcpRrcBase::fromDataport(): invalid RlcType %d", lteInfo->getRlcType());
    }
    pdcpPkt->setChunkLength(B(headerLength));
    pkt->trim();
    pkt->insertAtFront(pdcpPkt);
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&LteProtocol::pdcp);

    EV << "LtePdcp : Preparing to send "
       << lteTrafficClassToA((LteTrafficClass)lteInfo->getTraffic())
       << " traffic\n";
    EV << "LtePdcp : Packet size " << pkt->getByteLength() << " Bytes\n";

    EV << NOW << " LteTxPdcpEntity::handlePacketFromUpperLayer - LCID[" << lteInfo->getLcid() << "] - sending PDCP PDU to the RLC layer" << endl;
    deliverPdcpPdu(pkt);
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

