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

#include "simu5g/stack/pdcp/LtePdcp.h"

#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/transportlayer/tcp_common/TcpHeader.h>
#include <inet/transportlayer/udp/UdpHeader_m.h>

#include "simu5g/stack/packetFlowObserver/PacketFlowObserverBase.h"
#include "simu5g/stack/pdcp/packet/LteRohcPdu_m.h"
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"
#include "simu5g/common/LteControlInfoTags_m.h"
#include "simu5g/stack/d2dModeSelection/D2DModeSwitchNotification_m.h"
#include "simu5g/x2/packet/X2ControlInfo_m.h"

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

/*
 * Upper Layer handlers
 */

void LtePdcpBase::fromDataPort(cPacket *pktAux)
{
    emit(receivedPacketFromUpperLayerSignal_, pktAux);

    auto pkt = check_and_cast<inet::Packet *>(pktAux);

    auto lteInfo = pkt->getTag<FlowControlInfo>();
    verifyControlInfo(lteInfo.get());

    MacCid cid = MacCid(lteInfo->getDestId(), lteInfo->getLcid());

    if (isDualConnectivityEnabled() && lteInfo->getMulticastGroupId() == NODEID_NONE) {
        // Handle DC setup: Assume packet arrives in Master nodeB (LTE), and wants to use Secondary nodeB (NR).
        // Packet is processed by local PDCP entity, then needs to be tunneled over X2 to Secondary for transmission.
        // However, local PDCP entity is keyed on LTE nodeIds, so we need to tweak the cid and replace NR nodeId
        // with LTE nodeId so that lookup succeeds.
        if (getNodeTypeById(nodeId_) == NODEB && binder_->isGNodeB(nodeId_) != isNrUe(lteInfo->getDestId()) ) {
            // use another CID whose technology matches the nodeB
            MacNodeId otherDestId = binder_->getUeNodeId(lteInfo->getDestId(), !isNrUe(lteInfo->getDestId()));
            ASSERT(otherDestId != NODEID_NONE);
            cid = MacCid(otherDestId, lteInfo->getLcid());
        }

        // Handle DC setup on UE side: both legs should use the *same* key for entity lookup
        if (getNodeTypeById(nodeId_) == UE && getNodeTypeById(lteInfo->getDestId()) == NODEB)  {
            MacNodeId lteNodeB = binder_->getServingNode(nodeId_);
            cid = MacCid(lteNodeB, lteInfo->getLcid());
        }
    }

    LteTxPdcpEntity *entity = lookupTxEntity(cid);

    // get the PDCP entity for this LCID and process the packet
    EV << "fromDataPort in " << getFullPath() << " event #" << getSimulation()->getEventNumber()
       << ": Processing packet " << pkt->getName() << " src=" << lteInfo->getSourceId() << " dest=" << lteInfo->getDestId()
       << " multicast=" << lteInfo->getMulticastGroupId() << " direction=" << dirToA((Direction)lteInfo->getDirection())
       << " ---> CID " << cid << (entity == nullptr ? " (NEW)" : " (existing)") << std::endl;

    if (entity == nullptr) {
        binder_->establishUnidirectionalDataConnection((FlowControlInfo *)lteInfo.get());
        entity = lookupTxEntity(cid);
        ASSERT(entity != nullptr);
    }

    entity->handlePacketFromUpperLayer(pkt);
}

/*
 * Lower layer handlers
 */

