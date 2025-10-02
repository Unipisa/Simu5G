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

#include "simu5g/stack/pdcp/LtePdcpUeD2D.h"
#include <inet/networklayer/common/L3AddressResolver.h>
#include "simu5g/stack/d2dModeSelection/D2DModeSwitchNotification_m.h"
#include "simu5g/stack/packetFlowManager/PacketFlowManagerBase.h"

namespace simu5g {

Define_Module(LtePdcpUeD2D);

using namespace inet;
using namespace omnetpp;

MacNodeId LtePdcpUeD2D::getDestId(inet::Ptr<FlowControlInfo> lteInfo)
{
    Ipv4Address destAddr = Ipv4Address(lteInfo->getDstAddr());
    MacNodeId destId = binder_->getMacNodeId(destAddr);

    // check if the destination is inside the LTE network
    if (destId == NODEID_NONE || getDirection(destId) == UL) { // if not, the packet is destined to the eNB
        // UE is subject to handovers: master may change
        return binder_->getNextHop(lteInfo->getSourceId());
    }

    return destId;
}

/*
 * Upper Layer handlers
 */

void LtePdcpUeD2D::analyzePacket(inet::Packet *pkt)
{
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();

    setTrafficInformation(pkt, lteInfo);

    // get destination info
    Ipv4Address destAddr = Ipv4Address(lteInfo->getDstAddr());
    MacNodeId destId;

    // the direction of the incoming connection is a D2D_MULTI one if the application is of the same type,
    // else the direction will be selected according to the current status of the UE, i.e., D2D or UL
    if (destAddr.isMulticast()) {
        binder_->addD2DMulticastTransmitter(nodeId_);

        lteInfo->setDirection(D2D_MULTI);

        // assign a multicast group id
        // multicast IP addresses are 224.0.0.0/4.
        // We consider the host part of the IP address (the remaining 28 bits) as identifier of the group,
        // so as it is uniquely determined for the whole network
        uint32_t address = Ipv4Address(lteInfo->getDstAddr()).getInt();
        uint32_t mask = ~((uint32_t)255 << 28);      // 0000 1111 1111 1111
        uint32_t groupId = address & mask;
        lteInfo->setMulticastGroupId((int32_t)groupId);
    }
    else {
        destId = binder_->getMacNodeId(destAddr);
        if (destId != NODEID_NONE) { // the destination is a UE within the LTE network
            if (binder_->checkD2DCapability(nodeId_, destId)) {
                // this way, we record the ID of the endpoints even if the connection is currently in IM
                // this is useful for mode switching
                lteInfo->setD2dTxPeerId(nodeId_);
                lteInfo->setD2dRxPeerId(destId);
            }
            else {
                lteInfo->setD2dTxPeerId(NODEID_NONE);
                lteInfo->setD2dRxPeerId(NODEID_NONE);
            }

            // set actual flow direction based (D2D/UL) based on the current mode (DM/IM) of this peering
            lteInfo->setDirection(getDirection(destId));
        }
        else { // the destination is outside the LTE network
            lteInfo->setDirection(UL);
            lteInfo->setD2dTxPeerId(NODEID_NONE);
            lteInfo->setD2dRxPeerId(NODEID_NONE);
        }
    }

    // Cid Request
    EV << "LtePdcpUeD2D : Received CID request for Traffic [ " << "Source: " << Ipv4Address(lteInfo->getSrcAddr())
       << " Destination: " << Ipv4Address(lteInfo->getDstAddr())
       << " , ToS: " << lteInfo->getTypeOfService()
       << " , Direction: " << dirToA((Direction)lteInfo->getDirection()) << " ]\n";

    /*
     * Different lcid for different directions of the same flow are assigned.
     * RLC layer will create different RLC entities for different LCIDs
     */

    ConnectionKey key{Ipv4Address(lteInfo->getSrcAddr()), destAddr, lteInfo->getTypeOfService(), lteInfo->getDirection()};
    LogicalCid lcid = lookupOrAssignLcid(key);

    // assign LCID
    lteInfo->setLcid(lcid);
    lteInfo->setSourceId(nodeId_);

    EV << "LtePdcpUeD2D : Assigned Lcid: " << lcid << "\n";
    EV << "LtePdcpUeD2D : Assigned Node ID: " << nodeId_ << "\n";

    // get effective next hop dest ID
    destId = getDestId(lteInfo);

    // this is the body of former LteTxPdcpEntity::setIds()
    lteInfo->setSourceId(getNodeId());
    if (lteInfo->getMulticastGroupId() > 0)                                               // destId is meaningless for multicast D2D (we use the id of the source for statistic purposes at lower levels)
        lteInfo->setDestId(getNodeId());
    else
        lteInfo->setDestId(getDestId(lteInfo));
}

void LtePdcpUeD2D::handleMessage(cMessage *msg)
{
    cPacket *pktAux = check_and_cast<cPacket *>(msg);

    // check whether the message is a notification for mode switch
    if (strcmp(pktAux->getName(), "D2DModeSwitchNotification") == 0) {
        EV << "LtePdcpUeD2D::handleMessage - Received packet " << pktAux->getName() << " from port " << pktAux->getArrivalGate()->getName() << endl;

        auto pkt = check_and_cast<inet::Packet *>(pktAux);
        auto switchPkt = pkt->peekAtFront<D2DModeSwitchNotification>();

        // call handler
        pdcpHandleD2DModeSwitch(switchPkt->getPeerId(), switchPkt->getNewMode());

        delete pktAux;
    }
    else {
        LtePdcpBase::handleMessage(msg);
    }
}

void LtePdcpUeD2D::pdcpHandleD2DModeSwitch(MacNodeId peerId, LteD2DMode newMode)
{
    EV << NOW << " LtePdcpUeD2D::pdcpHandleD2DModeSwitch - peering with UE " << peerId << " set to " << d2dModeToA(newMode) << endl;

    // add here specific behavior for handling mode switch at the PDCP layer
}

} //namespace

