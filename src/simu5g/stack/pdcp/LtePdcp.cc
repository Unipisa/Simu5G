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

#include <inet/networklayer/common/NetworkInterface.h>
#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/transportlayer/tcp_common/TcpHeader.h>
#include <inet/transportlayer/udp/UdpHeader_m.h>

#include "simu5g/stack/pdcp/packet/LteRohcPdu_m.h"
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"
#include "simu5g/common/LteControlInfoTags_m.h"
#include "simu5g/stack/d2dModeSelection/D2DModeSwitchNotification_m.h"
#include "simu5g/x2/packet/X2ControlInfo_m.h"

namespace simu5g {

Define_Module(LtePdcp);

using namespace omnetpp;
using namespace inet;


LtePdcp::~LtePdcp()
{
}

/*
 * Upper Layer handlers
 */

void LtePdcp::fromDataPort(cPacket *pktAux)
{
    auto pkt = check_and_cast<inet::Packet *>(pktAux);

    auto lteInfo = pkt->getTag<FlowControlInfo>();
    verifyControlInfo(lteInfo.get());

    DrbKey id = DrbKey(lteInfo->getDestId(), lteInfo->getDrbId());

    if (isDualConnectivityEnabled() && lteInfo->getMulticastGroupId() == NODEID_NONE) {
        // Handle DC setup: Assume packet arrives in Master nodeB (LTE), and wants to use Secondary nodeB (NR).
        // Packet is processed by local PDCP entity, then needs to be tunneled over X2 to Secondary for transmission.
        // However, local PDCP entity is keyed on LTE nodeIds, so we need to tweak the id and replace NR nodeId
        // with LTE nodeId so that lookup succeeds.
        if (getNodeTypeById(nodeId_) == NODEB && binder_->isGNodeB(nodeId_) != isNrUe(lteInfo->getDestId()) ) {
            // use another ID whose technology matches the nodeB
            MacNodeId otherDestId = binder_->getUeNodeId(lteInfo->getDestId(), !isNrUe(lteInfo->getDestId()));
            ASSERT(otherDestId != NODEID_NONE);
            id = DrbKey(otherDestId, lteInfo->getDrbId());
        }

        // Handle DC setup on UE side: both legs should use the *same* key for entity lookup
        if (getNodeTypeById(nodeId_) == UE && getNodeTypeById(lteInfo->getDestId()) == NODEB)  {
            MacNodeId lteNodeB = binder_->getServingNode(nodeId_);
            id = DrbKey(lteNodeB, lteInfo->getDrbId());
        }
    }

    PdcpTxEntityBase *entity = lookupTxEntity(id);

    // get the PDCP entity for this DRB ID and process the packet
    EV << "fromDataPort in " << getFullPath() << " event #" << getSimulation()->getEventNumber()
       << ": Processing packet " << pkt->getName() << " src=" << lteInfo->getSourceId() << " dest=" << lteInfo->getDestId()
       << " multicast=" << lteInfo->getMulticastGroupId() << " direction=" << dirToA((Direction)lteInfo->getDirection())
       << " ---> " << id << (entity == nullptr ? " (NEW)" : " (existing)") << std::endl;

    if (entity == nullptr) {
        binder_->establishUnidirectionalDataConnection((FlowControlInfo *)lteInfo.get());
        entity = lookupTxEntity(id);
        ASSERT(entity != nullptr);
    }

    entity->handlePacketFromUpperLayer(pkt);
}

/*
 * Lower layer handlers
 */

void LtePdcp::fromLowerLayer(cPacket *pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);

    // NrPdcpEnb: trim packet for NR gNBs (removes trailing RLC/MAC padding)
    if (isNR_ && getNodeTypeById(nodeId_) == NODEB) {
        pkt->trim();
    }

    ASSERT(pkt->findTag<PdcpTrackingTag>() == nullptr);

    auto lteInfo = pkt->getTag<FlowControlInfo>();
    DrbKey id = DrbKey(lteInfo->getSourceId(), lteInfo->getDrbId());