void LtePdcpBase::fromLowerLayer(cPacket *pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);

    // NrPdcpEnb: if DC enabled and this is a secondary node, forward to master
    if (isNR_ && getNodeTypeById(nodeId_) == NODEB) {
        pkt->trim();
        MacNodeId masterId = binder_->getMasterNodeOrSelf(nodeId_);
        if (dualConnectivityEnabled_ && (nodeId_ != masterId)) {
            EV << NOW << " LtePdcpBase::fromLowerLayer - forward packet to the master node - id [" << masterId << "]" << endl;
            forwardDataToTargetNode(pkt, masterId);
            return;
        }
    }

    emit(receivedPacketFromLowerLayerSignal_, pkt);

    ASSERT(pkt->findTag<PdcpTrackingTag>() == nullptr);

    auto lteInfo = pkt->getTag<FlowControlInfo>();
    MacCid cid = MacCid(lteInfo->getSourceId(), lteInfo->getLcid());

    if (isDualConnectivityEnabled()) {
        // Handle DC setup: Assume packet arrives at this Master nodeB (LTE) from Secondary (NR) over X2.
        // Packet needs to be processed by local PDCP entity. However, local PDCP entity is keyed on LTE nodeIds,
        // so we need to tweak the cid and replace NR nodeId with LTE nodeId so that lookup succeeds.
        if (getNodeTypeById(nodeId_) == NODEB && binder_->isGNodeB(nodeId_) != isNrUe(lteInfo->getSourceId()) ) {
            // use another CID whose technology matches the nodeB
            MacNodeId otherSourceId = binder_->getUeNodeId(lteInfo->getSourceId(), !isNrUe(lteInfo->getSourceId()));
            ASSERT(otherSourceId != NODEID_NONE);
            cid = MacCid(otherSourceId, lteInfo->getLcid());
        }

        // Handle DC setup on UE side: both legs should use the *same* key for entity lookup
        if (getNodeTypeById(nodeId_) == UE && getNodeTypeById(lteInfo->getSourceId()) == NODEB)  {
            MacNodeId lteNodeB = binder_->getServingNode(nodeId_);
            cid = MacCid(lteNodeB, lteInfo->getLcid());
        }
    }

    // Handle DC setup on UE side: UE receives packet from base station
    // and needs to use the correct PDCP entity based on technology matching
    if (getNodeTypeById(nodeId_) == UE && lteInfo->getMulticastGroupId() == NODEID_NONE && isDualConnectivityEnabled()) {
        MacNodeId servingNodeId = binder_->getServingNode(nodeId_);

        // Check if there's a technology mismatch between packet source and UE's serving base station
        if (servingNodeId != NODEID_NONE &&
            binder_->isGNodeB(lteInfo->getSourceId()) != binder_->isGNodeB(servingNodeId)) {

            // Use alternate base station nodeId (Master or Secondary) for proper PDCP entity lookup
            MacNodeId otherSrcId;
            if (binder_->isGNodeB(lteInfo->getSourceId())) {
                // Packet came from gNodeB, get its master (LTE eNodeB)
                otherSrcId = binder_->getMasterNodeOrSelf(lteInfo->getSourceId());
            } else {
                // Packet came from eNodeB, get its secondary (NR gNodeB)
                otherSrcId = binder_->getSecondaryNode(lteInfo->getSourceId());
            }

            if (otherSrcId != NODEID_NONE && otherSrcId != lteInfo->getSourceId()) {
                cid = MacCid(otherSrcId, lteInfo->getLcid());

                EV << "LtePdcp: UE DC RX - Using alternate base station ID " << otherSrcId
                   << " instead of " << lteInfo->getSourceId()
                   << " for technology match with serving node " << servingNodeId << endl;
            }
        }
    }

    LteRxPdcpEntity *entity = lookupRxEntity(cid);

    EV << "fromLowerLayer in " << getFullPath() << " event #" << getSimulation()->getEventNumber()
       <<  ": Processing packet " << pkt->getName() << " src=" << lteInfo->getSourceId() << " dest=" << lteInfo->getDestId()
       << " multicast=" << lteInfo->getMulticastGroupId() << " direction=" << dirToA((Direction)lteInfo->getDirection())
       << " ---> CID " << cid << (entity == nullptr ? " (NEW)" : " (existing)") << std::endl;

    ASSERT(entity != nullptr);

    entity->handlePacketFromLowerLayer(pkt);
}

void LtePdcpBase::toDataPort(cPacket *pktAux)
{
    Enter_Method_Silent("LtePdcpBase::toDataPort");

    auto pkt = check_and_cast<Packet *>(pktAux);
    take(pkt);

    EV << "LtePdcp : Sending packet " << pkt->getName() << " on port upperLayerOut\n";

    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);

    EV << "LtePdcp : Sending packet " << pkt->getName() << " on port upperLayerOut\n";

    // Send message
    send(pkt, upperLayerOutGate_);
    emit(sentPacketToUpperLayerSignal_, pkt);
}

/*
 * Forwarding Handlers
 */

