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

#include "stack/pdcp_rrc/layer/LtePdcpRrc.h"
#include "stack/pdcp_rrc/packet/LteRohcPdu_m.h"

#include "inet/networklayer/common/L3Tools.h"
#include "inet/transportlayer/common/L4Tools.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/transportlayer/tcp_common/TcpHeader.h"
#include "inet/transportlayer/udp/UdpHeader_m.h"

Define_Module(LtePdcpRrcUe);
Define_Module(LtePdcpRrcEnb);

using namespace omnetpp;
using namespace inet;

// We require a minimum length of 1 Byte for each header even in compressed state
// (transport, network and ROHC header, i.e. minimum is 3 Bytes)
#define MIN_COMPRESSED_HEADER_SIZE B(3)

LtePdcpRrcBase::LtePdcpRrcBase()
{
    ht_ = new ConnectionsTable();
    lcid_ = 1;
}

LtePdcpRrcBase::~LtePdcpRrcBase()
{
    delete ht_;

    PdcpTxEntities::iterator t_it = txEntities_.begin();
    for (; t_it != txEntities_.end(); ++t_it)
        (t_it->second)->deleteModule();
    txEntities_.clear();

    PdcpRxEntities::iterator r_it = rxEntities_.begin();
    for (; r_it != rxEntities_.end(); ++r_it)
        (r_it->second)->deleteModule();
    rxEntities_.clear();
}

bool LtePdcpRrcBase::isCompressionEnabled()
{
    return (headerCompressedSize_ != LTE_PDCP_HEADER_COMPRESSION_DISABLED);
}

void LtePdcpRrcBase::headerCompress(Packet* pkt)
{
    if (isCompressionEnabled())
    {
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
        } else {
            EV_WARN << "LtePdcp : unknown transport header - cannot perform transport header compression";
            rohcHeader->setOrigSizeTransportHeader(B(0));
        }

        ipHeader->setChunkLength(B(1));
        pkt->insertAtFront(ipHeader);

        rohcHeader->setChunkLength(headerCompressedSize_-transportHeaderCompressedSize-B(1));
        pkt->insertAtFront(rohcHeader);

        EV << "LtePdcp : Header compression performed\n";
    }
}

void LtePdcpRrcBase::headerDecompress(Packet* pkt)
{
    if (isCompressionEnabled())
    {
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
        } else {
            EV_WARN << "LtePdcp : unknown transport header - cannot perform transport header decompression";
        }

        ipHeader->setChunkLength(rohcHeader->getOrigSizeIpHeader());
        pkt->insertAtFront(ipHeader);

        EV << "LtePdcp : Header decompression performed\n";
    }
}

void LtePdcpRrcBase::setTrafficInformation(cPacket* pkt,
    FlowControlInfo* lteInfo)
{
    if ((strcmp(pkt->getName(), "VoIP")) == 0)
    {
        lteInfo->setApplication(VOIP);
        lteInfo->setTraffic(CONVERSATIONAL);
        lteInfo->setRlcType((int) par("conversationalRlc"));
    }
    else if ((strcmp(pkt->getName(), "gaming")) == 0)
    {
        lteInfo->setApplication(GAMING);
        lteInfo->setTraffic(INTERACTIVE);
        lteInfo->setRlcType((int) par("interactiveRlc"));
    }
    else if ((strcmp(pkt->getName(), "VoDPacket") == 0)
        || (strcmp(pkt->getName(), "VoDFinishPacket") == 0))
    {
        lteInfo->setApplication(VOD);
        lteInfo->setTraffic(STREAMING);
        lteInfo->setRlcType((int) par("streamingRlc"));
    }
    else
    {
        lteInfo->setApplication(CBR);
        lteInfo->setTraffic(BACKGROUND);
        lteInfo->setRlcType((int) par("backgroundRlc"));
    }

    lteInfo->setDirection(getDirection());
}

/*
 * Upper Layer handlers
 */

void LtePdcpRrcBase::fromDataPort(cPacket *pktAux)
{
    emit(receivedPacketFromUpperLayer, pktAux);

    // Control Information
    auto pkt = check_and_cast<inet::Packet *> (pktAux);
    auto lteInfo = pkt->getTag<FlowControlInfo>();

    setTrafficInformation(pkt, lteInfo);

    MacNodeId destId = getDestId(lteInfo);

    // Cid Request
    EV << "LteRrc : Received CID request for Traffic [ " << "Source: " << Ipv4Address(lteInfo->getSrcAddr())
            << " Destination: " << Ipv4Address(lteInfo->getDstAddr())
            << " ToS: " << lteInfo->getTypeOfService() << " ]\n";

    // TODO: Since IP addresses can change when we add and remove nodes, maybe node IDs should be used instead of them
    LogicalCid mylcid;
    if ((mylcid = ht_->find_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(), lteInfo->getTypeOfService())) == 0xFFFF)
    {
        // LCID not found
        mylcid = lcid_++;

        EV << "LteRrc : Connection not found, new CID created with LCID " << mylcid << "\n";

        ht_->create_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(), lteInfo->getTypeOfService(), mylcid);
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
    LteTxPdcpEntity* entity = getTxEntity(cid);
    entity->handlePacketFromUpperLayer(pkt);
}

