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

#include "stack/pdcp_rrc/LtePdcpRrc.h"

#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/transportlayer/tcp_common/TcpHeader.h>
#include <inet/transportlayer/udp/UdpHeader_m.h>

#include "stack/packetFlowManager/PacketFlowManagerBase.h"
#include "stack/pdcp_rrc/packet/LteRohcPdu_m.h"

namespace simu5g {

Define_Module(LtePdcpRrcUe);
Define_Module(LtePdcpRrcEnb);

using namespace omnetpp;
using namespace inet;

// We require a minimum length of 1 Byte for each header even in compressed state
// (transport, network and ROHC header, i.e. minimum is 3 Bytes)
#define MIN_COMPRESSED_HEADER_SIZE    B(3)

// statistics
simsignal_t LtePdcpRrcBase::receivedPacketFromUpperLayerSignal_ = registerSignal("receivedPacketFromUpperLayer");
simsignal_t LtePdcpRrcBase::receivedPacketFromLowerLayerSignal_ = registerSignal("receivedPacketFromLowerLayer");
simsignal_t LtePdcpRrcBase::sentPacketToUpperLayerSignal_ = registerSignal("sentPacketToUpperLayer");
simsignal_t LtePdcpRrcBase::sentPacketToLowerLayerSignal_ = registerSignal("sentPacketToLowerLayer");

LtePdcpRrcBase::~LtePdcpRrcBase()
{
}

bool LtePdcpRrcBase::isCompressionEnabled()
{
    return headerCompressedSize_ != LTE_PDCP_HEADER_COMPRESSION_DISABLED;
}

void LtePdcpRrcBase::headerCompress(Packet *pkt)
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

void LtePdcpRrcBase::headerDecompress(Packet *pkt)
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

void LtePdcpRrcBase::setTrafficInformation(cPacket *pkt, inet::Ptr<FlowControlInfo> lteInfo)
{
    if ((strcmp(pkt->getName(), "VoIP")) == 0) {
        lteInfo->setApplication(VOIP);
        lteInfo->setTraffic(CONVERSATIONAL);
        lteInfo->setRlcType(conversationalRlc_);
    }
    else if ((strcmp(pkt->getName(), "gaming")) == 0) {
        lteInfo->setApplication(GAMING);
        lteInfo->setTraffic(INTERACTIVE);
        lteInfo->setRlcType(interactiveRlc_);
    }
    else if ((strcmp(pkt->getName(), "VoDPacket") == 0)
             || (strcmp(pkt->getName(), "VoDFinishPacket") == 0))
    {
        lteInfo->setApplication(VOD);
        lteInfo->setTraffic(STREAMING);
        lteInfo->setRlcType(streamingRlc_);
    }
    else {
        lteInfo->setApplication(CBR);
        lteInfo->setTraffic(BACKGROUND);
        lteInfo->setRlcType(backgroundRlc_);
    }

    lteInfo->setDirection(getDirection());
}

/*
 * Upper Layer handlers
 */

void LtePdcpRrcBase::fromDataPort(cPacket *pktAux)
{
    emit(receivedPacketFromUpperLayerSignal_, pktAux);

    // Control Information
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();

    setTrafficInformation(pkt, lteInfo);

    MacNodeId destId = getDestId(lteInfo);

    // CID Request
    EV << "LteRrc : Received CID request for Traffic [ " << "Source: " << Ipv4Address(lteInfo->getSrcAddr())
       << " Destination: " << Ipv4Address(lteInfo->getDstAddr())
       << " ToS: " << lteInfo->getTypeOfService() << " ]\n";

    // TODO: Since IP addresses can change when we add and remove nodes, maybe node IDs should be used instead of them
    LogicalCid mylcid;
    if ((mylcid = ht_.find_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(), lteInfo->getTypeOfService())) == 0xFFFF) {
        // LCID not found
        mylcid = lcid_++;

        EV << "LteRrc : Connection not found, new CID created with LCID " << mylcid << "\n";

        ht_.create_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(), lteInfo->getTypeOfService(), mylcid);
    }

    // assign LCID
    lteInfo->setLcid(mylcid);
    lteInfo->setSourceId(nodeId_);
    lteInfo->setDestId(destId);

    // obtain CID
    MacCid cid = idToMacCid(destId, mylcid);

    EV << "LteRrc : Assigned Lcid: " << mylcid << "\n";
    EV << "LteRrc : Assigned Node ID: " << nodeId_ << "\n";

    // get the PDCP entity for this LCID and process the packet
    LteTxPdcpEntity *entity = getTxEntity(cid);
    entity->handlePacketFromUpperLayer(pkt);
}

void LtePdcpRrcBase::fromEutranRrcSap(cPacket *pkt)
{
    // TODO For now use LCID 1000 for Control Traffic coming from RRC
    FlowControlInfo *lteInfo = new FlowControlInfo();
    lteInfo->setSourceId(nodeId_);
    lteInfo->setLcid(1000);
    lteInfo->setRlcType(TM);
    pkt->setControlInfo(lteInfo);
    EV << "LteRrc : Sending packet " << pkt->getName() << " on port TM_Sap$o\n";
    send(pkt, tmSapOutGate_);
}

