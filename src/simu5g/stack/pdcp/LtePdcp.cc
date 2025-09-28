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

#include "simu5g/stack/pdcp/LtePdcp.h"

#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/transportlayer/tcp_common/TcpHeader.h>
#include <inet/transportlayer/udp/UdpHeader_m.h>

#include "simu5g/stack/packetFlowManager/PacketFlowManagerBase.h"
#include "simu5g/stack/pdcp/packet/LteRohcPdu_m.h"

namespace simu5g {

Define_Module(LtePdcpUe);
Define_Module(LtePdcpEnb);

using namespace omnetpp;
using namespace inet;

// statistics
simsignal_t LtePdcpBase::receivedPacketFromUpperLayerSignal_ = registerSignal("receivedPacketFromUpperLayer");
simsignal_t LtePdcpBase::receivedPacketFromLowerLayerSignal_ = registerSignal("receivedPacketFromLowerLayer");
simsignal_t LtePdcpBase::sentPacketToUpperLayerSignal_ = registerSignal("sentPacketToUpperLayer");
simsignal_t LtePdcpBase::sentPacketToLowerLayerSignal_ = registerSignal("sentPacketToLowerLayer");

LtePdcpBase::~LtePdcpBase()
{
}

ApplicationType LtePdcpBase::getApplication(cPacket *pkt)
{
    if ((strcmp(pkt->getName(), "VoIP")) == 0)
        return VOIP;
    else if ((strcmp(pkt->getName(), "gaming")) == 0)
        return GAMING;
    else if ((strcmp(pkt->getName(), "VoDPacket") == 0) || (strcmp(pkt->getName(), "VoDFinishPacket") == 0))
        return VOD;
    else
        return CBR;
}

LteTrafficClass LtePdcpBase::getTrafficCategory(cPacket *pkt)
{
    if ((strcmp(pkt->getName(), "VoIP")) == 0)
        return CONVERSATIONAL;
    else if ((strcmp(pkt->getName(), "gaming")) == 0)
        return INTERACTIVE;
    else if ((strcmp(pkt->getName(), "VoDPacket") == 0) || (strcmp(pkt->getName(), "VoDFinishPacket") == 0))
        return STREAMING;
    else
        return BACKGROUND;
}

LteRlcType LtePdcpBase::getRlcType(LteTrafficClass trafficCategory)
{
    switch (trafficCategory) {
        case CONVERSATIONAL:
            return conversationalRlc_;
        case INTERACTIVE:
            return interactiveRlc_;
        case STREAMING:
            return streamingRlc_;
        case BACKGROUND:
            return backgroundRlc_;
        default:
            return backgroundRlc_;  // fallback
    }
}

void LtePdcpBase::setTrafficInformation(cPacket *pkt, inet::Ptr<FlowControlInfo> lteInfo)
{
    ApplicationType application = getApplication(pkt);
    LteTrafficClass trafficCategory = getTrafficCategory(pkt);
    LteRlcType rlcType = getRlcType(trafficCategory);

    lteInfo->setApplication(application);
    lteInfo->setTraffic(trafficCategory);
    lteInfo->setRlcType(rlcType);

    // direction of transmitted packets depends on node type
    Direction dir = getNodeTypeById(nodeId_) == UE ? UL : DL;
    lteInfo->setDirection(dir);
}

LogicalCid LtePdcpBase::lookupOrAssignLcid(const ConnectionKey& key)
{
    auto it = lcidTable_.find(key);
    if (it != lcidTable_.end())
        return it->second;
    else {
        LogicalCid lcid = lcid_++;
        lcidTable_[key] = lcid;
        EV << "Connection not found, new CID created with LCID " << lcid << "\n";
        return lcid;
    }
}

/*
 * Upper Layer handlers
 */

void LtePdcpBase::analyzePacket(inet::Packet *pkt)
{
    // Control Information
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();

    setTrafficInformation(pkt, lteInfo);

    MacNodeId destId = getDestId(lteInfo);

    // CID Request
    EV << "LteRrc : Received CID request for Traffic [ " << "Source: " << Ipv4Address(lteInfo->getSrcAddr())
       << " Destination: " << Ipv4Address(lteInfo->getDstAddr())
       << " ToS: " << lteInfo->getTypeOfService() << " ]\n";

    // TODO: Since IP addresses can change when we add and remove nodes, maybe node IDs should be used instead of them
    ConnectionKey key{Ipv4Address(lteInfo->getSrcAddr()), Ipv4Address(lteInfo->getDstAddr()), lteInfo->getTypeOfService(), 0xFFFF};
    LogicalCid lcid = lookupOrAssignLcid(key);

    // assign LCID
    lteInfo->setLcid(lcid);
    lteInfo->setSourceId(nodeId_);
    lteInfo->setDestId(destId);

    // this is the body of former LteTxPdcpEntity::setIds()
    lteInfo->setSourceId(getNodeId());
    if (lteInfo->getMulticastGroupId() > 0)                                               // destId is meaningless for multicast D2D (we use the id of the source for statistic purposes at lower levels)
        lteInfo->setDestId(getNodeId());
    else
        lteInfo->setDestId(getDestId(lteInfo));
}

void LtePdcpBase::fromDataPort(cPacket *pktAux)
{
    emit(receivedPacketFromUpperLayerSignal_, pktAux);

    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    analyzePacket(pkt);

    auto lteInfo = pkt->getTag<FlowControlInfo>();
    verifyControlInfo(lteInfo.get());

    MacCid cid = MacCid(lteInfo->getDestId(), lteInfo->getLcid());

    LteTxPdcpEntity *entity = lookupTxEntity(cid);

    // get the PDCP entity for this LCID and process the packet
    EV << "fromDataPort in " << getFullPath() << " event #" << getSimulation()->getEventNumber()
       << ": Processing packet " << pkt->getName() << " src=" << lteInfo->getSourceId() << " dest=" << lteInfo->getDestId()
       << " multicast=" << lteInfo->getMulticastGroupId() << " direction=" << dirToA((Direction)lteInfo->getDirection())
       << " ---> CID " << cid << (entity == nullptr ? " (NEW)" : " (existing)") << std::endl;

    if (entity == nullptr)
        entity = createTxEntity(cid);
    entity->handlePacketFromUpperLayer(pkt);
}

/*
 * Lower layer handlers
 */

void LtePdcpBase::fromLowerLayer(cPacket *pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);
    emit(receivedPacketFromLowerLayerSignal_, pkt);

