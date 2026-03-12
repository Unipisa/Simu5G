//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "NrRlcUm.h"


#include <inet/common/ModuleAccess.h>
#include <inet/common/ProtocolTag_m.h>
#include "simu5g/stack/mac/packet/LteMacSduRequest.h"
namespace simu5g {

Define_Module(NrRlcUm);

using namespace omnetpp;
// statistics
simsignal_t NrRlcUm::receivedPacketFromUpperLayerSignal_ = registerSignal("receivedPacketFromUpperLayer");
simsignal_t NrRlcUm::receivedPacketFromLowerLayerSignal_ = registerSignal("receivedPacketFromLowerLayer");
simsignal_t NrRlcUm::sentPacketToUpperLayerSignal_ = registerSignal("sentPacketToUpperLayer");
simsignal_t NrRlcUm::sentPacketToLowerLayerSignal_ = registerSignal("sentPacketToLowerLayer");

NrUmTxEntity *NrRlcUm::getNrTxBuffer(inet::Ptr<FlowControlInfo> lteInfo)
{
    MacNodeId nodeId = NODEID_NONE;
    LogicalCid lcid = 0;
    if (lteInfo != nullptr) {
        nodeId = ctrlInfoToUeId(lteInfo);

        /**
         * Note: Simu5G currently determines the LCID based
         * on the src/dst IP addresses and TOS field and uses separate
         * RLC entities for each of them. In order to evaluate the effect of using
         * only a single UM LCID and a single bearer, this parameter allows to
         * use only a single UM TxEntity, as it would be with a single bearer.
         *
         * Otherwise (i.e. if mapAllLcidsToSingleBearer_ is false), separate
         * entities are used for each LCID.
         */
        lcid = mapAllLcidsToSingleBearer_ ? 1 : lteInfo->getLcid();
    }
    else
        throw cRuntimeError("NrRlcUm::getTxBuffer - lteInfo is NULL pointer. Aborting.");

    // Find TXBuffer for this CID
    MacCid cid = MacCid(nodeId, lcid);
    NrUmTxEntities::iterator it = nrtxEntities_.find(cid);
    if (it == nrtxEntities_.end()) {
        // Not found: create
        std::stringstream buf;

        buf << "NrUmTxEntity Lcid: " << lcid;
        cModuleType *moduleType = cModuleType::get("simu5g.stack.rlc.um.NrUmTxEntity");
        NrUmTxEntity *txEnt = check_and_cast<NrUmTxEntity *>(moduleType->createScheduleInit(buf.str().c_str(), getParentModule()));
        nrtxEntities_[cid] = txEnt;    // Add to tx_entities map

        if (lteInfo != nullptr) {
            // store control info for this flow
            txEnt->setFlowControlInfo(lteInfo.get());
        }

        EV << "NrRlcUm : Added new NrUmTxEntity: " << txEnt->getId() <<
                " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return txEnt;
    }
    else {
        // Found
        EV << "NrRlcUm : Using old NrUmTxEntity: " << it->second->getId() <<
                " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return it->second;
    }
}

NrUmRxEntity *NrRlcUm::getNrRxBuffer(inet::Ptr<FlowControlInfo> lteInfo)
{
    MacNodeId nodeId;
    if (lteInfo->getDirection() == DL)
        nodeId = lteInfo->getDestId();
    else
        nodeId = lteInfo->getSourceId();
    LogicalCid lcid = lteInfo->getLcid();

    // Find RXBuffer for this CID
    MacCid cid = MacCid(nodeId, lcid);

    NrUmRxEntities::iterator it = nrrxEntities_.find(cid);
    if (it == nrrxEntities_.end()) {
        // Not found: create
        std::stringstream buf;
        buf << "UmRxEntity Lcid: " << lcid << " cid: " << cid.asPackedInt();
        cModuleType *moduleType = cModuleType::get("simu5g.stack.rlc.um.NrUmRxEntity");
        NrUmRxEntity *rxEnt = check_and_cast<NrUmRxEntity *>(
                moduleType->createScheduleInit(buf.str().c_str(), getParentModule()));
        nrrxEntities_[cid] = rxEnt;    // Add to rx_entities map

        // store control info for this flow
        rxEnt->setFlowControlInfo(lteInfo.get());

        EV << "NrRlcUm : Added new NrUmRxEntity: " << rxEnt->getId() <<
                " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return rxEnt;
    }
    else {
        // Found
        EV << "NrRlcUm : Using old NrUmRxEntity: " << it->second->getId() <<
                " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return it->second;
    }
}

void NrRlcUm::sendDefragmented(cPacket *pkt)
{
    Enter_Method_Silent("sendDefragmented()");                    // Direct Method Call
    take(pkt);                                                    // Take ownership

    EV << "NrRlcUm : Sending packet " << pkt->getName() << " to port UM_Sap_up$o\n";
    send(pkt, upOutGate_);

    emit(sentPacketToUpperLayerSignal_, pkt);
}

void NrRlcUm::sendToLowerLayer(cPacket *pktAux)
{
    Enter_Method_Silent("sendToLowerLayer()");                    // Direct Method Call
    take(pktAux);                                                    // Take ownership
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    pkt->addTagIfAbsent<inet::PacketProtocolTag>()->setProtocol(&LteProtocol::rlc);
    EV << "NrRlcUm : Sending packet " << pktAux->getName() << " to port UM_Sap_down$o\n";
    send(pktAux, downOutGate_);
    emit(sentPacketToLowerLayerSignal_, pkt);
}

void NrRlcUm::dropBufferOverflow(cPacket *pktAux)
{
    Enter_Method_Silent("dropBufferOverflow()");                  // Direct Method Call
    take(pktAux);                                                    // Take ownership

    EV << "NrRlcUm : Dropping packet " << pktAux->getName() << " (queue full) \n";
    delete pktAux;
}

void NrRlcUm::handleUpperMessage(cPacket *pktAux)
{
    emit(receivedPacketFromUpperLayerSignal_, pktAux);

    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();

    auto chunk = pkt->peekAtFront<inet::Chunk>();
    EV << "NrRlcUm::handleUpperMessage - Received packet " << chunk->getClassName() << " from upper layer, size " << pktAux->getByteLength() << "\n";

    NrUmTxEntity *txbuf = getNrTxBuffer(lteInfo);

    // Create a new RLC packet
    auto rlcPkt = inet::makeShared<LteRlcSdu>();
    rlcPkt->setSnoMainPacket(lteInfo->getSequenceNumber());
    rlcPkt->setLengthMainPacket(pkt->getByteLength());
    pkt->insertAtFront(rlcPkt);

    drop(pkt);

    if (txbuf->isHoldingDownstreamInPackets()) {
        // do not store in the TX buffer and do not signal the MAC layer
        EV << "NrRlcUm::handleUpperMessage - Enqueue packet " << rlcPkt->getClassName() << " into the Holding Buffer\n";
        txbuf->enqueHoldingPackets(pkt);
    }
    else {
        if (txbuf->enque(pkt)) {
            EV << "NrRlcUm::handleUpperMessage - Enqueue packet " << rlcPkt->getClassName() << " into the Tx Buffer\n";
            // create a message to notify the MAC layer that the queue contains new data
            auto newDataPkt = inet::makeShared<LteRlcPduNewData>();
            // make a copy of the RLC SDU
            auto pktDup = check_and_cast<inet::Packet*>(pkt)->dup();
            pktDup->insertAtFront(newDataPkt);
            // the MAC will only be interested in the size of this packet

            EV << "NrRlcUm::handleUpperMessage - Sending message " << newDataPkt->getClassName() << " to port UM_Sap_down$o\n";
            send(pktDup, downOutGate_);


        }
        else {
            // Queue is full - drop SDU
            dropBufferOverflow(pkt);
        }
    }
}

void NrRlcUm::sendFragmented(cPacket *pkt) {
}
void NrRlcUm::indicateNewDataToMac(cPacket* pktAux) {
    Enter_Method("NrRlcUm::indicateNewDataToMac()");


    auto pkt = check_and_cast<inet::Packet *>(pktAux);

    // create a message to notify the MAC layer that the queue contains new data
    // (MAC is only interested in FlowControlInfo tag and size)

    auto newData = new inet::Packet("UM-NewData");
    auto rlcSdu = inet::makeShared<LteRlcSdu>();
    rlcSdu->setLengthMainPacket(pkt->getByteLength());

    newData->insertAtFront(rlcSdu);

    auto newDataHdr = inet::makeShared<LteRlcPduNewData>();
    newData->insertAtFront(newDataHdr);

    newData->copyTags(*pkt);

    EV << "NrRlcUm::sendNewDataPkt - Sending message " << newData->getName() << " to port AM_Sap_down$o\n";
    send(newData, downOutGate_);

}
void NrRlcUm::handleLowerMessage(cPacket *pktAux)
{
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    EV << "NrRlcUm::handleLowerMessage - Received packet " << pkt->getName() << " from lower layer\n";
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    auto chunk = pkt->peekAtFront<inet::Chunk>();

    if (inet::dynamicPtrCast<const LteMacSduRequest>(chunk) != nullptr) {
        // get the corresponding Tx buffer
        NrUmTxEntity *txbuf = getNrTxBuffer(lteInfo);

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
        NrUmRxEntity *rxbuf = getNrRxBuffer(lteInfo);
        drop(pkt);

        // Bufferize PDU
        EV << "NrRlcUm::handleLowerMessage - Enqueue packet " << pkt->getName() << " into the Rx Buffer\n";
        rxbuf->enque(pkt);
    }
}

void NrRlcUm::deleteQueues(MacNodeId nodeId)
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

void NrRlcUm::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        upInGate_ = gate("UM_Sap_up$i");
        upOutGate_ = gate("UM_Sap_up$o");
        downInGate_ = gate("UM_Sap_down$i");
        downOutGate_ = gate("UM_Sap_down$o");

