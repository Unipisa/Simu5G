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

#include "simu5g/stack/pdcp/PdcpMux.h"

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

Define_Module(PdcpMux);

using namespace omnetpp;
using namespace inet;


PdcpMux::~PdcpMux()
{
}

/*
 * Upper Layer handlers
 */

void PdcpMux::fromDataPort(cPacket *pktAux)
{
    auto pkt = check_and_cast<inet::Packet *>(pktAux);

    auto lteInfo = pkt->getTag<FlowControlInfo>();
    verifyControlInfo(lteInfo.get());

    DrbKey id = DrbKey(lteInfo->getDestId(), lteInfo->getDrbId());
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

void PdcpMux::fromLowerLayer(cPacket *pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);

    // NrPdcpEnb: trim packet for NR gNBs (removes trailing RLC/MAC padding)
    if (isNR_ && getNodeTypeById(nodeId_) == NODEB) {
        pkt->trim();
    }

    ASSERT(pkt->findTag<PdcpTrackingTag>() == nullptr);

    auto lteInfo = pkt->getTag<FlowControlInfo>();
    DrbKey id = DrbKey(lteInfo->getSourceId(), lteInfo->getDrbId());

    // UE with DC: both RLC legs (LTE from eNB, NR from gNB) must reach the same
    // reordering entity, which is keyed by the serving (master) base station ID.
    if (isDualConnectivityEnabled() && getNodeTypeById(nodeId_) == UE
            && lteInfo->getMulticastGroupId() == NODEID_NONE) {
        id = DrbKey(binder_->getServingNode(nodeId_), lteInfo->getDrbId());
    }

    PdcpRxEntityBase *entity = lookupRxEntity(id);

    EV << "fromLowerLayer in " << getFullPath() << " event #" << getSimulation()->getEventNumber()
       <<  ": Processing packet " << pkt->getName() << " src=" << lteInfo->getSourceId() << " dest=" << lteInfo->getDestId()
       << " multicast=" << lteInfo->getMulticastGroupId() << " direction=" << dirToA((Direction)lteInfo->getDirection())
       << " ---> " << id << (entity == nullptr ? " (NEW)" : " (existing)") << std::endl;

    ASSERT(entity != nullptr);

    entity->handlePacketFromLowerLayer(pkt);
}

void PdcpMux::sendToUpperLayer(Packet *pkt)
{
    Enter_Method_Silent("sendToUpperLayer");
    take(pkt);

    EV << "LtePdcp : Sending packet " << pkt->getName() << " on port upperLayerOut\n";

    send(pkt, upperLayerOutGate_);
}

void PdcpMux::sendToRlc(Packet *pkt)
{
    Enter_Method_Silent("sendToRlc");
    take(pkt);

    EV << "LtePdcp : Sending packet " << pkt->getName() << " on port " << rlcOutGate_->getFullName() << endl;

    send(pkt, rlcOutGate_);
}

void PdcpMux::sendToNrRlc(Packet *pkt)
{
    Enter_Method_Silent("sendToNrRlc");
    take(pkt);

    EV << "LtePdcp : Sending packet " << pkt->getName() << " on port " << nrRlcOutGate_->getFullName() << endl;

    send(pkt, nrRlcOutGate_);
}

void PdcpMux::sendToX2(Packet *pkt)
{
    Enter_Method_Silent("sendToX2");
    take(pkt);

    EV << NOW << " PdcpMux::sendToX2 - Send PDCP packet via X2" << endl;

    send(pkt, "dcManagerOut");
}

/*
 * Main functions
 */

void PdcpMux::initialize(int stage)
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

