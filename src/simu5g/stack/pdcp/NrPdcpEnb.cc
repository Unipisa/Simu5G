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

#include <inet/networklayer/common/NetworkInterface.h>

#include "simu5g/stack/pdcp/NrPdcpEnb.h"
#include "simu5g/stack/packetFlowManager/PacketFlowManagerBase.h"

namespace simu5g {

Define_Module(NrPdcpEnb);

void NrPdcpEnb::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        inet::NetworkInterface *nic = inet::getContainingNicModule(this);
        dualConnectivityEnabled_ = nic->par("dualConnectivityEnabled").boolValue();
        if (dualConnectivityEnabled_)
            dualConnectivityManager_.reference(this, "dualConnectivityManagerModule", true);
    }
    LtePdcpEnbD2D::initialize(stage);
}

/*
 * Upper Layer handlers
 */

MacCid NrPdcpEnb::analyzePacket(inet::Packet *pkt)
{
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    setTrafficInformation(pkt, lteInfo);

    // get source info
    Ipv4Address srcAddr = Ipv4Address(lteInfo->getSrcAddr());
    // get destination info
    Ipv4Address destAddr = Ipv4Address(lteInfo->getDstAddr());
    MacNodeId srcId, destId;

    // set direction based on the destination Id. If the destination can be reached
    // using D2D, set D2D direction. Otherwise, set UL direction
    srcId = (lteInfo->getUseNR()) ? binder_->getNrMacNodeId(srcAddr) : binder_->getMacNodeId(srcAddr);
    destId = (lteInfo->getUseNR()) ? binder_->getNrMacNodeId(destAddr) : binder_->getMacNodeId(destAddr);   // get final destination
    lteInfo->setDirection(getDirection());

    // check if src and dest of the flow are D2D-capable UEs (currently in IM)
    if (getNodeTypeById(srcId) == UE && getNodeTypeById(destId) == UE && binder_->getD2DCapability(srcId, destId)) {
        // this way, we record the ID of the endpoint even if the connection is in IM
        // this is useful for mode switching
        lteInfo->setD2dTxPeerId(srcId);
        lteInfo->setD2dRxPeerId(destId);
    }
    else {
        lteInfo->setD2dTxPeerId(NODEID_NONE);
        lteInfo->setD2dRxPeerId(NODEID_NONE);
    }

    // CID Request
    EV << "NrPdcpEnb : Received CID request for Traffic [ " << "Source: " << Ipv4Address(lteInfo->getSrcAddr())
       << " Destination: " << Ipv4Address(lteInfo->getDstAddr())
       << " , ToS: " << lteInfo->getTypeOfService()
       << " , Direction: " << dirToA((Direction)lteInfo->getDirection()) << " ]\n";

    /*
     * Different LCID for different directions of the same flow are assigned.
     * RLC layer will create different RLC entities for different LCIDs
     */

    ConnectionKey key{srcAddr, destAddr, lteInfo->getTypeOfService(), lteInfo->getDirection()};
    LogicalCid lcid = lookupOrAssignLcid(key);

    // assign LCID
    lteInfo->setLcid(lcid);

    // obtain CID
    return MacCid(destId, lcid);
}

void NrPdcpEnb::fromLowerLayer(cPacket *pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);
    pkt->trim();

    // if dual connectivity is enabled and this is a secondary node,
    // forward the packet to the PDCP of the master node
    MacNodeId masterId = binder_->getMasterNode(nodeId_);
    if (dualConnectivityEnabled_ && (nodeId_ != masterId)) {
        EV << NOW << " NrPdcpEnb::fromLowerLayer - forward packet to the master node - id [" << masterId << "]" << endl;
        forwardDataToTargetNode(pkt, masterId);
        return;
    }

    emit(receivedPacketFromLowerLayerSignal_, pkt);

    auto lteInfo = pkt->getTag<FlowControlInfo>();

    EV << "LtePdcp : Received packet with CID " << lteInfo->getLcid() << "\n";
    EV << "LtePdcp : Packet size " << pkt->getByteLength() << " Bytes\n";

    MacCid cid = MacCid(lteInfo->getSourceId(), lteInfo->getLcid());   // TODO: check if you have to get master node id

    LteRxPdcpEntity *entity = getOrCreateRxEntity(cid);
    entity->handlePacketFromLowerLayer(pkt);
}

