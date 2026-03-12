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

#include "NrRlcAm.h"
#include "simu5g/stack/rlc/am/NrAmTxQueue.h"
#include "simu5g/stack/rlc/am/NrAmRxQueue.h"
#include "simu5g/stack/mac/packet/LteMacSduRequest.h"
#include "simu5g/stack/rlc/am/packet/NrRlcAmStatusPdu_m.h"

namespace simu5g {

using namespace omnetpp;
using namespace inet;

Define_Module(NrRlcAm);

NrAmTxQueue *NrRlcAm::getNrTxBuffer(MacNodeId nodeId, LogicalCid lcid)
{
    MacCid cid = MacCid(nodeId, lcid);
    auto it = txBuffers_.find(cid);

    if (it == txBuffers_.end()) {
        std::stringstream buf;
        buf << "NrAmTxQueue Lcid: " << lcid << " cid: " << cid.asPackedInt();
        auto *moduleType = cModuleType::get("simu5g.stack.rlc.am.NrAmTxQueue");
        auto *txbuf = check_and_cast<NrAmTxQueue *>(
                moduleType->createScheduleInit(buf.str().c_str(), getParentModule()));
        txBuffers_[cid] = txbuf;

        EV << NOW << " NrRlcAm::getTxBuffer( Added new NrAmTxBuffer: " << txbuf->getId()
           << " for node: " << nodeId << " for Lcid: " << lcid << "\n";
        return txbuf;
    }
    else {
        EV << NOW << " NrRlcAm::getTxBuffer( Using old NrAmTxBuffer: " << it->second->getId()
           << " for node: " << nodeId << " for Lcid: " << lcid << "\n";
        return it->second;
    }
}
void NrRlcAm::handleUpperMessage(cPacket *pktAux)
{
    emit(receivedPacketFromUpperLayerSignal_, pktAux);
    auto *pkt = check_and_cast<Packet *>(pktAux);
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();

    NrAmTxQueue *txbuf = getNrTxBuffer(ctrlInfoToUeId(lteInfo), lteInfo->getLcid());

    auto rlcPkt = makeShared<LteRlcSdu>();
    rlcPkt->setSnoMainPacket(lteInfo->getSequenceNumber());
    rlcPkt->setChunkLength(B(RLC_HEADER_AM));
    rlcPkt->setLengthMainPacket(pkt->getByteLength());
    pkt->insertAtFront(rlcPkt);
    drop(pkt);

    EV << NOW << " NrRlcAm::handleUpperMessage sending " << pkt
       << " to AM TX Queue sn=" << lteInfo->getSequenceNumber() << endl;
    txbuf->enque(pkt);
}
void NrRlcAm::handleLowerMessage(cPacket *pktAux)
{
    auto *pkt = check_and_cast<Packet *>(pktAux);
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    auto chunk = pkt->peekAtFront<inet::Chunk>();

    if (inet::dynamicPtrCast<const LteMacSduRequest>(chunk) != nullptr) {
        // SDU request from MAC
        NrAmTxQueue *txbuf = getNrTxBuffer(ctrlInfoToUeId(lteInfo), lteInfo->getLcid());
        auto macSduRequest = pkt->peekAtFront<LteMacSduRequest>();
        txbuf->sendPdus(macSduRequest->getSduSize());
        drop(pkt);
        delete pkt;
    }
    else {
        // AM PDU or control PDU from lower layer
        NrAmRxQueue *rxbuf = getNrRxBuffer(ctrlInfoToUeId(lteInfo), lteInfo->getLcid());
        drop(pkt);

        if (inet::dynamicPtrCast<const NrRlcAmStatusPdu>(chunk) != nullptr) {
            EV << NOW << " NrRlcAm::handleLowerMessage sending control PDU "
               << pkt << " to AM TX Queue" << endl;
            rxbuf->handleControlPdu(pkt);
        }
        else {
            emit(receivedPacketFromLowerLayerSignal_, pkt);
            EV << NOW << " NrRlcAm::handleLowerMessage sending packet to AM RX Queue" << endl;
            rxbuf->enque(pkt);
        }
    }
}

NrAmRxQueue *NrRlcAm::getNrRxBuffer(MacNodeId nodeId, LogicalCid lcid)
{
    MacCid cid = MacCid(nodeId, lcid);
    auto it = rxBuffers_.find(cid);

    if (it == rxBuffers_.end()) {
        std::stringstream buf;
        buf << "NrAmRxQueue Lcid: " << lcid << " cid: " << cid.asPackedInt();
        auto *moduleType = cModuleType::get("simu5g.stack.rlc.am.NrAmRxQueue");
        auto *rxbuf = check_and_cast<NrAmRxQueue *>(
                moduleType->createScheduleInit(buf.str().c_str(), getParentModule()));
        rxBuffers_[cid] = rxbuf;
        rxbuf->setRemoteEntity(nodeId);

        EV << NOW << " NrRlcAm::getRxBuffer( Added new AmRxBuffer: " << rxbuf->getId()
           << " for node: " << nodeId << " for Lcid: " << lcid << "\n";
        return rxbuf;
    }
    else {
        EV << NOW << " NrRlcAm::getRxBuffer( Using old AmRxBuffer: " << it->second->getId()
           << " for node: " << nodeId << " for Lcid: " << lcid << "\n";
        return it->second;
    }
}
void NrRlcAm::deleteQueues(MacNodeId nodeId)
{
    for (auto it = txBuffers_.begin(); it != txBuffers_.end(); ) {
        if (it->first.getNodeId() == nodeId) {
            it->second->deleteModule();
            it = txBuffers_.erase(it);
        }
        else {
            ++it;
        }
    }
    for (auto it = rxBuffers_.begin(); it != rxBuffers_.end(); ) {
        if (it->first.getNodeId() == nodeId) {
            it->second->deleteModule();
            it = rxBuffers_.erase(it);
        }
        else {
            ++it;
        }
    }
}

void NrRlcAm::bufferControlPdu(cPacket *pktAux)
{
    auto *pkt = check_and_cast<inet::Packet *>(pktAux);
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    NrAmTxQueue *txbuf = getNrTxBuffer(ctrlInfoToUeId(lteInfo), lteInfo->getLcid());
    txbuf->bufferControlPdu(pkt);
}

void NrRlcAm::routeControlMessage(cPacket *pktAux)
{
    Enter_Method("routeControlMessage");
    auto *pkt = check_and_cast<Packet *>(pktAux);
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    NrAmTxQueue *txbuf = getNrTxBuffer(ctrlInfoToUeId(lteInfo), lteInfo->getLcid());
    // Remove tag before handleControlPacket, which deletes pkt
    lteInfo = pkt->removeTag<FlowControlInfo>();
    txbuf->handleControlPacket(pkt);
}

} //namespace
