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

#include <inet/common/ModuleAccess.h>
#include <inet/common/ProtocolTag_m.h>

#include "simu5g/stack/rlc/um/LteRlcUm.h"
#include "simu5g/stack/rlc/RlcUpperMux.h"
#include "simu5g/stack/d2dModeSelection/D2DModeSwitchNotification_m.h"
#include "simu5g/stack/mac/packet/LteMacSduRequest.h"
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"

namespace simu5g {

Define_Module(LteRlcUm);

using namespace omnetpp;

// statistics
simsignal_t LteRlcUm::receivedPacketFromUpperLayerSignal_ = registerSignal("receivedPacketFromUpperLayer");
simsignal_t LteRlcUm::receivedPacketFromLowerLayerSignal_ = registerSignal("receivedPacketFromLowerLayer");
simsignal_t LteRlcUm::sentPacketToUpperLayerSignal_ = registerSignal("sentPacketToUpperLayer");
simsignal_t LteRlcUm::sentPacketToLowerLayerSignal_ = registerSignal("sentPacketToLowerLayer");

UmTxEntity *LteRlcUm::lookupTxBuffer(DrbKey id)
{
    return upperMux_->lookupTxBuffer(id);
}

UmTxEntity *LteRlcUm::createTxBuffer(DrbKey id, FlowControlInfo *lteInfo)
{
    return upperMux_->createTxBuffer(id, lteInfo);
}


UmRxEntity *LteRlcUm::lookupRxBuffer(DrbKey id)
{
    UmRxEntities::iterator it = rxEntities_.find(id);
    return (it != rxEntities_.end()) ? it->second : nullptr;
}

UmRxEntity *LteRlcUm::createRxBuffer(DrbKey id, FlowControlInfo *lteInfo)
{
    if (rxEntities_.find(id) != rxEntities_.end())
        throw cRuntimeError("RLC-UM connection RX entity for %s already exists", id.str().c_str());

    std::stringstream buf;
    buf << "UmRxEntity Lcid: " << id.getDrbId() << " cid: " << id.asPackedInt();
    UmRxEntity *rxEnt = check_and_cast<UmRxEntity *>(rxEntityModuleType_->createScheduleInit(buf.str().c_str(), getParentModule()));
    rxEntities_[id] = rxEnt;

    // configure entity
    rxEnt->setFlowControlInfo(lteInfo);

    EV << "LteRlcUm::createRxBuffer - Added new UmRxEntity: " << rxEnt->getId() << " for " << id << "\n";

    return rxEnt;
}


void LteRlcUm::sendToUpperLayer(cPacket *pkt)
{
    Enter_Method_Silent("sendDefragmented()");                    // Direct Method Call
    take(pkt);                                                    // Take ownership

    EV << "LteRlcUm : Sending packet " << pkt->getName() << " to port UM_Sap_up$o\n";
    send(pkt, upOutGate_);

    emit(sentPacketToUpperLayerSignal_, pkt);
}

void LteRlcUm::sendToLowerLayer(cPacket *pktAux)
{
    Enter_Method_Silent("sendToLowerLayer()");                    // Direct Method Call
    take(pktAux);                                                    // Take ownership
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    pkt->addTagIfAbsent<inet::PacketProtocolTag>()->setProtocol(&LteProtocol::rlc);
    EV << "LteRlcUm : Sending packet " << pktAux->getName() << " to port UM_Sap_down$o\n";
    send(pktAux, downOutGate_);
    emit(sentPacketToLowerLayerSignal_, pkt);
}

void LteRlcUm::sendNewDataIndication(cPacket *pktAux)
{
    Enter_Method_Silent("sendNewDataIndication()");
    take(pktAux);
    EV << "LteRlcUm : Sending new data indication to port UM_Sap_down$o\n";
    send(pktAux, downOutGate_);
}

void LteRlcUm::handleUpperMessage(cPacket *pktAux)
{
    emit(receivedPacketFromUpperLayerSignal_, pktAux);

    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();

    EV << "LteRlcUm::handleUpperMessage - Received packet from upper layer, size " << pktAux->getByteLength() << "\n";

    DrbKey id = ctrlInfoToTxDrbKey(lteInfo.get());
    UmTxEntity *txbuf = lookupTxBuffer(id);
    ASSERT(txbuf != nullptr);

    drop(pkt);
    txbuf->handleSdu(pkt);
}

void LteRlcUm::handleLowerMessage(cPacket *pktAux)
{
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    EV << "LteRlcUm::handleLowerMessage - Received packet " << pkt->getName() << " from lower layer\n";
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    auto chunk = pkt->peekAtFront<inet::Chunk>();

    ASSERT(pkt->findTag<PdcpTrackingTag>() == nullptr);

    // D2D: handle mode switch notification
    if (hasD2DSupport_ && inet::dynamicPtrCast<const D2DModeSwitchNotification>(chunk) != nullptr) {
        auto switchPkt = pkt->peekAtFront<D2DModeSwitchNotification>();

        // add here specific behavior for handling mode switch at the RLC layer

        if (switchPkt->getTxSide()) {
            // get the corresponding Tx buffer & call handler
            DrbKey id = ctrlInfoToTxDrbKey(lteInfo.get());
            UmTxEntity *txbuf = lookupTxBuffer(id);
            if (txbuf == nullptr)
                txbuf = createTxBuffer(id, lteInfo.get());
            txbuf->rlcHandleD2DModeSwitch(switchPkt->getOldConnection(), switchPkt->getClearRlcBuffer());

            // forward packet to PDCP
            EV << "LteRlcUm::handleLowerMessage - Sending packet " << pkt->getName() << " to port UM_Sap_up$o\n";
            send(pkt, upOutGate_);
        }
        else { // rx side
            // get the corresponding Rx buffer & call handler
            DrbKey id = ctrlInfoToRxDrbKey(lteInfo.get());
            UmRxEntity *rxbuf = lookupRxBuffer(id);
            if (rxbuf == nullptr)
                rxbuf = createRxBuffer(id, lteInfo.get());
            rxbuf->rlcHandleD2DModeSwitch(switchPkt->getOldConnection(), switchPkt->getOldMode(), switchPkt->getClearRlcBuffer());

            delete pkt;
        }
        return;
    }

    if (inet::dynamicPtrCast<const LteMacSduRequest>(chunk) != nullptr) {
        // get the corresponding Tx buffer
        DrbKey id = ctrlInfoToTxDrbKey(lteInfo.get());
        UmTxEntity *txbuf = lookupTxBuffer(id);
        ASSERT(txbuf != nullptr);

        drop(pkt);
        txbuf->handleMacSduRequest(pkt);
    }
    else {
        emit(receivedPacketFromLowerLayerSignal_, pkt);

        // Extract information from fragment
        DrbKey id = ctrlInfoToRxDrbKey(lteInfo.get());
        UmRxEntity *rxbuf = lookupRxBuffer(id);
        ASSERT(rxbuf != nullptr);
        drop(pkt);

        // Bufferize PDU
        EV << "LteRlcUm::handleLowerMessage - Enqueue packet " << pkt->getName() << " into the Rx Buffer\n";
        rxbuf->enque(pkt);
    }
}

void LteRlcUm::deleteQueues(MacNodeId nodeId)
{
    Enter_Method_Silent();

    // delegate TX entity deletion to UpperMux
    upperMux_->deleteTxEntities(nodeId);

    // RX entity deletion remains here for now
    for (auto rit = rxEntities_.begin(); rit != rxEntities_.end();) {
        // D2D: if the entity refers to a D2D_MULTI connection, do not erase it
        if (hasD2DSupport_ && rit->second->isD2DMultiConnection()) {
            ++rit;
            continue;
        }

        if (nodeType == UE || (nodeType == NODEB && rit->first.getNodeId() == nodeId)) {
            rit->second->deleteModule(); // Delete Entity
            rit = rxEntities_.erase(rit);    // Delete Element
        }
        else {
            ++rit;
        }
    }
}

/*
 * Main functions
 */

void LteRlcUm::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        upInGate_ = gate("UM_Sap_up$i");
        upOutGate_ = gate("UM_Sap_up$o");
        downInGate_ = gate("UM_Sap_down$i");
        downOutGate_ = gate("UM_Sap_down$o");