/*
 * Lower layer handlers
 */

void LtePdcpRrcBase::fromLowerLayer(cPacket *pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);
    emit(receivedPacketFromLowerLayerSignal_, pkt);

    auto lteInfo = pkt->getTag<FlowControlInfo>();

    MacCid cid = idToMacCid(lteInfo->getSourceId(), lteInfo->getLcid());   // TODO: check if you have to get master node id

    LteRxPdcpEntity *entity = getRxEntity(cid);
    entity->handlePacketFromLowerLayer(pkt);
}

void LtePdcpRrcBase::toDataPort(cPacket *pktAux)
{
    Enter_Method_Silent("LtePdcpRrcBase::toDataPort");

    auto pkt = check_and_cast<Packet *>(pktAux);
    take(pkt);

    EV << "LtePdcp : Sending packet " << pkt->getName() << " on port DataPort$o\n";

    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);

    EV << "LtePdcp : Sending packet " << pkt->getName() << " on port DataPort$o\n";

    // Send message
    send(pkt, dataPortOutGate_);
    emit(sentPacketToUpperLayerSignal_, pkt);
}

void LtePdcpRrcBase::toEutranRrcSap(cPacket *pkt)
{
    cPacket *upPkt = pkt->decapsulate();
    delete pkt;

    EV << "LteRrc : Sending packet " << upPkt->getName() << " on port EUTRAN_RRC_Sap$o\n";
    send(upPkt, eutranRrcSapOutGate_);
}

/*
 * Forwarding Handlers
 */

void LtePdcpRrcBase::sendToLowerLayer(Packet *pkt)
{
    auto lteInfo = pkt->getTag<FlowControlInfo>();

    cGate *gate;
    switch (lteInfo->getRlcType()) {
        case UM:
            gate = umSapOutGate_;
            break;
        case AM:
            gate = amSapOutGate_;
            break;
        case TM:
            gate = tmSapOutGate_;
            break;
        default:
            throw cRuntimeError("LtePdcpRrcBase::sendToLowerLayer(): invalid RlcType %d", lteInfo->getRlcType());
    }

    EV << "LtePdcp : Sending packet " << pkt->getName() << " on port " << gate->getFullName() << endl;

    /*
     * @author Alessandro Noferi
     *
     * Since the other methods, e.g. fromData, are overridden
     * in many classes, this method is the only one used by
     * all the classes (except the NRPdcpUe that has its
     * own sendToLowerLayer method).
     * So, the notification about the new PDCP to the pfm
     * is done here.
     *
     * packets sent in D2D mode are not considered
     */

    if (lteInfo->getDirection() != D2D_MULTI && lteInfo->getDirection() != D2D) {
        if (packetFlowManager_ != nullptr)
            packetFlowManager_->insertPdcpSdu(pkt);
    }

    // Send message
    send(pkt, gate);
    emit(sentPacketToLowerLayerSignal_, pkt);
}

void LtePdcpRrcBase::sendToUpperLayer(cPacket *pkt)
{
    EV << "LtePdcp : Sending packet " << pkt->getName() << " on port DataPort$o" << endl;

    // Send message
    send(pkt, dataPortOutGate_);
    emit(sentPacketToUpperLayerSignal_, pkt);
}

/*
 * Main functions
 */

void LtePdcpRrcBase::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        dataPortInGate_ = gate("DataPort$i");
        dataPortOutGate_ = gate("DataPort$o");
        eutranRrcSapInGate_ = gate("EUTRAN_RRC_Sap$i");
        eutranRrcSapOutGate_ = gate("EUTRAN_RRC_Sap$o");
        tmSapInGate_ = gate("TM_Sap$i", 0);
        tmSapOutGate_ = gate("TM_Sap$o", 0);
        umSapInGate_ = gate("UM_Sap$i", 0);
        umSapOutGate_ = gate("UM_Sap$o", 0);
        amSapInGate_ = gate("AM_Sap$i", 0);
        amSapOutGate_ = gate("AM_Sap$o", 0);

        binder_.reference(this, "binderModule", true);
        headerCompressedSize_ = B(par("headerCompressedSize"));
        if (headerCompressedSize_ != LTE_PDCP_HEADER_COMPRESSION_DISABLED &&
            headerCompressedSize_ < MIN_COMPRESSED_HEADER_SIZE) {
            throw cRuntimeError("Size of compressed header must not be less than %" PRId64 "B.", MIN_COMPRESSED_HEADER_SIZE.get());
        }

        nodeId_ = MacNodeId(getContainingNode(this)->par("macNodeId").intValue());

        packetFlowManager_.reference(this, "packetFlowManagerModule", false);
        NRpacketFlowManager_.reference(this, "nrPacketFlowManagerModule", false);

        if (packetFlowManager_) {
            EV << "LtePdcpRrcBase::initialize - PacketFlowManager present" << endl;
        }
        if (NRpacketFlowManager_) {
            EV << "LtePdcpRrcBase::initialize - NRpacketFlowManager present" << endl;
        }

        conversationalRlc_ = aToRlcType(par("conversationalRlc"));
        interactiveRlc_ = aToRlcType(par("interactiveRlc"));
        streamingRlc_ = aToRlcType(par("streamingRlc"));
        backgroundRlc_ = aToRlcType(par("backgroundRlc"));

        // TODO WATCH_MAP(gatemap_);
        WATCH(headerCompressedSize_);
        WATCH(nodeId_);
        WATCH(lcid_);
    }
}