void LtePdcpRrcBase::fromEutranRrcSap(cPacket *pkt)
{
    // TODO For now use LCID 1000 for Control Traffic coming from RRC
    FlowControlInfo* lteInfo = new FlowControlInfo();
    lteInfo->setSourceId(nodeId_);
    lteInfo->setLcid(1000);
    lteInfo->setRlcType(TM);
    pkt->setControlInfo(lteInfo);
    EV << "LteRrc : Sending packet " << pkt->getName() << " on port TM_Sap$o\n";
    send(pkt, tmSap_[OUT_GATE]);
}

/*
 * Lower layer handlers
 */

void LtePdcpRrcBase::fromLowerLayer(cPacket *pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);
    emit(receivedPacketFromLowerLayer, pkt);

    auto lteInfo = pkt->getTag<FlowControlInfo>();

    MacCid cid = idToMacCid(lteInfo->getSourceId(), lteInfo->getLcid());   // TODO: check if you have to get master node id

    LteRxPdcpEntity* entity = getRxEntity(cid);
    entity->handlePacketFromLowerLayer(pkt);
}

void LtePdcpRrcBase::toDataPort(cPacket *pktAux)
{
    Enter_Method_Silent("LtePdcpRrcBase::toDataPort");

    auto pkt = check_and_cast<Packet *>(pktAux);
    take(pkt);

    EV << "LtePdcp : Sending packet " << pkt->getName() << " on port DataPort$o\n";

    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);

    EV << "LtePdcp : Sending packet " << pkt->getName()
       << " on port DataPort$o\n";

    // Send message
    send(pkt, dataPort_[OUT_GATE]);
    emit(sentPacketToUpperLayer, pkt);
}

void LtePdcpRrcBase::toEutranRrcSap(cPacket *pkt)
{
    cPacket* upPkt = pkt->decapsulate();
    delete pkt;

    EV << "LteRrc : Sending packet " << upPkt->getName()
       << " on port EUTRAN_RRC_Sap$o\n";
    send(upPkt, eutranRrcSap_[OUT_GATE]);
}

/*
 * Forwarding Handlers
 */

void LtePdcpRrcBase::sendToLowerLayer(Packet *pkt)
{
    auto lteInfo = pkt->getTag<FlowControlInfo>();

    std::string portName;
    omnetpp::cGate* gate;
    switch(lteInfo->getRlcType()){
    case UM:
        portName = "UM_Sap$o";
        gate = umSap_[OUT_GATE];
        break;
    case AM:
        portName = "AM_Sap$o";
        gate = amSap_[OUT_GATE];
        break;
    case TM:
        portName = "TM_Sap$o";
        gate = tmSap_[OUT_GATE];
        break;
    default:
        throw cRuntimeError("LtePdcpRrcBase::sendToLowerLayer(): invalid RlcType %d", lteInfo->getRlcType());
    }

    EV << "LtePdcp : Sending packet " << pkt->getName() << " on port " << portName << endl;

        // Send message
    send(pkt, gate);
    emit(sentPacketToLowerLayer, pkt);
}

void LtePdcpRrcBase::sendToUpperLayer(cPacket *pkt)
{
    EV << "LtePdcp : Sending packet " << pkt->getName() << " on port DataPort$o" << endl;

    // Send message
    send(pkt, dataPort_[OUT_GATE]);
    emit(sentPacketToUpperLayer, pkt);
}


/*
 * Main functions
 */

void LtePdcpRrcBase::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
    {
        dataPort_[IN_GATE] = gate("DataPort$i");
        dataPort_[OUT_GATE] = gate("DataPort$o");
        eutranRrcSap_[IN_GATE] = gate("EUTRAN_RRC_Sap$i");
        eutranRrcSap_[OUT_GATE] = gate("EUTRAN_RRC_Sap$o");
        tmSap_[IN_GATE] = gate("TM_Sap$i",0);
        tmSap_[OUT_GATE] = gate("TM_Sap$o",0);
        umSap_[IN_GATE] = gate("UM_Sap$i",0);
        umSap_[OUT_GATE] = gate("UM_Sap$o",0);
        amSap_[IN_GATE] = gate("AM_Sap$i",0);
        amSap_[OUT_GATE] = gate("AM_Sap$o",0);

        binder_ = getBinder();
        headerCompressedSize_ = B(par("headerCompressedSize"));
        if(headerCompressedSize_ != LTE_PDCP_HEADER_COMPRESSION_DISABLED &&
                headerCompressedSize_ < MIN_COMPRESSED_HEADER_SIZE)
        {
            throw cRuntimeError("Size of compressed header must not be less than %i", MIN_COMPRESSED_HEADER_SIZE.get());
        }

        nodeId_ = getAncestorPar("macNodeId");

        // statistics
        receivedPacketFromUpperLayer = registerSignal("receivedPacketFromUpperLayer");
        receivedPacketFromLowerLayer = registerSignal("receivedPacketFromLowerLayer");
        sentPacketToUpperLayer = registerSignal("sentPacketToUpperLayer");
        sentPacketToLowerLayer = registerSignal("sentPacketToLowerLayer");

        WATCH(headerCompressedSize_);
        WATCH(nodeId_);
        WATCH(lcid_);
    }
}