void LtePdcpBase::sendToLowerLayer(Packet *pkt)
{
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();

    // NrPdcpUe: route to NR or LTE RLC depending on DC and useNR flag
    if (isNR_ && nrRlcOutGate_ != nullptr) {
        bool useNR = pkt->getTag<TechnologyReq>()->getUseNR();
        if (!dualConnectivityEnabled_ || useNR) {
            EV << "NrPdcpUe : Sending packet " << pkt->getName() << " on port " << nrRlcOutGate_->getFullName() << endl;

            // use NR id as source
            lteInfo->setSourceId(nrNodeId_);

            // notify the packetFlowObserver only with UL packet
            if (lteInfo->getDirection() != D2D_MULTI && lteInfo->getDirection() != D2D) {
                if (NRpacketFlowObserver_ != nullptr) {
                    EV << "LteTxPdcpEntity::handlePacketFromUpperLayer - notify NRpacketFlowObserver_" << endl;
                    NRpacketFlowObserver_->insertPdcpSdu(pkt);
                }
            }

            send(pkt, nrRlcOutGate_);
            emit(sentPacketToLowerLayerSignal_, pkt);
            return;
        }
    }

    EV << "LtePdcp : Sending packet " << pkt->getName() << " on port " << rlcOutGate_->getFullName() << endl;

    if (lteInfo->getDirection() != D2D_MULTI && lteInfo->getDirection() != D2D) {
        if (packetFlowObserver_ != nullptr)
            packetFlowObserver_->insertPdcpSdu(pkt);
    }

    // Send message
    send(pkt, rlcOutGate_);
    emit(sentPacketToLowerLayerSignal_, pkt);
}

/*
 * Main functions
 */

void LtePdcpBase::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        upperLayerInGate_ = gate("upperLayerIn");
        upperLayerOutGate_ = gate("upperLayerOut");
        rlcInGate_ = gate("rlcIn");
        rlcOutGate_ = gate("rlcOut");

        binder_.reference(this, "binderModule", true);

        nodeId_ = MacNodeId(getContainingNode(this)->par("macNodeId").intValue());

        packetFlowObserver_.reference(this, "packetFlowObserverModule", false);
        NRpacketFlowObserver_.reference(this, "nrPacketFlowObserverModule", false);

        if (packetFlowObserver_) {
            EV << "LtePdcpBase::initialize - PacketFlowObserver present" << endl;
        }
        if (NRpacketFlowObserver_) {
            EV << "LtePdcpBase::initialize - NRpacketFlowObserver present" << endl;
        }

        const char *rxEntityModuleTypeName = par("rxEntityModuleType").stringValue();
        rxEntityModuleType_ = cModuleType::get(rxEntityModuleTypeName);

        const char *txEntityModuleTypeName = par("txEntityModuleType").stringValue();
        txEntityModuleType_ = cModuleType::get(txEntityModuleTypeName);

        // TODO WATCH_MAP(gatemap_);
        WATCH(nodeId_);
    }
}

void LtePdcpBase::handleMessage(cMessage *msg)
{
    cPacket *pkt = check_and_cast<cPacket *>(msg);
    EV << "LtePdcp : Received packet " << pkt->getName() << " from port "
       << pkt->getArrivalGate()->getName() << endl;

    // NrPdcpEnb: incoming data from DualConnectivityManager via X2
    if (isNR_ && getNodeTypeById(nodeId_) == NODEB && msg->getArrivalGate()->isName("dcManagerIn")) {
        EV << "LtePdcpBase::handleMessage - Received packet from DualConnectivityManager" << endl;
        auto datagram = check_and_cast<Packet*>(pkt);
        auto tag = datagram->removeTag<X2SourceNodeInd>();
        MacNodeId sourceNode = tag->getSourceNode();
        receiveDataFromSourceNode(datagram, sourceNode);
        return;
    }

    // D2D mode switch notification (LtePdcpEnbD2D / LtePdcpUeD2D / NrPdcpEnb / NrPdcpUe)
    if (hasD2DSupport_) {
        auto inet_pkt = check_and_cast<inet::Packet *>(pkt);
        auto chunk = inet_pkt->peekAtFront<Chunk>();
        if (inet::dynamicPtrCast<const D2DModeSwitchNotification>(chunk) != nullptr) {
            EV << "LtePdcpBase::handleMessage - Received packet " << pkt->getName() << " from port " << pkt->getArrivalGate()->getName() << endl;
            auto switchPkt = inet_pkt->peekAtFront<D2DModeSwitchNotification>();
            pdcpHandleD2DModeSwitch(switchPkt->getPeerId(), switchPkt->getNewMode());
            delete pkt;
            return;
        }
    }

    cGate *incoming = pkt->getArrivalGate();
    if (incoming == upperLayerInGate_) {
        fromDataPort(pkt);
    }
    else {
        fromLowerLayer(pkt);
    }
}

void LtePdcpBase::pdcpHandleD2DModeSwitch(MacNodeId peerId, LteD2DMode newMode)
{
    EV << NOW << " LtePdcpBase::pdcpHandleD2DModeSwitch - peering with UE " << peerId << " set to " << d2dModeToA(newMode) << endl;
    // add here specific behavior for handling mode switch at the PDCP layer
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

} //namespace
