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

#include <inet/common/ProtocolTag_m.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/stack/rlc/am/LteRlcAm.h"
#include "simu5g/stack/rlc/am/AmTxQueue.h"
#include "simu5g/stack/rlc/am/AmRxQueue.h"
#include "simu5g/stack/mac/packet/LteMacSduRequest.h"

namespace simu5g {

Define_Module(LteRlcAm);

using namespace omnetpp;

AmTxQueue *LteRlcAm::lookupTxBuffer(MacCid cid)
{
    auto it = txBuffers_.find(cid);
    return (it != txBuffers_.end()) ? it->second : nullptr;
}

AmTxQueue *LteRlcAm::createTxBuffer(MacCid cid)
{
    std::stringstream buf;
    buf << "AmTxQueue Lcid: " << cid.getLcid() << " cid: " << cid.asPackedInt();

    AmTxQueue *txbuf = check_and_cast<AmTxQueue *>(txEntityModuleType_->createScheduleInit(buf.str().c_str(), getParentModule()));
    txBuffers_[cid] = txbuf;

    EV << "LteRlcAm::createTxBuffer - Added new AmTxBuffer: " << txbuf->getId() << " for CID " << cid << "\n";

    return txbuf;
}

AmRxQueue *LteRlcAm::lookupRxBuffer(MacCid cid)
{
    auto it = rxBuffers_.find(cid);
    return (it != rxBuffers_.end()) ? it->second : nullptr;
}

AmRxQueue *LteRlcAm::createRxBuffer(MacCid cid)
{
    std::stringstream buf;
    buf << "AmRxQueue Lcid: " << cid.getLcid() << " cid: " << cid.asPackedInt();

    AmRxQueue *rxbuf = check_and_cast<AmRxQueue *>(rxEntityModuleType_->createScheduleInit(buf.str().c_str(), getParentModule()));
    rxBuffers_[cid] = rxbuf;

    EV << "LteRlcAm::createRxBuffer - Added new AmRxBuffer: " << rxbuf->getId() << " for CID " << cid << "\n";

    return rxbuf;
}


void LteRlcAm::sendDefragmented(cPacket *pktAux)
{
    Enter_Method("sendDefragmented()"); // Direct Method Call
    take(pktAux); // Take ownership
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    pkt->addTagIfAbsent<inet::PacketProtocolTag>()->setProtocol(&LteProtocol::pdcp);

    EV << NOW << " LteRlcAm : Sending packet " << pkt->getName()
       << " to port AM_Sap_up$o\n";
    send(pkt, upOutGate_);
}

void LteRlcAm::bufferControlPdu(cPacket *pktAux) {
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    MacCid cid = ctrlInfoToMacCid(lteInfo.get());

    // Find TXBuffer for this CID
    AmTxQueue *txbuf = lookupTxBuffer(cid);
    if (txbuf == nullptr)
        txbuf = createTxBuffer(cid);

    txbuf->bufferControlPdu(pkt);
}

void LteRlcAm::sendFragmented(cPacket *pktAux)
{
    Enter_Method("sendFragmented()"); // Direct Method Call
    take(pktAux); // Take ownership
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    pkt->addTagIfAbsent<inet::PacketProtocolTag>()->setProtocol(&LteProtocol::rlc);

    EV << NOW << " LteRlcAm : Sending packet " << pkt->getName() << " of size "
       << pkt->getByteLength() << "  to port AM_Sap_down$o\n";

    send(pkt, downOutGate_);
}

void LteRlcAm::handleUpperMessage(cPacket *pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    MacCid cid = ctrlInfoToMacCid(lteInfo.get());

    // Find TXBuffer for this CID
    AmTxQueue *txbuf = lookupTxBuffer(cid);
    if (txbuf == nullptr)
        txbuf = createTxBuffer(cid);

    // Create a new RLC packet
    auto rlcPkt = makeShared<LteRlcAmSdu>();
    rlcPkt->setSnoMainPacket(lteInfo->getSequenceNumber());
    rlcPkt->setChunkLength(B(RLC_HEADER_AM));
    pkt->insertAtFront(rlcPkt);
    drop(pkt);
    EV << NOW << " LteRlcAm : handleUpperMessage sending to AM TX Queue" << endl;
    // Fragment Packet
    txbuf->enque(pkt);
}

void LteRlcAm::routeControlMessage(cPacket *pktAux)
{
    Enter_Method("routeControlMessage");

    auto pkt = check_and_cast<Packet *>(pktAux);
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    MacCid cid = ctrlInfoToMacCid(lteInfo.get());

    // Find TXBuffer for this CID
    AmTxQueue *txbuf = lookupTxBuffer(cid);
    if (txbuf == nullptr)
        txbuf = createTxBuffer(cid);

    txbuf->handleControlPacket(pkt);
    lteInfo = pkt->removeTag<FlowControlInfo>();
}

void LteRlcAm::handleLowerMessage(cPacket *pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    auto chunk = pkt->peekAtFront<inet::Chunk>();

    if (inet::dynamicPtrCast<const LteMacSduRequest>(chunk) != nullptr) {
        // process SDU request received from MAC

        // get the corresponding Tx buffer
        MacCid cid = ctrlInfoToMacCid(lteInfo.get());

        // Find TXBuffer for this CID
        AmTxQueue *txbuf = lookupTxBuffer(cid);
        if (txbuf == nullptr)
            txbuf = createTxBuffer(cid);

        auto macSduRequest = pkt->peekAtFront<LteMacSduRequest>();
        unsigned int size = macSduRequest->getSduSize();

        txbuf->sendPdus(size);

        drop(pkt);

        delete pkt;
    }
    else {
        // process AM PDU
        auto pdu = pkt->peekAtFront<LteRlcAmPdu>();
        if ((pdu->getAmType() == ACK) || (pdu->getAmType() == MRW_ACK)) {
            EV << NOW << " LteRlcAm::handleLowerMessage Received ACK message" << endl;

            // forwarding ACK to associated transmitting entity
            routeControlMessage(pkt);
            return;
        }

        // Extract information from fragment
        MacCid cid = ctrlInfoToMacCid(lteInfo.get());
        // Find RXBuffer for this CID
        AmRxQueue *rxbuf = lookupRxBuffer(cid);
        if (rxbuf == nullptr)
            rxbuf = createRxBuffer(cid);
        drop(pkt);

        EV << NOW << " LteRlcAm::handleLowerMessage sending packet to AM RX Queue " << endl;
        // Defragment packet
        rxbuf->enque(pkt);
    }
}

void LteRlcAm::deleteQueues(MacNodeId nodeId)
{
    for (auto tit = txBuffers_.begin(); tit != txBuffers_.end(); ) {
        if (tit->first.getNodeId() == nodeId) {
            delete tit->second; // Delete Queue
            tit = txBuffers_.erase(tit); // Delete Element
        }
        else {
            ++tit;
        }
    }
    for (auto rit = rxBuffers_.begin(); rit != rxBuffers_.end(); ) {
        if (rit->first.getNodeId() == nodeId) {
            delete rit->second; // Delete Queue
            rit = rxBuffers_.erase(rit); // Delete Element
        }
        else {
            ++rit;
        }
    }
}

/**
 * Note: The current implementation of the
 *       newDataPkt to MAC, MacSduRequest to RlcAm mechanism is a workaround:
 *       Instead of sending one newDataPkt for a new SDU, we send a newDataPkt
 *       for each PDU since the number of retransmissions is not known beforehand.
 */
void LteRlcAm::indicateNewDataToMac(cPacket *pktAux) {
    Enter_Method("indicateNewDataToMac()");

    auto pkt = check_and_cast<inet::Packet *>(pktAux);

    // create a message to notify the MAC layer that the queue contains new data
    // (MAC is only interested in FlowControlInfo tag and size)

    auto newData = new Packet("AM-NewData");
    auto rlcSdu = inet::makeShared<LteRlcSdu>();
    rlcSdu->setLengthMainPacket(pkt->getByteLength());

    newData->insertAtFront(rlcSdu);

    auto newDataHdr = inet::makeShared<LteRlcPduNewData>();
    newData->insertAtFront(newDataHdr);

    newData->copyTags(*pkt);

    EV << "LteRlcAm::sendNewDataPkt - Sending message " << newData->getName() << " to port AM_Sap_down$o\n";
    send(newData, downOutGate_);
}

/*
 * Main functions
 */

void LteRlcAm::initialize()
{
    upInGate_ = gate("AM_Sap_up$i");
    upOutGate_ = gate("AM_Sap_up$o");
    downInGate_ = gate("AM_Sap_down$i");
    downOutGate_ = gate("AM_Sap_down$o");

    // parameters
    txEntityModuleType_ = cModuleType::get(par("txEntityModuleType").stringValue());
    rxEntityModuleType_ = cModuleType::get(par("rxEntityModuleType").stringValue());
}

void LteRlcAm::handleMessage(cMessage *msg)
{
    cPacket *pkt = check_and_cast<cPacket *>(msg);
    EV << NOW << " LteRlcAm : Received packet " << pkt->getName() << " from port "
       << pkt->getArrivalGate()->getName() << endl;

    cGate *incoming = pkt->getArrivalGate();
    if (incoming == upInGate_) {
        EV << NOW << " LteRlcAm : calling handleUpperMessage" << endl;
        handleUpperMessage(pkt);
    }
    else if (incoming == downInGate_) {
        EV << NOW << " LteRlcAm : calling handleLowerMessage" << endl;
        handleLowerMessage(pkt);
    }
}

} //namespace