        // parameters
        hasD2DSupport_ = par("hasD2DSupport").boolValue();
        rxEntityModuleType_ = cModuleType::get(par("rxEntityModuleType").stringValue());

        std::string nodeTypeStr = par("nodeType").stdstringValue();
        nodeType = aToNodeType(nodeTypeStr);

        upperMux_ = check_and_cast<RlcUpperMux *>(getParentModule()->getSubmodule("upperMux"));

        WATCH_MAP(rxEntities_);
    }
}

void LteRlcUm::handleMessage(cMessage *msg)
{
    cPacket *pkt = check_and_cast<cPacket *>(msg);
    EV << "LteRlcUm : Received packet " << pkt->getName() << " from port " << pkt->getArrivalGate()->getName() << endl;

    cGate *incoming = pkt->getArrivalGate();
    if (incoming == upInGate_) {
        handleUpperMessage(pkt);
    }
    else if (incoming == downInGate_) {
        handleLowerMessage(pkt);
    }
}

void LteRlcUm::resumeDownstreamInPackets(MacNodeId peerId)
{
    upperMux_->resumeDownstreamInPackets(peerId);
}

bool LteRlcUm::isEmptyingTxBuffer(MacNodeId peerId)
{
    return upperMux_->isEmptyingTxBuffer(peerId);
}

void LteRlcUm::activeUeUL(std::set<MacNodeId> *ueSet)
{
    for (const auto& [id, entity] : rxEntities_) {
        MacNodeId nodeId = id.getNodeId();
        if ((ueSet->find(nodeId) == ueSet->end()) && !entity->isEmpty())
            ueSet->insert(nodeId);
    }
}

void LteRlcUm::addUeThroughput(MacNodeId nodeId, Throughput throughput)
{
    auto& nodeUlThroughput = ulThroughput_[nodeId];
    nodeUlThroughput.pktSizeCount += throughput.pktSizeCount;
    nodeUlThroughput.time += throughput.time;
}

double LteRlcUm::getUeThroughput(MacNodeId nodeId)
{
    ULThroughputPerUE::iterator it = ulThroughput_.find(nodeId);
    return it == ulThroughput_.end() ? 0 : it->second.pktSizeCount / it->second.time.dbl();
}

void LteRlcUm::resetThroughputStats(MacNodeId nodeId)
{
    auto& nodeUlThroughput = ulThroughput_[nodeId];
    nodeUlThroughput.pktSizeCount = 0;
    nodeUlThroughput.time = 0;
}

} //namespace