    auto lteInfo = pkt->getTag<FlowControlInfo>();

    MacCid cid = MacCid(lteInfo->getSourceId(), lteInfo->getLcid());   // TODO: check if you have to get master node id

    LteRxPdcpEntity *entity = lookupRxEntity(cid);
    if (entity == nullptr)
        entity = createRxEntity(cid);
    entity->handlePacketFromLowerLayer(pkt);
}

void LtePdcpBase::toDataPort(cPacket *pktAux)
{
    Enter_Method_Silent("LtePdcpBase::toDataPort");

    auto pkt = check_and_cast<Packet *>(pktAux);
    take(pkt);

    EV << "LtePdcp : Sending packet " << pkt->getName() << " on port DataPort$o\n";

    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);

    EV << "LtePdcp : Sending packet " << pkt->getName() << " on port DataPort$o\n";

    // Send message
    send(pkt, dataPortOutGate_);
    emit(sentPacketToUpperLayerSignal_, pkt);
}

/*
 * Forwarding Handlers
 */

void LtePdcpBase::sendToLowerLayer(Packet *pkt)
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
            throw cRuntimeError("LtePdcpBase::sendToLowerLayer(): invalid RlcType %d", lteInfo->getRlcType());
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

/*
 * Main functions
 */

void LtePdcpBase::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        dataPortInGate_ = gate("DataPort$i");
        dataPortOutGate_ = gate("DataPort$o");
        tmSapInGate_ = gate("TM_Sap$i", 0);
        tmSapOutGate_ = gate("TM_Sap$o", 0);
        umSapInGate_ = gate("UM_Sap$i", 0);
        umSapOutGate_ = gate("UM_Sap$o", 0);
        amSapInGate_ = gate("AM_Sap$i", 0);
        amSapOutGate_ = gate("AM_Sap$o", 0);

        binder_.reference(this, "binderModule", true);

        nodeId_ = MacNodeId(getContainingNode(this)->par("macNodeId").intValue());

        packetFlowManager_.reference(this, "packetFlowManagerModule", false);
        NRpacketFlowManager_.reference(this, "nrPacketFlowManagerModule", false);

        if (packetFlowManager_) {
            EV << "LtePdcpBase::initialize - PacketFlowManager present" << endl;
        }
        if (NRpacketFlowManager_) {
            EV << "LtePdcpBase::initialize - NRpacketFlowManager present" << endl;
        }

        conversationalRlc_ = aToRlcType(par("conversationalRlc"));
        interactiveRlc_ = aToRlcType(par("interactiveRlc"));
        streamingRlc_ = aToRlcType(par("streamingRlc"));
        backgroundRlc_ = aToRlcType(par("backgroundRlc"));

        const char *rxEntityModuleTypeName = par("rxEntityModuleType").stringValue();
        rxEntityModuleType_ = cModuleType::get(rxEntityModuleTypeName);

        const char *txEntityModuleTypeName = par("txEntityModuleType").stringValue();
        txEntityModuleType_ = cModuleType::get(txEntityModuleTypeName);

        // TODO WATCH_MAP(gatemap_);
        WATCH(nodeId_);
        WATCH(lcid_);
    }
}