void LtePdcpRrcBase::handleMessage(cMessage* msg)
{
    cPacket* pkt = check_and_cast<cPacket *>(msg);
    EV << "LtePdcp : Received packet " << pkt->getName() << " from port "
       << pkt->getArrivalGate()->getName() << endl;

    cGate* incoming = pkt->getArrivalGate();
    if (incoming == dataPort_[IN_GATE])
    {
        fromDataPort(pkt);
    }
    else if (incoming == eutranRrcSap_[IN_GATE])
    {
        fromEutranRrcSap(pkt);
    }
    else if (incoming == tmSap_[IN_GATE])
    {
        toEutranRrcSap(pkt);
    }
    else
    {
        fromLowerLayer(pkt);
    }
    return;
}

LteTxPdcpEntity* LtePdcpRrcBase::getTxEntity(MacCid cid)
{
    // Find entity for this LCID
    PdcpTxEntities::iterator it = txEntities_.find(cid);
    if (it == txEntities_.end())
    {
        // Not found: create
        // Not found: create
        std::stringstream buf;
        // FIXME HERE

        buf << "LteTxPdcpEntity cid: " << cid;
        cModuleType* moduleType = cModuleType::get("simu5g.stack.pdcp_rrc.LteTxPdcpEntity");
        LteTxPdcpEntity* txEnt = check_and_cast<LteTxPdcpEntity *>(moduleType->createScheduleInit(buf.str().c_str(), this));
        txEntities_[cid] = txEnt;    // Add to entities map

        EV << "LtePdcpRrcBase::getTxEntity - Added new TxPdcpEntity for Cid: " << cid << "\n";

        return txEnt;
    }
    else
    {
        // Found
        EV << "LtePdcpRrcBase::getTxEntity - Using old TxPdcpEntity for Cid: " << cid << "\n";

        return it->second;
    }
}

LteRxPdcpEntity* LtePdcpRrcBase::getRxEntity(MacCid cid)
{
    // Find entity for this CID
    PdcpRxEntities::iterator it = rxEntities_.find(cid);
    if (it == rxEntities_.end())
    {
        // Not found: create

        std::stringstream buf;
        buf << "LteRxPdcpEntity Cid: " << cid;
        cModuleType* moduleType = cModuleType::get("simu5g.stack.pdcp_rrc.LteRxPdcpEntity");
        LteRxPdcpEntity* rxEnt = check_and_cast<LteRxPdcpEntity*>(moduleType->createScheduleInit(buf.str().c_str(), this));
        rxEntities_[cid] = rxEnt;    // Add to entities map

        EV << "LtePdcpRrcBase::getRxEntity - Added new RxPdcpEntity for Cid: " << cid << "\n";

        return rxEnt;
    }
    else
    {
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
    if (stage == inet::INITSTAGE_LOCAL)
        nodeId_ = getAncestorPar("macNodeId");
}

void LtePdcpRrcEnb::deleteEntities(MacNodeId nodeId)
{
    PdcpTxEntities::iterator tit;
    PdcpRxEntities::iterator rit;

    // delete connections related to the given UE
    for (tit = txEntities_.begin(); tit != txEntities_.end(); )
    {
        if (MacCidToNodeId(tit->first) == nodeId)
        {
            (tit->second)->deleteModule();  // Delete Entity
            txEntities_.erase(tit++);       // Delete Elem
        }
        else
        {
            ++tit;
        }
    }
    for (rit = rxEntities_.begin(); rit != rxEntities_.end(); )
    {
        if (MacCidToNodeId(rit->first) == nodeId)
        {
            (rit->second)->deleteModule();  // Delete Entity
            rxEntities_.erase(rit++);       // Delete Elem
        }
        else
        {
            ++rit;
        }
    }
}

void LtePdcpRrcUe::deleteEntities(MacNodeId nodeId)
{
    PdcpTxEntities::iterator tit;
    PdcpRxEntities::iterator rit;

    // delete all connections TODO: check this (for NR dual connectivity)
    for (tit = txEntities_.begin(); tit != txEntities_.end(); )
    {
        (tit->second)->deleteModule();  // Delete Entity
        txEntities_.erase(tit++);       // Delete Elem
    }
    for (rit = rxEntities_.begin(); rit != rxEntities_.end(); )
    {
        (rit->second)->deleteModule();  // Delete Entity
        rxEntities_.erase(rit++);       // Delete Elem
    }
}

void LtePdcpRrcUe::initialize(int stage)
{
    LtePdcpRrcBase::initialize(stage);
    if (stage == inet::INITSTAGE_NETWORK_LAYER)
    {
        nodeId_ = getAncestorPar("macNodeId");
    }
}
