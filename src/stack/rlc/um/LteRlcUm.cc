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

#include "stack/rlc/um/LteRlcUm.h"
#include "stack/mac/packet/LteMacSduRequest.h"

Define_Module(LteRlcUm);

using namespace omnetpp;

UmTxEntity* LteRlcUm::getTxBuffer(FlowControlInfo* lteInfo)
{
    MacNodeId nodeId = 0;
    LogicalCid lcid = 0;
    if (lteInfo != NULL)
    {
        nodeId = ctrlInfoToUeId(lteInfo);

        /**
         * Note: Simu5G currently determines the LCID based
         * on the src/dst ip addresses and TOS field and uses separate
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
        throw cRuntimeError("LteRlcUm::getTxBuffer - lteInfo is NULL pointer. Aborting.");

    // Find TXBuffer for this CID
    MacCid cid = idToMacCid(nodeId, lcid);
    UmTxEntities::iterator it = txEntities_.find(cid);
    if (it == txEntities_.end())
    {
        // Not found: create
        std::stringstream buf;

        buf << "UmTxEntity Lcid: " << lcid;
        cModuleType* moduleType = cModuleType::get("simu5g.stack.rlc.UmTxEntity");
        UmTxEntity* txEnt = check_and_cast<UmTxEntity *>(moduleType->createScheduleInit(buf.str().c_str(), getParentModule()));
        txEntities_[cid] = txEnt;    // Add to tx_entities map

        if (lteInfo != nullptr)
        {
            // store control info for this flow
            txEnt->setFlowControlInfo(lteInfo->dup());
        }

        EV << "LteRlcUm : Added new UmTxEntity: " << txEnt->getId() <<
        " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return txEnt;
    }
    else
    {
        // Found
        EV << "LteRlcUm : Using old UmTxBuffer: " << it->second->getId() <<
        " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return it->second;
    }
}


UmRxEntity* LteRlcUm::getRxBuffer(FlowControlInfo* lteInfo)
{
    MacNodeId nodeId;
    if (lteInfo->getDirection() == DL)
        nodeId = lteInfo->getDestId();
    else
        nodeId = lteInfo->getSourceId();
    LogicalCid lcid = lteInfo->getLcid();

    // Find RXBuffer for this CID
    MacCid cid = idToMacCid(nodeId, lcid);

    UmRxEntities::iterator it = rxEntities_.find(cid);
    if (it == rxEntities_.end())
    {
        // Not found: create
        std::stringstream buf;
        buf << "UmRxEntity Lcid: " << lcid;
        cModuleType* moduleType = cModuleType::get("simu5g.stack.rlc.UmRxEntity");
        UmRxEntity* rxEnt = check_and_cast<UmRxEntity *>(
            moduleType->createScheduleInit(buf.str().c_str(), getParentModule()));
        rxEntities_[cid] = rxEnt;    // Add to rx_entities map

        // store control info for this flow
        rxEnt->setFlowControlInfo(lteInfo->dup());

        EV << "LteRlcUm : Added new UmRxEntity: " << rxEnt->getId() <<
        " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return rxEnt;
    }
    else
    {
        // Found
        EV << "LteRlcUm : Using old UmRxEntity: " << it->second->getId() <<
        " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return it->second;
    }
}

void LteRlcUm::sendDefragmented(cPacket *pkt)
{
    Enter_Method_Silent("sendDefragmented()");                    // Direct Method Call
    take(pkt);                                                    // Take ownership

    EV << "LteRlcUm : Sending packet " << pkt->getName() << " to port UM_Sap_up$o\n";
    send(pkt, up_[OUT_GATE]);

    emit(sentPacketToUpperLayer, pkt);
}

void LteRlcUm::sendToLowerLayer(cPacket *pktAux)
{
    Enter_Method_Silent("sendToLowerLayer()");                    // Direct Method Call
    take(pktAux);                                                    // Take ownership
    auto pkt = check_and_cast<inet::Packet *> (pktAux);
    pkt->addTagIfAbsent<inet::PacketProtocolTag>()->setProtocol(&LteProtocol::rlc);
    EV << "LteRlcUm : Sending packet " << pktAux->getName() << " to port UM_Sap_down$o\n";
    send(pktAux, down_[OUT_GATE]);

    auto  lteInfo = pkt->getTag<FlowControlInfo>();

    if (lteInfo->getDirection()==DL)
        emit(rlcPacketLossDl, 0.0);
    else
        emit(rlcPacketLossUl, 0.0);

    emit(sentPacketToLowerLayer, pkt);
}

void LteRlcUm::dropBufferOverflow(cPacket *pktAux)
{
    Enter_Method_Silent("dropBufferOverflow()");                  // Direct Method Call
    take(pktAux);                                                    // Take ownership

    EV << "LteRlcUm : Dropping packet " << pktAux->getName() << " (queue full) \n";

    auto pkt = check_and_cast<inet::Packet *> (pktAux);
    auto lteInfo = pkt->getTag<FlowControlInfo>();

   if (lteInfo->getDirection()==DL)
       emit(rlcPacketLossDl, 1.0);
   else
       emit(rlcPacketLossUl, 1.0);

   delete pkt;
}

void LteRlcUm::handleUpperMessage(cPacket *pktAux)
{
    emit(receivedPacketFromUpperLayer, pktAux);

    auto pkt = check_and_cast<inet::Packet *> (pktAux);
    auto lteInfo = pkt->getTag<FlowControlInfo>();

    auto chunk = pkt->peekAtFront<inet::Chunk>();
    EV << "LteRlcUm::handleUpperMessage - Received packet " << chunk->getClassName() << " from upper layer, size " << pktAux->getByteLength() << "\n";

    UmTxEntity* txbuf = getTxBuffer(lteInfo);

    // Create a new RLC packet
    auto rlcPkt = inet::makeShared<LteRlcSdu>();
    rlcPkt->setSnoMainPacket(lteInfo->getSequenceNumber());
    rlcPkt->setLengthMainPacket(pkt->getByteLength());
    pkt->insertAtFront(rlcPkt);

    drop(pkt);

    if (txbuf->isHoldingDownstreamInPackets())
    {
        // do not store in the TX buffer and do not signal the MAC layer
        EV << "LteRlcUm::handleUpperMessage - Enque packet " << rlcPkt->getClassName() << " into the Holding Buffer\n";
        txbuf->enqueHoldingPackets(pkt);
    }
    else
    {
        if(txbuf->enque(pkt)){
            EV << "LteRlcUm::handleUpperMessage - Enque packet " << rlcPkt->getClassName() << " into the Tx Buffer\n";

            // create a message so as to notify the MAC layer that the queue contains new data
            auto newDataPkt = inet::makeShared<LteRlcPduNewData>();
            // make a copy of the RLC SDU
            auto pktDup = pkt->dup();
            pktDup->insertAtFront(newDataPkt);
            // the MAC will only be interested in the size of this packet

            EV << "LteRlcUm::handleUpperMessage - Sending message " << newDataPkt->getClassName() << " to port UM_Sap_down$o\n";
            send(pktDup, down_[OUT_GATE]);
        } else {
            // Queue is full - drop SDU
            dropBufferOverflow(pkt);
        }
    }
}

void LteRlcUm::handleLowerMessage(cPacket *pktAux)
{
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    EV << "LteRlcUm::handleLowerMessage - Received packet " << pkt->getName() << " from lower layer\n";
    auto lteInfo = pkt->getTag<FlowControlInfo>();
    auto chunk = pkt->peekAtFront<inet::Chunk>();

    if (inet::dynamicPtrCast<const LteMacSduRequest>(chunk) != nullptr)
    {
        // get the corresponding Tx buffer
        UmTxEntity* txbuf = getTxBuffer(lteInfo);

        auto macSduRequest = pkt->peekAtFront<LteMacSduRequest>();
        unsigned int size = macSduRequest->getSduSize();

        drop(pkt);

        // do segmentation/concatenation and send a pdu to the lower layer
        txbuf->rlcPduMake(size);

        delete pkt;
    }
    else
    {
        emit(receivedPacketFromLowerLayer, pkt);

        // Extract informations from fragment
        UmRxEntity* rxbuf = getRxBuffer(lteInfo);
        drop(pkt);

        // Bufferize PDU
        EV << "LteRlcUm::handleLowerMessage - Enque packet " << pkt->getName() << " into the Rx Buffer\n";
        rxbuf->enque(pkt);
    }
}

void LteRlcUm::deleteQueues(MacNodeId nodeId)
{
    UmTxEntities::iterator tit;
    UmRxEntities::iterator rit;

    RanNodeType nodeType;
    std::string nodeTypePar = getAncestorPar("nodeType").stdstringValue();
    if (strcmp(nodeTypePar.c_str(), "ENODEB") == 0)
        nodeType = ENODEB;
    else if (strcmp(nodeTypePar.c_str(), "GNODEB") == 0)
        nodeType = GNODEB;
    else
        nodeType = UE;

    // at the UE, delete all connections
    // at the eNB, delete connections related to the given UE
    for (tit = txEntities_.begin(); tit != txEntities_.end(); )
    {
        if (nodeType == UE || ((nodeType == ENODEB || nodeType == GNODEB) && MacCidToNodeId(tit->first) == nodeId))
        {
            tit->second->deleteModule(); // Delete Entity
            txEntities_.erase(tit++);    // Delete Elem
        }
        else
        {
            ++tit;
        }
    }
    for (rit = rxEntities_.begin(); rit != rxEntities_.end(); )
    {
        if (nodeType == UE || ((nodeType == ENODEB || nodeType == GNODEB) && MacCidToNodeId(rit->first) == nodeId))
        {
            rit->second->deleteModule(); // Delete Entity
            rxEntities_.erase(rit++);    // Delete Elem
        }
        else
        {
            ++rit;
        }
    }
}

/*
 * Main functions
 */

void LteRlcUm::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
    {
        up_[IN_GATE] = gate("UM_Sap_up$i");
        up_[OUT_GATE] = gate("UM_Sap_up$o");
        down_[IN_GATE] = gate("UM_Sap_down$i");
        down_[OUT_GATE] = gate("UM_Sap_down$o");

        // parameters
        mapAllLcidsToSingleBearer_ = par("mapAllLcidsToSingleBearer");

        // statistics
        receivedPacketFromUpperLayer = registerSignal("receivedPacketFromUpperLayer");
        receivedPacketFromLowerLayer = registerSignal("receivedPacketFromLowerLayer");
        sentPacketToUpperLayer = registerSignal("sentPacketToUpperLayer");
        sentPacketToLowerLayer = registerSignal("sentPacketToLowerLayer");
        rlcPacketLossDl = registerSignal("rlcPacketLossDl");
        rlcPacketLossUl = registerSignal("rlcPacketLossUl");

        WATCH_MAP(txEntities_);
        WATCH_MAP(rxEntities_);
    }
}

void LteRlcUm::handleMessage(cMessage* msg)
{
    cPacket* pkt = check_and_cast<cPacket *>(msg);
    EV << "LteRlcUm : Received packet " << pkt->getName() << " from port " << pkt->getArrivalGate()->getName() << endl;

    cGate* incoming = pkt->getArrivalGate();
    if (incoming == up_[IN_GATE])
    {
        handleUpperMessage(pkt);
    }
    else if (incoming == down_[IN_GATE])
    {
        handleLowerMessage(pkt);
    }
    return;
}