void PdcpMux::handleMessage(cMessage *msg)
{
    cPacket *pkt = check_and_cast<cPacket *>(msg);
    EV << "LtePdcp : Received packet " << pkt->getName() << " from port "
       << pkt->getArrivalGate()->getName() << endl;

    // NrPdcpEnb: incoming data from DualConnectivityManager via X2
    if (isNR_ && getNodeTypeById(nodeId_) == NODEB && msg->getArrivalGate()->isName("dcManagerIn")) {
        EV << "PdcpMux::handleMessage - Received packet from DualConnectivityManager" << endl;
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
            EV << "PdcpMux::handleMessage - Received packet " << pkt->getName() << " from port " << pkt->getArrivalGate()->getName() << endl;
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

void PdcpMux::receiveDataFromSourceNode(inet::Packet *pkt, MacNodeId sourceNode)
{
    Enter_Method("receiveDataFromSourceNode");
    take(pkt);

    auto ctrlInfo = pkt->getTag<FlowControlInfo>();
    if (ctrlInfo->getDirection() == DL) {
        MacNodeId destId = ctrlInfo->getDestId();
        EV << NOW << " PdcpMux::receiveDataFromSourceNode - Received PDCP PDU from master node with id " << sourceNode << " - destination node[" << destId << "]" << endl;

        DrbKey id = DrbKey(destId, ctrlInfo->getDrbId());
        PdcpTxEntityBase *entity = lookupTxEntity(id);
        ASSERT(entity != nullptr);
        entity->handlePacketFromUpperLayer(pkt);
    }
    else { // UL
        EV << NOW << " PdcpMux::receiveDataFromSourceNode - Received PDCP PDU from secondary node with id " << sourceNode << endl;
        fromLowerLayer(pkt);
    }
}

void PdcpMux::pdcpHandleD2DModeSwitch(MacNodeId peerId, LteD2DMode newMode)
{
    EV << NOW << " PdcpMux::pdcpHandleD2DModeSwitch - peering with UE " << peerId << " set to " << d2dModeToA(newMode) << endl;
    // add here specific behavior for handling mode switch at the PDCP layer
}

PdcpTxEntityBase *PdcpMux::lookupTxEntity(DrbKey id)
{
    auto it = txEntities_.find(id);
    return it != txEntities_.end() ? it->second : nullptr;
}

PdcpTxEntityBase *PdcpMux::createTxEntity(DrbKey id)
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

    EV << "PdcpMux::createTxEntity - Added new TxPdcpEntity for " << id << "\n";

    return txEnt;
}


PdcpRxEntityBase *PdcpMux::lookupRxEntity(DrbKey id)
{
    auto it = rxEntities_.find(id);
    return it != rxEntities_.end() ? it->second : nullptr;
}

PdcpRxEntityBase *PdcpMux::createRxEntity(DrbKey id)
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

    EV << "PdcpMux::createRxEntity - Added new RxPdcpEntity for " << id << "\n";

    return rxEnt;
}

PdcpTxEntityBase *PdcpMux::createBypassTxEntity(DrbKey id)
{
    std::stringstream buf;
    buf << "bypass-tx-" << id.getNodeId() << "-" << id.getDrbId();
    auto *module = bypassTxEntityModuleType_->create(buf.str().c_str(), this);
    module->finalizeParameters();
    module->buildInside();
    module->scheduleStart(simTime());
    module->callInitialize();
    PdcpTxEntityBase *txEnt = check_and_cast<PdcpTxEntityBase *>(module);
    txEntities_[id] = txEnt;

    EV << "PdcpMux::createBypassTxEntity - Added new BypassTxPdcpEntity for " << id << "\n";

    return txEnt;
}

PdcpRxEntityBase *PdcpMux::createBypassRxEntity(DrbKey id)
{
    std::stringstream buf;
    buf << "bypass-rx-" << id.getNodeId() << "-" << id.getDrbId();
    auto *module = bypassRxEntityModuleType_->create(buf.str().c_str(), this);
    module->finalizeParameters();
    module->buildInside();
    module->scheduleStart(simTime());
    module->callInitialize();
    PdcpRxEntityBase *rxEnt = check_and_cast<PdcpRxEntityBase *>(module);
    rxEntities_[id] = rxEnt;

    EV << "PdcpMux::createBypassRxEntity - Added new BypassRxPdcpEntity for " << id << "\n";

    return rxEnt;
}

void PdcpMux::deleteEntities(MacNodeId nodeId)
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

void PdcpMux::activeUeUL(std::set<MacNodeId> *ueSet)
{
    for (const auto& [id, rxEntity] : rxEntities_) {
        MacNodeId nodeId = id.getNodeId();
        if (!(rxEntity->isEmpty()))
            ueSet->insert(nodeId);
    }
}

} //namespace