    if (isDualConnectivityEnabled()) {
        // Handle DC setup: Assume packet arrives at this Master nodeB (LTE) from Secondary (NR) over X2.
        // Packet needs to be processed by local PDCP entity. However, local PDCP entity is keyed on LTE nodeIds,
        // so we need to tweak the id and replace NR nodeId with LTE nodeId so that lookup succeeds.
        if (getNodeTypeById(nodeId_) == NODEB && binder_->isGNodeB(nodeId_) != isNrUe(lteInfo->getSourceId()) ) {
            // use another ID whose technology matches the nodeB
            MacNodeId otherSourceId = binder_->getUeNodeId(lteInfo->getSourceId(), !isNrUe(lteInfo->getSourceId()));
            ASSERT(otherSourceId != NODEID_NONE);
            id = DrbKey(otherSourceId, lteInfo->getDrbId());
        }

        // Handle DC setup on UE side: both legs should use the *same* key for entity lookup
        if (getNodeTypeById(nodeId_) == UE && getNodeTypeById(lteInfo->getSourceId()) == NODEB)  {
            MacNodeId lteNodeB = binder_->getServingNode(nodeId_);
            id = DrbKey(lteNodeB, lteInfo->getDrbId());
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
                id = DrbKey(otherSrcId, lteInfo->getDrbId());

                EV << "LtePdcp: UE DC RX - Using alternate base station ID " << otherSrcId
                   << " instead of " << lteInfo->getSourceId()
                   << " for technology match with serving node " << servingNodeId << endl;
            }
        }
    }

    PdcpRxEntityBase *entity = lookupRxEntity(id);

    EV << "fromLowerLayer in " << getFullPath() << " event #" << getSimulation()->getEventNumber()
       <<  ": Processing packet " << pkt->getName() << " src=" << lteInfo->getSourceId() << " dest=" << lteInfo->getDestId()
       << " multicast=" << lteInfo->getMulticastGroupId() << " direction=" << dirToA((Direction)lteInfo->getDirection())
       << " ---> " << id << (entity == nullptr ? " (NEW)" : " (existing)") << std::endl;

    ASSERT(entity != nullptr);

    entity->handlePacketFromLowerLayer(pkt);
}

void LtePdcp::sendToUpperLayer(Packet *pkt)
{
    Enter_Method_Silent("sendToUpperLayer");
    take(pkt);

    EV << "LtePdcp : Sending packet " << pkt->getName() << " on port upperLayerOut\n";

    send(pkt, upperLayerOutGate_);
}

void LtePdcp::sendToRlc(Packet *pkt)
{
    Enter_Method_Silent("sendToRlc");
    take(pkt);

    EV << "LtePdcp : Sending packet " << pkt->getName() << " on port " << rlcOutGate_->getFullName() << endl;

    send(pkt, rlcOutGate_);
}

void LtePdcp::sendToNrRlc(Packet *pkt)
{
    Enter_Method_Silent("sendToNrRlc");
    take(pkt);

    EV << "LtePdcp : Sending packet " << pkt->getName() << " on port " << nrRlcOutGate_->getFullName() << endl;

    send(pkt, nrRlcOutGate_);
}

void LtePdcp::sendToX2(Packet *pkt)
{
    Enter_Method_Silent("sendToX2");
    take(pkt);

    EV << NOW << " LtePdcp::sendToX2 - Send PDCP packet via X2" << endl;

    send(pkt, "dcManagerOut");
}

/*
 * Main functions
 */

void LtePdcp::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        upperLayerInGate_ = gate("upperLayerIn");
        upperLayerOutGate_ = gate("upperLayerOut");
        rlcInGate_ = gate("rlcIn");
        rlcOutGate_ = gate("rlcOut");

        binder_.reference(this, "binderModule", true);

        nodeId_ = MacNodeId(getContainingNode(this)->par("macNodeId").intValue());

        const char *rxEntityModuleTypeName = par("rxEntityModuleType").stringValue();
        rxEntityModuleType_ = cModuleType::get(rxEntityModuleTypeName);

        const char *txEntityModuleTypeName = par("txEntityModuleType").stringValue();
        txEntityModuleType_ = cModuleType::get(txEntityModuleTypeName);

        const char *bypassRxEntityModuleTypeName = par("bypassRxEntityModuleType").stringValue();
        bypassRxEntityModuleType_ = cModuleType::get(bypassRxEntityModuleTypeName);

        const char *bypassTxEntityModuleTypeName = par("bypassTxEntityModuleType").stringValue();
        bypassTxEntityModuleType_ = cModuleType::get(bypassTxEntityModuleTypeName);

        // Set flags from NED parameters
        isNR_ = par("isNR").boolValue();
        hasD2DSupport_ = par("hasD2DSupport").boolValue();

        // NR-specific initialization
        if (isNR_) {
            inet::NetworkInterface *nic = inet::getContainingNicModule(this);
            dualConnectivityEnabled_ = nic->par("dualConnectivityEnabled").boolValue();

            if (getNodeTypeById(nodeId_) == UE) {
                // NrPdcpUe: read NR node ID and initialize NR RLC gate
                nrNodeId_ = MacNodeId(getContainingNode(this)->par("nrMacNodeId").intValue());
                nrRlcOutGate_ = gate("nrRlcOut");
            }
        }

        // TODO WATCH_MAP(gatemap_);
        WATCH(nodeId_);
    }
}