MacNodeId NrPdcpEnb::getDestId(inet::Ptr<FlowControlInfo> lteInfo)
{
    MacNodeId destId;
    if (!dualConnectivityEnabled_ || lteInfo->getUseNR())
        destId = binder_->getNrMacNodeId(Ipv4Address(lteInfo->getDstAddr()));
    else
        destId = binder_->getMacNodeId(Ipv4Address(lteInfo->getDstAddr()));

    // master of this UE
    MacNodeId master = binder_->getNextHop(destId);
    if (master != nodeId_) {
        destId = master;
    }
    else {
        // for dual connectivity
        master = binder_->getMasterNode(master);
        if (master != nodeId_) {
            destId = master;
        }
    }
    // else UE is directly attached
    return destId;
}

LteTxPdcpEntity *NrPdcpEnb::getOrCreateTxEntity(MacCid cid)
{
    // Find entity for this CID
    PdcpTxEntities::iterator it = txEntities_.find(cid);
    if (it == txEntities_.end()) {
        // Not found: create

        std::stringstream buf;
        buf << "tx-" << cid.getNodeId() << "-" << cid.getLcid();
        cModuleType *moduleType = cModuleType::get("simu5g.stack.pdcp.NrTxPdcpEntity");
        NrTxPdcpEntity *txEnt = check_and_cast<NrTxPdcpEntity *>(moduleType->createScheduleInit(buf.str().c_str(), this));
        txEntities_[cid] = txEnt;    // Add to entities map

        EV << "NrPdcpEnb::getEntity - Added new PdcpEntity for Cid: " << cid << "\n";

        return txEnt;
    }
    else {
        // Found
        EV << "NrPdcpEnb::getEntity - Using old PdcpEntity for Cid: " << cid << "\n";

        return it->second;
    }
}

LteRxPdcpEntity *NrPdcpEnb::getOrCreateRxEntity(MacCid cid)
{
    // Find entity for this CID
    PdcpRxEntities::iterator it = rxEntities_.find(cid);
    if (it == rxEntities_.end()) {
        // Not found: create

        std::stringstream buf;
        buf << "rx-" << cid.getNodeId() << "-" << cid.getLcid();
        cModuleType *moduleType = cModuleType::get("simu5g.stack.pdcp.NrRxPdcpEntity");
        LteRxPdcpEntity *rxEnt = check_and_cast<LteRxPdcpEntity *>(moduleType->createScheduleInit(buf.str().c_str(), this));
        rxEntities_[cid] = rxEnt;    // Add to entities map

        EV << "NrPdcpEnb::getRxEntity - Added new RxPdcpEntity for Cid: " << cid << "\n";

        return rxEnt;
    }
    else {
        // Found
        EV << "NrPdcpEnb::getRxEntity - Using old RxPdcpEntity for Cid: " << cid << "\n";

        return it->second;
    }
}

void NrPdcpEnb::forwardDataToTargetNode(Packet *pkt, MacNodeId targetNode)
{
    EV << NOW << " NrPdcpEnb::forwardDataToTargetNode - Send PDCP packet to node with id " << targetNode << endl;
    dualConnectivityManager_->forwardDataToTargetNode(pkt, targetNode);
}

void NrPdcpEnb::receiveDataFromSourceNode(Packet *pkt, MacNodeId sourceNode)
{
    Enter_Method("receiveDataFromSourceNode");
    take(pkt);

    auto ctrlInfo = pkt->getTagForUpdate<FlowControlInfo>();
    if (ctrlInfo->getDirection() == DL) {
        // if DL, forward the PDCP PDU to the RLC layer

        // recover the original destId of the UE, using the destAddress and write it into the ControlInfo
        MacNodeId destId = binder_->getNrMacNodeId(Ipv4Address(ctrlInfo->getDstAddr()));
        ctrlInfo->setSourceId(nodeId_);
        ctrlInfo->setDestId(destId);

        EV << NOW << " NrPdcpEnb::receiveDataFromSourceNode - Received PDCP PDU from master node with id " << sourceNode << " - destination node[" << destId << "]" << endl;

        sendToLowerLayer(pkt);
    }
    else { // UL
        // if UL, call the handler for reception from RLC layer (of the secondary node)
        EV << NOW << " NrPdcpEnb::receiveDataFromSourceNode - Received PDCP PDU from secondary node with id " << sourceNode << endl;
        fromLowerLayer(pkt);
    }
}

void NrPdcpEnb::activeUeUL(std::set<MacNodeId> *ueSet)
{
    for (const auto& entity : rxEntities_) {
        MacNodeId nodeId = entity.first.getNodeId();
        if (!(entity.second->isEmpty()))
            ueSet->insert(nodeId);
    }
}

} //namespace

