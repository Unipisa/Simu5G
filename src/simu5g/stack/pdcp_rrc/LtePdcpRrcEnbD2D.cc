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

#include "stack/pdcp_rrc/LtePdcpRrcEnbD2D.h"

#include <inet/common/packet/Packet.h>

#include "stack/d2dModeSelection/D2DModeSwitchNotification_m.h"
#include "stack/packetFlowManager/PacketFlowManagerBase.h"

namespace simu5g {

Define_Module(LtePdcpRrcEnbD2D);

using namespace omnetpp;
using namespace inet;

/*
 * Upper Layer handlers
 */
void LtePdcpRrcEnbD2D::fromDataPort(cPacket *pktAux)
{
    emit(receivedPacketFromUpperLayerSignal_, pktAux);

    // Control Information
    auto pkt = check_and_cast<Packet *>(pktAux);
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();

    setTrafficInformation(pkt, lteInfo);

    // get source info
    Ipv4Address srcAddr = Ipv4Address(lteInfo->getSrcAddr());
    // get destination info
    Ipv4Address destAddr = Ipv4Address(lteInfo->getDstAddr());
    MacNodeId srcId, destId;

    // set direction based on the destination Id. If the destination can be reached
    // using D2D, set D2D direction. Otherwise, set UL direction
    srcId = binder_->getMacNodeId(srcAddr);
    destId = binder_->getMacNodeId(destAddr);   // get final destination
    lteInfo->setDirection(getDirection());

    // check if src and dest of the flow are D2D-capable (currently in IM)
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

    // Cid Request
    EV << "LtePdcpRrcEnbD2D : Received CID request for Traffic [ " << "Source: " << Ipv4Address(lteInfo->getSrcAddr())
       << " Destination: " << Ipv4Address(lteInfo->getDstAddr())
       << " , ToS: " << lteInfo->getTypeOfService()
       << " , Direction: " << dirToA((Direction)lteInfo->getDirection()) << " ]\n";

    /*
     * Different LCID for different directions of the same flow are assigned.
     * RLC layer will create different RLC entities for different LCIDs
     */

    LogicalCid mylcid;
    if ((mylcid = ht_.find_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(), lteInfo->getTypeOfService(), lteInfo->getDirection())) == 0xFFFF) {
        // LCID not found

        // assign a new LCID to the connection
        mylcid = lcid_++;

        EV << "LtePdcpRrcEnbD2D : Connection not found, new CID created with LCID " << mylcid << "\n";

        ht_.create_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(), lteInfo->getTypeOfService(), lteInfo->getDirection(), mylcid);
    }

    // assign LCID
    lteInfo->setLcid(mylcid);
    lteInfo->setSourceId(nodeId_);

    // get effective next hop dest ID
    destId = getDestId(lteInfo);

    // obtain CID
    MacCid cid = idToMacCid(destId, mylcid);

    EV << "LtePdcpRrcEnbD2D : Assigned Lcid: " << mylcid << " [CID: " << cid << "]\n";
    EV << "LtePdcpRrcEnbD2D : Assigned Node ID: " << nodeId_ << "\n";
    EV << "LtePdcpRrcEnbD2D : dest ID: " << destId << "\n";

    // get the PDCP entity for this LCID and process the packet
    LteTxPdcpEntity *entity = getTxEntity(cid);
    entity->handlePacketFromUpperLayer(pkt);
}

void LtePdcpRrcEnbD2D::initialize(int stage)
{
    LtePdcpRrcEnb::initialize(stage);
}

void LtePdcpRrcEnbD2D::handleMessage(cMessage *msg)
{
    auto pkt = check_and_cast<inet::Packet *>(msg);
    auto chunk = pkt->peekAtFront<Chunk>();

    // check whether the message is a notification for mode switch
    if (inet::dynamicPtrCast<const D2DModeSwitchNotification>(chunk) != nullptr) {
        EV << "LtePdcpRrcEnbD2D::handleMessage - Received packet " << pkt->getName() << " from port " << pkt->getArrivalGate()->getName() << endl;

        auto switchPkt = pkt->peekAtFront<D2DModeSwitchNotification>();
        // call handler
        pdcpHandleD2DModeSwitch(switchPkt->getPeerId(), switchPkt->getNewMode());
        delete pkt;
    }
    else {
        LtePdcpRrcEnb::handleMessage(msg);
    }
}

void LtePdcpRrcEnbD2D::pdcpHandleD2DModeSwitch(MacNodeId peerId, LteD2DMode newMode)
{
    EV << NOW << " LtePdcpRrcEnbD2D::pdcpHandleD2DModeSwitch - peering with UE " << peerId << " set to " << d2dModeToA(newMode) << endl;

    // add here specific behavior for handling mode switch at the PDCP layer
}

} //namespace