void LtePdcpRrcBase::handleMessage(cMessage *msg)
{
    cPacket *pkt = check_and_cast<cPacket *>(msg);
    EV << "LtePdcp : Received packet " << pkt->getName() << " from port "
       << pkt->getArrivalGate()->getName() << endl;

    cGate *incoming = pkt->getArrivalGate();
    if (incoming == dataPortInGate_) {
        fromDataPort(pkt);
    }
    else if (incoming == eutranRrcSapInGate_) {
        fromEutranRrcSap(pkt);
    }
    else if (incoming == tmSapInGate_) {
        toEutranRrcSap(pkt);
    }
    else {
        fromLowerLayer(pkt);
    }
}

LteTxPdcpEntity *LtePdcpRrcBase::getTxEntity(MacCid cid)
{
    // Find entity for this LCID
    PdcpTxEntities::iterator it = txEntities_.find(cid);
    if (it == txEntities_.end()) {
        // Not found: create
        std::stringstream buf;
        // FIXME HERE

        buf << "LteTxPdcpEntity cid: " << cid;
        cModuleType *moduleType = cModuleType::get("simu5g.stack.pdcp_rrc.LteTxPdcpEntity");
        LteTxPdcpEntity *txEnt = check_and_cast<LteTxPdcpEntity *>(moduleType->createScheduleInit(buf.str().c_str(), this));
        txEntities_[cid] = txEnt;    // Add to entities map

        EV << "LtePdcpRrcBase::getTxEntity - Added new TxPdcpEntity for Cid: " << cid << "\n";

        return txEnt;
    }
    else {
        // Found
        EV << "LtePdcpRrcBase::getTxEntity - Using old TxPdcpEntity for Cid: " << cid << "\n";

        return it->second;
    }
}

LteRxPdcpEntity *LtePdcpRrcBase::getRxEntity(MacCid cid)
{
    // Find entity for this CID
    PdcpRxEntities::iterator it = rxEntities_.find(cid);
    if (it == rxEntities_.end()) {
        // Not found: create

        std::stringstream buf;
        buf << "LteRxPdcpEntity Cid: " << cid;
        cModuleType *moduleType = cModuleType::get("simu5g.stack.pdcp_rrc.LteRxPdcpEntity");
        LteRxPdcpEntity *rxEnt = check_and_cast<LteRxPdcpEntity *>(moduleType->createScheduleInit(buf.str().c_str(), this));
        rxEntities_[cid] = rxEnt;    // Add to entities map

        EV << "LtePdcpRrcBase::getRxEntity - Added new RxPdcpEntity for Cid: " << cid << "\n";

        return rxEnt;
    }
    else {
        // Found
        EV << "LtePdcpRrcBase::getRxEntity - Using old RxPdcpEntity for Cid: " << cid << "\n";

        return it->second;
    }
}

void LtePdcpRrcBase::finish()
{
    // TODO make-finish
}

void LtePdcpRrcEnb::initialize(int stage)
{
    LtePdcpRrcBase::initialize(stage);
}

void LtePdcpRrcEnb::deleteEntities(MacNodeId nodeId)
{
    Enter_Method_Silent();

    // delete connections related to the given UE
    for (auto tit = txEntities_.begin(); tit != txEntities_.end(); ) {
        if (MacCidToNodeId(tit->first) == nodeId) {
            tit->second->deleteModule();
            tit = txEntities_.erase(tit);
        }
        else {
            ++tit;
        }
    }

    for (auto rit = rxEntities_.begin(); rit != rxEntities_.end(); ) {
        if (MacCidToNodeId(rit->first) == nodeId) {
            rit->second->deleteModule();
            rit = rxEntities_.erase(rit);
        }
        else {
            ++rit;
        }
    }
}

void LtePdcpRrcUe::deleteEntities(MacNodeId nodeId)
{
    // delete all connections TODO: check this (for NR dual connectivity)
    for (auto& [txId, txEntity] : txEntities_) {
        txEntity->deleteModule();  // Delete Entity
    }
    txEntities_.clear(); // Clear all entities after deletion

    for (auto& [rxId, rxEntity] : rxEntities_) {
        rxEntity->deleteModule();  // Delete Entity
    }
    rxEntities_.clear(); // Clear all entities after deletion
}

void LtePdcpRrcUe::initialize(int stage)
{
    LtePdcpRrcBase::initialize(stage);
    if (stage == inet::INITSTAGE_NETWORK_LAYER) {
        // refresh value, the parameter may have changed between INITSTAGE_LOCAL and INITSTAGE_NETWORK_LAYER
        nodeId_ = MacNodeId(getContainingNode(this)->par("macNodeId").intValue());
    }
}

} //namespace

