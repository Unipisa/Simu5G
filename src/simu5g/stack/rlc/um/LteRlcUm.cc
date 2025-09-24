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

#include <inet/common/ModuleAccess.h>
#include <inet/common/ProtocolTag_m.h>

#include "simu5g/stack/rlc/um/LteRlcUm.h"
#include "simu5g/stack/mac/packet/LteMacSduRequest.h"

namespace simu5g {

Define_Module(LteRlcUm);

using namespace omnetpp;

// statistics
simsignal_t LteRlcUm::receivedPacketFromUpperLayerSignal_ = registerSignal("receivedPacketFromUpperLayer");
simsignal_t LteRlcUm::receivedPacketFromLowerLayerSignal_ = registerSignal("receivedPacketFromLowerLayer");
simsignal_t LteRlcUm::sentPacketToUpperLayerSignal_ = registerSignal("sentPacketToUpperLayer");
simsignal_t LteRlcUm::sentPacketToLowerLayerSignal_ = registerSignal("sentPacketToLowerLayer");

UmTxEntity *LteRlcUm::lookupTxBuffer(MacCid cid)
{
    UmTxEntities::iterator it = txEntities_.find(cid);
    return (it != txEntities_.end()) ? it->second : nullptr;
}

UmTxEntity *LteRlcUm::createTxBuffer(MacCid cid, FlowControlInfo *lteInfo)
{
    if (txEntities_.find(cid) != txEntities_.end())
        throw cRuntimeError("RLC-UM connection TX entity for %s already exists", cid.str().c_str());

    std::stringstream buf;
    buf << "UmTxEntity Lcid: " << cid.getLcid() << " cid: " << cid.asPackedInt();
    UmTxEntity *txEnt = check_and_cast<UmTxEntity *>(txEntityModuleType_->createScheduleInit(buf.str().c_str(), getParentModule()));
    txEntities_[cid] = txEnt;

    txEnt->setFlowControlInfo(lteInfo);

    EV << "LteRlcUm::createTxBuffer - Added new UmTxEntity: " << txEnt->getId() << " for CID " << cid << "\n";

    return txEnt;
}


UmRxEntity *LteRlcUm::lookupRxBuffer(MacCid cid)
{
    UmRxEntities::iterator it = rxEntities_.find(cid);
    return (it != rxEntities_.end()) ? it->second : nullptr;
}

UmRxEntity *LteRlcUm::createRxBuffer(MacCid cid, FlowControlInfo *lteInfo)
{
    if (rxEntities_.find(cid) != rxEntities_.end())
        throw cRuntimeError("RLC-UM connection RX entity for %s already exists", cid.str().c_str());

    std::stringstream buf;
    buf << "UmRxEntity Lcid: " << cid.getLcid() << " cid: " << cid.asPackedInt();
    UmRxEntity *rxEnt = check_and_cast<UmRxEntity *>(rxEntityModuleType_->createScheduleInit(buf.str().c_str(), getParentModule()));
    rxEntities_[cid] = rxEnt;

    // configure entity
    rxEnt->setFlowControlInfo(lteInfo);

    EV << "LteRlcUm::createRxBuffer - Added new UmRxEntity: " << rxEnt->getId() << " for CID " << cid << "\n";

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

void LteRlcUm::handleUpperMessage(cPacket *pktAux)
{
    emit(receivedPacketFromUpperLayerSignal_, pktAux);

    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();

    auto chunk = pkt->peekAtFront<inet::Chunk>();
    EV << "LteRlcUm::handleUpperMessage - Received packet " << chunk->getClassName() << " from upper layer, size " << pktAux->getByteLength() << "\n";

    MacCid cid = ctrlInfoToMacCid(lteInfo.get());
    UmTxEntity *txbuf = lookupTxBuffer(cid);
    if (txbuf == nullptr)
        txbuf = createTxBuffer(cid, lteInfo.get());

    // Create a new RLC packet
    auto rlcPkt = inet::makeShared<LteRlcSdu>();
    rlcPkt->setSnoMainPacket(lteInfo->getSequenceNumber());
    rlcPkt->setLengthMainPacket(pkt->getByteLength());
    pkt->insertAtFront(rlcPkt);

    drop(pkt);

    if (txbuf->isHoldingDownstreamInPackets()) {
        // do not store in the TX buffer and do not signal the MAC layer
        EV << "LteRlcUm::handleUpperMessage - Enqueue packet " << rlcPkt->getClassName() << " into the Holding Buffer\n";
        txbuf->enqueHoldingPackets(pkt);
    }
    else {
        if (txbuf->enque(pkt)) {
            EV << "LteRlcUm::handleUpperMessage - Enqueue packet " << rlcPkt->getClassName() << " into the Tx Buffer\n";

            // create a message to notify the MAC layer that the queue contains new data
            auto newDataPkt = inet::makeShared<LteRlcPduNewData>();
            // make a copy of the RLC SDU
            auto pktDup = pkt->dup();
            pktDup->insertAtFront(newDataPkt);
            // the MAC will only be interested in the size of this packet

            EV << "LteRlcUm::handleUpperMessage - Sending message " << newDataPkt->getClassName() << " to port UM_Sap_down$o\n";
            send(pktDup, downOutGate_);
        }
        else {
            // Queue is full - drop SDU
            txbuf->dropBufferOverflow(pkt);
        }
    }
}

void LteRlcUm::handleLowerMessage(cPacket *pktAux)
{
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    EV << "LteRlcUm::handleLowerMessage - Received packet " << pkt->getName() << " from lower layer\n";
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    auto chunk = pkt->peekAtFront<inet::Chunk>();

    if (inet::dynamicPtrCast<const LteMacSduRequest>(chunk) != nullptr) {
        // get the corresponding Tx buffer
        MacCid cid = ctrlInfoToMacCid(lteInfo.get());
        UmTxEntity *txbuf = lookupTxBuffer(cid);
        if (txbuf == nullptr)
            txbuf = createTxBuffer(cid, lteInfo.get());

        auto macSduRequest = pkt->peekAtFront<LteMacSduRequest>();
        unsigned int size = macSduRequest->getSduSize();

        drop(pkt);

        // do segmentation/concatenation and send a pdu to the lower layer
        txbuf->rlcPduMake(size);

        delete pkt;
    }
    else {
        emit(receivedPacketFromLowerLayerSignal_, pkt);

        // Extract information from fragment
        MacNodeId nodeId = (lteInfo->getDirection() == DL) ? lteInfo->getDestId() : lteInfo->getSourceId();
        MacCid cid = MacCid(nodeId, lteInfo->getLcid());
        UmRxEntity *rxbuf = lookupRxBuffer(cid);
        if (rxbuf == nullptr)
            rxbuf = createRxBuffer(cid, lteInfo.get());
        drop(pkt);

        // Bufferize PDU
        EV << "LteRlcUm::handleLowerMessage - Enqueue packet " << pkt->getName() << " into the Rx Buffer\n";
        rxbuf->enque(pkt);
    }
}

void LteRlcUm::deleteQueues(MacNodeId nodeId)
{
    Enter_Method_Silent();

    // at the UE, delete all connections
    // at the eNB, delete connections related to the given UE
    for (auto tit = txEntities_.begin(); tit != txEntities_.end();) {
        if (nodeType == UE || ((nodeType == ENODEB || nodeType == GNODEB) && tit->first.getNodeId() == nodeId)) {
            tit->second->deleteModule(); // Delete Entity
            tit = txEntities_.erase(tit);    // Delete Element
        }
        else {
            ++tit;
        }
    }
    for (auto rit = rxEntities_.begin(); rit != rxEntities_.end();) {
        if (nodeType == UE || ((nodeType == ENODEB || nodeType == GNODEB) && rit->first.getNodeId() == nodeId)) {
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
        txEntityModuleType_ = cModuleType::get(par("txEntityModuleType").stringValue());
        rxEntityModuleType_ = cModuleType::get(par("rxEntityModuleType").stringValue());

        std::string nodeTypePar = par("nodeType").stdstringValue();
        nodeType = static_cast<RanNodeType>(cEnum::get("simu5g::RanNodeType")->lookup(nodeTypePar.c_str()));

        WATCH_MAP(txEntities_);
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

void LteRlcUm::activeUeUL(std::set<MacNodeId> *ueSet)
{
    for (const auto& [macCid, entity] : rxEntities_) {
        MacNodeId nodeId = macCid.getNodeId();
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