        // parameters
        mapAllLcidsToSingleBearer_ = par("mapAllLcidsToSingleBearer");
        std::string nodeTypePar = par("nodeType").stdstringValue();
        nodeType = static_cast<RanNodeType>(cEnum::get("simu5g::RanNodeType")->lookup(nodeTypePar.c_str()));

        WATCH_MAP(txEntities_);
        WATCH_MAP(rxEntities_);
    }
}

void NrRlcUm::handleMessage(cMessage *msg)
{
    cPacket *pkt = check_and_cast<cPacket *>(msg);
    EV << "NrRlcUm : Received packet " << pkt->getName() << " from port " << pkt->getArrivalGate()->getName() << endl;

    cGate *incoming = pkt->getArrivalGate();
    if (incoming == upInGate_) {
        handleUpperMessage(pkt);
    }
    else if (incoming == downInGate_) {
        handleLowerMessage(pkt);
    }
}

void NrRlcUm::activeUeUL(std::set<MacNodeId> *ueSet)
{
    for (const auto& [macCid, entity] : rxEntities_) {
        MacNodeId nodeId = macCid.getNodeId();
        if ((ueSet->find(nodeId) == ueSet->end()) && !entity->isEmpty())
            ueSet->insert(nodeId);
    }
}

void NrRlcUm::addUeThroughput(MacNodeId nodeId, Throughput throughput)
{
    ULThroughputPerUE::iterator it = ulThroughput_.find(nodeId);
    if (it == ulThroughput_.end()) {
        ulThroughput_[nodeId] = throughput;
    }
    else {
        it->second.pktSizeCount += throughput.pktSizeCount;
        it->second.time += throughput.time;
    }
}

double NrRlcUm::getUeThroughput(MacNodeId nodeId)
{
    ULThroughputPerUE::iterator it = ulThroughput_.find(nodeId);
    if (it == ulThroughput_.end()) {
        return 0;
    }
    else {
        return it->second.pktSizeCount / (it->second.time).dbl();
    }
}

void NrRlcUm::resetThroughputStats(MacNodeId nodeId)
{
    ULThroughputPerUE::iterator it = ulThroughput_.find(nodeId);
    if (it == ulThroughput_.end()) {
        return;
    }
    else {
        it->second.pktSizeCount = 0;
        it->second.time = 0;
    }
}


} //namespace