void LtePdcpBase::handleMessage(cMessage *msg)
{
    cPacket *pkt = check_and_cast<cPacket *>(msg);
    EV << "LtePdcp : Received packet " << pkt->getName() << " from port "
       << pkt->getArrivalGate()->getName() << endl;

    cGate *incoming = pkt->getArrivalGate();
    if (incoming == dataPortInGate_) {
        fromDataPort(pkt);
    }
    else {
        fromLowerLayer(pkt);
    }
}

LteTxPdcpEntity *LtePdcpBase::lookupTxEntity(MacCid cid)
{
    auto it = txEntities_.find(cid);
    return it != txEntities_.end() ? it->second : nullptr;
}

LteTxPdcpEntity *LtePdcpBase::createTxEntity(MacCid cid)
{
    std::stringstream buf;
    buf << "tx-" << cid.getNodeId() << "-" << cid.getLcid();
    LteTxPdcpEntity *txEnt = check_and_cast<LteTxPdcpEntity *>(txEntityModuleType_->createScheduleInit(buf.str().c_str(), this));
    txEntities_[cid] = txEnt;

    EV << "LtePdcpBase::createTxEntity - Added new TxPdcpEntity for Cid: " << cid << "\n";

    return txEnt;
}


LteRxPdcpEntity *LtePdcpBase::lookupRxEntity(MacCid cid)
{
    auto it = rxEntities_.find(cid);
    return it != rxEntities_.end() ? it->second : nullptr;
}

LteRxPdcpEntity *LtePdcpBase::createRxEntity(MacCid cid)
{
    std::stringstream buf;
    buf << "rx-" << cid.getNodeId() << "-" << cid.getLcid();
    LteRxPdcpEntity *rxEnt = check_and_cast<LteRxPdcpEntity *>(rxEntityModuleType_->createScheduleInit(buf.str().c_str(), this));
    rxEntities_[cid] = rxEnt;

    EV << "LtePdcpBase::createRxEntity - Added new RxPdcpEntity for Cid: " << cid << "\n";

    return rxEnt;
}


void LtePdcpBase::finish()
{
    // TODO make-finish
}

void LtePdcpEnb::initialize(int stage)
{
    LtePdcpBase::initialize(stage);
}

void LtePdcpEnb::deleteEntities(MacNodeId nodeId)
{
    Enter_Method_Silent();

    // delete connections related to the given UE
    for (auto tit = txEntities_.begin(); tit != txEntities_.end(); ) {
        auto& [cid, txEntity] = *tit;
        if (cid.getNodeId() == nodeId) {
            txEntity->deleteModule();
            tit = txEntities_.erase(tit);
        }
        else {
            ++tit;
        }
    }

    for (auto rit = rxEntities_.begin(); rit != rxEntities_.end(); ) {
        auto& [cid, rxEntity] = *rit;
        if (cid.getNodeId() == nodeId) {
            rxEntity->deleteModule();
            rit = rxEntities_.erase(rit);
        }
        else {
            ++rit;
        }
    }
}

void LtePdcpUe::deleteEntities(MacNodeId nodeId)
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

void LtePdcpUe::initialize(int stage)
{
    LtePdcpBase::initialize(stage);
    if (stage == inet::INITSTAGE_NETWORK_LAYER) {
        // refresh value, the parameter may have changed between INITSTAGE_LOCAL and INITSTAGE_NETWORK_LAYER
        nodeId_ = MacNodeId(getContainingNode(this)->par("macNodeId").intValue());
    }
}

} //namespace
