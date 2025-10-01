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

void NrPdcpEnb::analyzePacket(inet::Packet *pkt)
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

    // this is the body of former NrTxPdcpEntity::setIds()
    if (lteInfo->getUseNR() && getNodeTypeById(getNodeId()) != ENODEB && getNodeTypeById(getNodeId()) != GNODEB)
        lteInfo->setSourceId(getNrNodeId());
    else
        lteInfo->setSourceId(getNodeId());

    if (lteInfo->getMulticastGroupId() > 0)                                               // destId is meaningless for multicast D2D (we use the id of the source for statistical purposes at lower levels)
        lteInfo->setDestId(getNodeId());
    else
        lteInfo->setDestId(getDestId(lteInfo));

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

    // Dual Connectivity: adjust source and dest IDs for downlink packets in DC scenarios.
    // If this is a master eNB in DC and there's a secondary for this UE which will get this packet via X2 and transmit it via its RAN
    MacNodeId secondaryNodeId = binder_->getSecondaryNode(nodeId_);
    if (dualConnectivityEnabled_ && secondaryNodeId != NODEID_NONE && lteInfo->getUseNR()) {
        lteInfo->setSourceId(secondaryNodeId);
        lteInfo->setDestId(binder_->getNrMacNodeId(destAddr)); // use NR nodeId of the UE
    }

    // assign LCID
    lteInfo->setLcid(lcid);
}

void NrPdcpEnb::fromLowerLayer(cPacket *pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);
    pkt->trim();

    // if dual connectivity is enabled and this is a secondary node,
    // forward the packet to the PDCP of the master node
    MacNodeId masterId = binder_->getMasterNodeOrSelf(nodeId_);
    if (dualConnectivityEnabled_ && (nodeId_ != masterId)) {
        EV << NOW << " NrPdcpEnb::fromLowerLayer - forward packet to the master node - id [" << masterId << "]" << endl;
        forwardDataToTargetNode(pkt, masterId);
        return;
    }

    LtePdcpEnbD2D::fromLowerLayer(pktAux);
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
        master = binder_->getMasterNodeOrSelf(master);
        if (master != nodeId_) {
            destId = master;
        }
    }
    // else UE is directly attached
    return destId;
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

    auto ctrlInfo = pkt->getTag<FlowControlInfo>();
    if (ctrlInfo->getDirection() == DL) {
        MacNodeId destId = ctrlInfo->getDestId();
        EV << NOW << " NrPdcpEnb::receiveDataFromSourceNode - Received PDCP PDU from master node with id " << sourceNode << " - destination node[" << destId << "]" << endl;
        sendToLowerLayer(pkt);
    }
    else { // UL
        EV << NOW << " NrPdcpEnb::receiveDataFromSourceNode - Received PDCP PDU from secondary node with id " << sourceNode << endl;
        fromLowerLayer(pkt);
    }
}

void NrPdcpEnb::activeUeUL(std::set<MacNodeId> *ueSet)
{
    for (const auto& [cid, rxEntity] : rxEntities_) {
        MacNodeId nodeId = cid.getNodeId();
        if (!(rxEntity->isEmpty()))
            ueSet->insert(nodeId);
    }
}

} //namespace