void LtePdcp::handleMessage(cMessage *msg)
{
    cPacket *pkt = check_and_cast<cPacket *>(msg);
    EV << "LtePdcp : Received packet " << pkt->getName() << " from port "
       << pkt->getArrivalGate()->getName() << endl;

    // NrPdcpEnb: incoming data from DualConnectivityManager via X2
    if (isNR_ && getNodeTypeById(nodeId_) == NODEB && msg->getArrivalGate()->isName("dcManagerIn")) {
        EV << "LtePdcp::handleMessage - Received packet from DualConnectivityManager" << endl;
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
            EV << "LtePdcp::handleMessage - Received packet " << pkt->getName() << " from port " << pkt->getArrivalGate()->getName() << endl;
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

void LtePdcp::receiveDataFromSourceNode(inet::Packet *pkt, MacNodeId sourceNode)
{
    Enter_Method("receiveDataFromSourceNode");
    take(pkt);

    auto ctrlInfo = pkt->getTag<FlowControlInfo>();
    if (ctrlInfo->getDirection() == DL) {
        MacNodeId destId = ctrlInfo->getDestId();
        EV << NOW << " LtePdcp::receiveDataFromSourceNode - Received PDCP PDU from master node with id " << sourceNode << " - destination node[" << destId << "]" << endl;

        // Look up the bypass TX entity for this DRB on the secondary node.
        // The entity was keyed using the UE's NR node ID (from the NR connection setup),
        // but the X2-forwarded packet may carry the UE's LTE node ID. Normalize if needed.
        DrbKey id = DrbKey(destId, ctrlInfo->getDrbId());
        if (binder_->isGNodeB(nodeId_) != isNrUe(destId)) {
            MacNodeId otherDestId = binder_->getUeNodeId(destId, !isNrUe(destId));
            ASSERT(otherDestId != NODEID_NONE);
            id = DrbKey(otherDestId, ctrlInfo->getDrbId());
        }
        PdcpTxEntityBase *entity = lookupTxEntity(id);
        ASSERT(entity != nullptr);
        entity->handlePacketFromUpperLayer(pkt);
    }
    else { // UL
        EV << NOW << " LtePdcp::receiveDataFromSourceNode - Received PDCP PDU from secondary node with id " << sourceNode << endl;
        fromLowerLayer(pkt);
    }
}

void LtePdcp::pdcpHandleD2DModeSwitch(MacNodeId peerId, LteD2DMode newMode)
{
    EV << NOW << " LtePdcp::pdcpHandleD2DModeSwitch - peering with UE " << peerId << " set to " << d2dModeToA(newMode) << endl;
    // add here specific behavior for handling mode switch at the PDCP layer
}

PdcpTxEntityBase *LtePdcp::lookupTxEntity(DrbKey id)
{
    auto it = txEntities_.find(id);
    return it != txEntities_.end() ? it->second : nullptr;
}

PdcpTxEntityBase *LtePdcp::createTxEntity(DrbKey id)
{
    std::stringstream buf;
    buf << "tx-" << id.getNodeId() << "-" << id.getDrbId();
    auto *module = txEntityModuleType_->create(buf.str().c_str(), this);
    module->par("headerCompressedSize") = par("headerCompressedSize");
    module->finalizeParameters();
    module->buildInside();
    module->scheduleStart(simTime());
    module->callInitialize();
    PdcpTxEntityBase *txEnt = check_and_cast<PdcpTxEntityBase *>(module);
    txEntities_[id] = txEnt;

    EV << "LtePdcp::createTxEntity - Added new TxPdcpEntity for " << id << "\n";

    return txEnt;
}


PdcpRxEntityBase *LtePdcp::lookupRxEntity(DrbKey id)
{
    auto it = rxEntities_.find(id);
    return it != rxEntities_.end() ? it->second : nullptr;
}

PdcpRxEntityBase *LtePdcp::createRxEntity(DrbKey id)
{
    std::stringstream buf;
    buf << "rx-" << id.getNodeId() << "-" << id.getDrbId();
    auto *module = rxEntityModuleType_->create(buf.str().c_str(), this);
    module->par("headerCompressedSize") = par("headerCompressedSize");
    module->finalizeParameters();
    module->buildInside();
    module->scheduleStart(simTime());
    module->callInitialize();
    PdcpRxEntityBase *rxEnt = check_and_cast<PdcpRxEntityBase *>(module);
    rxEntities_[id] = rxEnt;

    EV << "LtePdcp::createRxEntity - Added new RxPdcpEntity for " << id << "\n";

    return rxEnt;
}

PdcpTxEntityBase *LtePdcp::createBypassTxEntity(DrbKey id)
{
    std::stringstream buf;
    buf << "bypass-tx-" << id.getNodeId() << "-" << id.getDrbId();
    auto *module = bypassTxEntityModuleType_->create(buf.str().c_str(), this);
    module->par("headerCompressedSize") = par("headerCompressedSize");
    module->finalizeParameters();
    module->buildInside();
    module->scheduleStart(simTime());
    module->callInitialize();
    PdcpTxEntityBase *txEnt = check_and_cast<PdcpTxEntityBase *>(module);
    txEntities_[id] = txEnt;

    EV << "LtePdcp::createBypassTxEntity - Added new BypassTxPdcpEntity for " << id << "\n";

    return txEnt;
}

PdcpRxEntityBase *LtePdcp::createBypassRxEntity(DrbKey id)
{
    std::stringstream buf;
    buf << "bypass-rx-" << id.getNodeId() << "-" << id.getDrbId();
    auto *module = bypassRxEntityModuleType_->create(buf.str().c_str(), this);
    module->par("headerCompressedSize") = par("headerCompressedSize");
    module->finalizeParameters();
    module->buildInside();
    module->scheduleStart(simTime());
    module->callInitialize();
    PdcpRxEntityBase *rxEnt = check_and_cast<PdcpRxEntityBase *>(module);
    rxEntities_[id] = rxEnt;

    EV << "LtePdcp::createBypassRxEntity - Added new BypassRxPdcpEntity for " << id << "\n";

    return rxEnt;
}

void LtePdcp::deleteEntities(MacNodeId nodeId)
{
    Enter_Method_Silent();

    bool isEnb = (getNodeTypeById(nodeId_) == NODEB);

    if (isEnb || isNR_) {
        // ENB and NrPdcpUe: delete connections related to the given UE only
        // TODO: LtePdcpUe deletes ALL entities — check if that is correct for NR dual connectivity
        for (auto tit = txEntities_.begin(); tit != txEntities_.end(); ) {
            auto& [id, txEntity] = *tit;
            if (id.getNodeId() == nodeId) {
                txEntity->deleteModule();
                tit = txEntities_.erase(tit);
            }
            else {
                ++tit;
            }
        }
        for (auto rit = rxEntities_.begin(); rit != rxEntities_.end(); ) {
            auto& [id, rxEntity] = *rit;
            if (id.getNodeId() == nodeId) {
                rxEntity->deleteModule();
                rit = rxEntities_.erase(rit);
            }
            else {
                ++rit;
            }
        }
    }
    else {
        // LtePdcpUe / LtePdcpUeD2D: delete all connections
        // TODO: check this (for NR dual connectivity)
        for (auto& [txId, txEntity] : txEntities_)
            txEntity->deleteModule();
        txEntities_.clear();

        for (auto& [rxId, rxEntity] : rxEntities_)
            rxEntity->deleteModule();
        rxEntities_.clear();
    }
}

void LtePdcp::activeUeUL(std::set<MacNodeId> *ueSet)
{
    for (const auto& [id, rxEntity] : rxEntities_) {
        MacNodeId nodeId = id.getNodeId();
        if (!(rxEntity->isEmpty()))
            ueSet->insert(nodeId);
    }
}

} //namespace
