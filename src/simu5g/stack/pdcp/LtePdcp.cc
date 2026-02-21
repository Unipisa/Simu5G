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

#include "simu5g/stack/pdcp/LtePdcp.h"
#include "simu5g/stack/pdcp/NrPdcpEnb.h"
#include "simu5g/stack/pdcp/NrPdcpUe.h"
#include "simu5g/stack/pdcp/LtePdcpEnbD2D.h"
#include "simu5g/stack/pdcp/LtePdcpUeD2D.h"

#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/transportlayer/tcp_common/TcpHeader.h>
#include <inet/transportlayer/udp/UdpHeader_m.h>

#include "simu5g/stack/packetFlowObserver/PacketFlowObserverBase.h"
#include "simu5g/stack/pdcp/packet/LteRohcPdu_m.h"
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

Define_Module(LtePdcpUe);
Define_Module(LtePdcpEnb);

using namespace omnetpp;
using namespace inet;

// statistics
simsignal_t LtePdcpBase::receivedPacketFromUpperLayerSignal_ = registerSignal("receivedPacketFromUpperLayer");
simsignal_t LtePdcpBase::receivedPacketFromLowerLayerSignal_ = registerSignal("receivedPacketFromLowerLayer");
simsignal_t LtePdcpBase::sentPacketToUpperLayerSignal_ = registerSignal("sentPacketToUpperLayer");
simsignal_t LtePdcpBase::sentPacketToLowerLayerSignal_ = registerSignal("sentPacketToLowerLayer");

LtePdcpBase::~LtePdcpBase()
{
}

LteTrafficClass LtePdcpBase::getTrafficCategory(cPacket *pkt)
{
    const char *name = pkt->getName();
    if (opp_stringbeginswith(name, "VoIP"))
        return CONVERSATIONAL;
    else if (opp_stringbeginswith(name, "gaming"))
        return INTERACTIVE;
    else if (opp_stringbeginswith(name, "VoDPacket") || opp_stringbeginswith(name, "VoDFinishPacket"))
        return STREAMING;
    else
        return BACKGROUND;
}

LteRlcType LtePdcpBase::getRlcType(LteTrafficClass trafficCategory)
{
    switch (trafficCategory) {
        case CONVERSATIONAL:
            return conversationalRlc_;
        case INTERACTIVE:
            return interactiveRlc_;
        case STREAMING:
            return streamingRlc_;
        case BACKGROUND:
            return backgroundRlc_;
        default:
            return backgroundRlc_;  // fallback
    }
}

LogicalCid LtePdcpBase::lookupOrAssignLcid(const ConnectionKey& key)
{
    auto it = lcidTable_.find(key);
    if (it != lcidTable_.end())
        return it->second;
    else {
        LogicalCid lcid = lcid_++;
        lcidTable_[key] = lcid;
        EV << "Connection not found, new CID created with LCID " << lcid << "\n";
        return lcid;
    }
}

/*
 * Upper Layer handlers
 */

void LtePdcpBase::analyzePacket(inet::Packet *pkt)
{
    if (dynamic_cast<NrPdcpEnb *>(this)) {
        // --- NrPdcpEnb ---
        auto lteInfo = pkt->addTagIfAbsent<FlowControlInfo>();

        // Traffic category, RLC type
        LteTrafficClass trafficCategory = getTrafficCategory(pkt);
        LteRlcType rlcType = getRlcType(trafficCategory);
        lteInfo->setTraffic(trafficCategory);
        lteInfo->setRlcType(rlcType);

        // direction of transmitted packets depends on node type
        Direction dir = getNodeTypeById(nodeId_) == UE ? UL : DL;
        lteInfo->setDirection(dir);

        // get IP flow information
        auto ipFlowInd = pkt->getTag<IpFlowInd>();
        Ipv4Address srcAddr = ipFlowInd->getSrcAddr();
        Ipv4Address destAddr = ipFlowInd->getDstAddr();
        uint16_t typeOfService = ipFlowInd->getTypeOfService();
        EV << "Received packet from data port, src= " << srcAddr << " dest=" << destAddr << " ToS=" << typeOfService << endl;

        bool useNR = pkt->getTag<TechnologyReq>()->getUseNR();

        lteInfo->setD2dTxPeerId(NODEID_NONE);
        lteInfo->setD2dRxPeerId(NODEID_NONE);

        lteInfo->setSourceId(getNodeId());

        if (lteInfo->getMulticastGroupId() != NODEID_NONE)   // destId is meaningless for multicast D2D (we use the id of the source for statistical purposes at lower levels)
            lteInfo->setDestId(getNodeId());
        else
            lteInfo->setDestId(getNextHopNodeId(destAddr, useNR, lteInfo->getSourceId()));

        // Dual Connectivity: adjust source and dest IDs for downlink packets in DC scenarios.
        // If this is a master eNB in DC and there's a secondary for this UE which will get this packet via X2 and transmit it via its RAN
        MacNodeId secondaryNodeId = binder_->getSecondaryNode(nodeId_);
        if (isDualConnectivityEnabled() && secondaryNodeId != NODEID_NONE && useNR) {
            lteInfo->setSourceId(secondaryNodeId);
            lteInfo->setDestId(binder_->getNrMacNodeId(destAddr)); // use NR nodeId of the UE
        }

        // assign LCID
        ConnectionKey key{srcAddr, destAddr, typeOfService, lteInfo->getDirection()};
        LogicalCid lcid = lookupOrAssignLcid(key);
        lteInfo->setLcid(lcid);
    }
    else if (dynamic_cast<NrPdcpUe *>(this)) {
        // --- NrPdcpUe ---
        auto lteInfo = pkt->addTagIfAbsent<FlowControlInfo>();

        // Traffic category, RLC type
        LteTrafficClass trafficCategory = getTrafficCategory(pkt);
        LteRlcType rlcType = getRlcType(trafficCategory);
        lteInfo->setTraffic(trafficCategory);
        lteInfo->setRlcType(rlcType);

        // direction of transmitted packets depends on node type
        Direction dir = getNodeTypeById(nodeId_) == UE ? UL : DL;
        lteInfo->setDirection(dir);

        bool useNR = pkt->getTag<TechnologyReq>()->getUseNR();

        // select the correct nodeId for the source
        MacNodeId nodeId = useNR ? getNrNodeId() : nodeId_;
        lteInfo->setSourceId(nodeId);

        // get IP flow information
        auto ipFlowInd = pkt->getTag<IpFlowInd>();
        Ipv4Address srcAddr = ipFlowInd->getSrcAddr();
        Ipv4Address destAddr = ipFlowInd->getDstAddr();
        uint16_t typeOfService = ipFlowInd->getTypeOfService();

        // the direction of the incoming connection is a D2D_MULTI one if the application is of the same type,
        // else the direction will be selected according to the current status of the UE, i.e., D2D or UL
        if (destAddr.isMulticast()) {
            binder_->addD2DMulticastTransmitter(nodeId);

            lteInfo->setDirection(D2D_MULTI);

            // assign a multicast group id
            MacNodeId groupId = binder_->getOrAssignDestIdForMulticastAddress(destAddr);
            lteInfo->setMulticastGroupId(groupId);
        }
        else {
            MacNodeId destId = binder_->getMacNodeId(destAddr);
            if (destId != NODEID_NONE) { // the destination is a UE within the LTE network
                if (binder_->checkD2DCapability(nodeId, destId)) {
                    // this way, we record the ID of the endpoints even if the connection is currently in IM
                    // this is useful for mode switching
                    lteInfo->setD2dTxPeerId(nodeId);
                    lteInfo->setD2dRxPeerId(destId);
                }
                else {
                    lteInfo->setD2dTxPeerId(NODEID_NONE);
                    lteInfo->setD2dRxPeerId(NODEID_NONE);
                }

                // set actual flow direction based (D2D/UL) based on the current mode (DM/IM) of this pairing
                // (inlined from NrPdcpUe::getDirection)
                if (binder_->getD2DCapability(nodeId, destId) && binder_->getD2DMode(nodeId, destId) == DM)
                    lteInfo->setDirection(D2D);
                else
                    lteInfo->setDirection(UL);
            }
            else { // the destination is outside the LTE network
                lteInfo->setDirection(UL);
                lteInfo->setD2dTxPeerId(NODEID_NONE);
                lteInfo->setD2dRxPeerId(NODEID_NONE);
            }
        }

        lteInfo->setSourceId(useNR ? getNrNodeId() : getNodeId());

        if (lteInfo->getMulticastGroupId() != NODEID_NONE)   // destId is meaningless for multicast D2D (we use the id of the source for statistical purposes at lower levels)
            lteInfo->setDestId(getNodeId());
        else
            lteInfo->setDestId(getNextHopNodeId(destAddr, useNR, lteInfo->getSourceId()));

        // assign LCID
        ConnectionKey key{srcAddr, destAddr, typeOfService, lteInfo->getDirection()};
        LogicalCid lcid = lookupOrAssignLcid(key);
        lteInfo->setLcid(lcid);

        EV << "NrPdcpUe : Assigned Lcid: " << lcid << "\n";
        EV << "NrPdcpUe : Assigned Node ID: " << nodeId << "\n";

        // get effective next hop dest ID
        // TODO this was in the original code, but has no effect:
        // MacNodeId destId = getNextHopNodeId(destAddr, useNR, lteInfo->getSourceId());
    }
    else if (dynamic_cast<LtePdcpEnbD2D *>(this)) {
        // --- LtePdcpEnbD2D ---
        auto lteInfo = pkt->addTagIfAbsent<FlowControlInfo>();

        // Traffic category, RLC type
        LteTrafficClass trafficCategory = getTrafficCategory(pkt);
        LteRlcType rlcType = getRlcType(trafficCategory);
        lteInfo->setTraffic(trafficCategory);
        lteInfo->setRlcType(rlcType);

        // direction of transmitted packets depends on node type
        Direction dir = getNodeTypeById(nodeId_) == UE ? UL : DL;
        lteInfo->setDirection(dir);

        // get IP flow information
        auto ipFlowInd = pkt->getTag<IpFlowInd>();
        Ipv4Address srcAddr = ipFlowInd->getSrcAddr();
        Ipv4Address destAddr = ipFlowInd->getDstAddr();
        uint16_t typeOfService = ipFlowInd->getTypeOfService();
        EV << "Received packet from data port, src= " << srcAddr << " dest=" << destAddr << " ToS=" << typeOfService << endl;

        lteInfo->setD2dTxPeerId(NODEID_NONE);
        lteInfo->setD2dRxPeerId(NODEID_NONE);

        // assign LCID
        ConnectionKey key{srcAddr, destAddr, typeOfService, lteInfo->getDirection()};
        LogicalCid lcid = lookupOrAssignLcid(key);
        lteInfo->setLcid(lcid);

        lteInfo->setSourceId(nodeId_);

        // get effective next hop dest ID
        bool useNR = pkt->getTag<TechnologyReq>()->getUseNR();
        (void)useNR; // we explicitly ignore this (why?)

        // this is the body of former LteTxPdcpEntity::setIds()
        lteInfo->setSourceId(getNodeId());   // TODO CHANGE HERE!!! Must be the NR node ID if this is an NR connection
        if (lteInfo->getMulticastGroupId() != NODEID_NONE)  // destId is meaningless for multicast D2D (we use the id of the source for statistic purposes at lower levels)
            lteInfo->setDestId(getNodeId());
        else
            lteInfo->setDestId(getNextHopNodeId(destAddr, false, lteInfo->getSourceId()));
    }
    else if (dynamic_cast<LtePdcpUeD2D *>(this)) {
        // --- LtePdcpUeD2D ---
        auto lteInfo = pkt->addTagIfAbsent<FlowControlInfo>();

        // Traffic category, RLC type
        LteTrafficClass trafficCategory = getTrafficCategory(pkt);
        LteRlcType rlcType = getRlcType(trafficCategory);
        lteInfo->setTraffic(trafficCategory);
        lteInfo->setRlcType(rlcType);

        // direction of transmitted packets depends on node type
        Direction dir = getNodeTypeById(nodeId_) == UE ? UL : DL;
        lteInfo->setDirection(dir);

        // get IP flow information
        auto ipFlowInd = pkt->getTag<IpFlowInd>();
        Ipv4Address srcAddr = ipFlowInd->getSrcAddr();
        Ipv4Address destAddr = ipFlowInd->getDstAddr();
        uint16_t typeOfService = ipFlowInd->getTypeOfService();
        EV << "Received packet from data port, src= " << srcAddr << " dest=" << destAddr << " ToS=" << typeOfService << endl;

        MacNodeId destId;

        // the direction of the incoming connection is a D2D_MULTI one if the application is of the same type,
        // else the direction will be selected according to the current status of the UE, i.e., D2D or UL
        if (destAddr.isMulticast()) {
            binder_->addD2DMulticastTransmitter(nodeId_);

            lteInfo->setDirection(D2D_MULTI);

            // assign a multicast group id
            MacNodeId groupId = binder_->getOrAssignDestIdForMulticastAddress(destAddr);
            lteInfo->setMulticastGroupId(groupId);
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
                // (inlined from LtePdcpUeD2D::getDirection)
                if (binder_->getD2DCapability(nodeId_, destId) && binder_->getD2DMode(nodeId_, destId) == DM)
                    lteInfo->setDirection(D2D);
                else
                    lteInfo->setDirection(UL);
            }
            else { // the destination is outside the LTE network
                lteInfo->setDirection(UL);
                lteInfo->setD2dTxPeerId(NODEID_NONE);
                lteInfo->setD2dRxPeerId(NODEID_NONE);
            }
        }

        // assign LCID
        ConnectionKey key{srcAddr, destAddr, typeOfService, lteInfo->getDirection()};
        LogicalCid lcid = lookupOrAssignLcid(key);
        lteInfo->setLcid(lcid);

        lteInfo->setSourceId(nodeId_);

        EV << "LtePdcpUeD2D : Assigned Lcid: " << lcid << "\n";
        EV << "LtePdcpUeD2D : Assigned Node ID: " << nodeId_ << "\n";

        bool useNR = pkt->getTag<TechnologyReq>()->getUseNR();
        destId = getNextHopNodeId(destAddr, useNR, lteInfo->getSourceId());

        lteInfo->setSourceId(getNodeId());   // TODO CHANGE HERE!!! Must be the NR node ID if this is an NR connection
        if (lteInfo->getMulticastGroupId() != NODEID_NONE)   // destId is meaningless for multicast D2D (we use the id of the source for statistic purposes at lower levels)
            lteInfo->setDestId(getNodeId());
        else
            lteInfo->setDestId(getNextHopNodeId(destAddr, false, lteInfo->getSourceId()));
    }
    else {
        // --- LtePdcpBase (also used by LtePdcpEnb and LtePdcpUe) ---
        // Control Information
        auto lteInfo = pkt->addTagIfAbsent<FlowControlInfo>();

        // Traffic category, RLC type
        LteTrafficClass trafficCategory = getTrafficCategory(pkt);
        LteRlcType rlcType = getRlcType(trafficCategory);
        lteInfo->setTraffic(trafficCategory);
        lteInfo->setRlcType(rlcType);

        // direction of transmitted packets depends on node type
        Direction dir = getNodeTypeById(nodeId_) == UE ? UL : DL;
        lteInfo->setDirection(dir);

        // get IP flow information
        auto ipFlowInd = pkt->getTag<IpFlowInd>();
        Ipv4Address srcAddr = ipFlowInd->getSrcAddr();
        Ipv4Address destAddr = ipFlowInd->getDstAddr();
        uint16_t typeOfService = ipFlowInd->getTypeOfService();
        EV << "Received packet from data port, src= " << srcAddr << " dest=" << destAddr << " ToS=" << typeOfService << endl;

        bool useNR = pkt->getTag<TechnologyReq>()->getUseNR();
        MacNodeId destId = getNextHopNodeId(destAddr, useNR, lteInfo->getSourceId());

        // TODO: Since IP addresses can change when we add and remove nodes, maybe node IDs should be used instead of them
        ConnectionKey key{srcAddr, destAddr, typeOfService, 0xFFFF};
        LogicalCid lcid = lookupOrAssignLcid(key);

        // assign LCID and node IDs
        lteInfo->setLcid(lcid);
        lteInfo->setSourceId(nodeId_);
        lteInfo->setDestId(destId);

        lteInfo->setSourceId(getNodeId());   // TODO CHANGE HERE!!! Must be the NR node ID if this is an NR connection
        if (lteInfo->getMulticastGroupId() != NODEID_NONE)  // destId is meaningless for multicast D2D (we use the id of the source for statistic purposes at lower levels)
            lteInfo->setDestId(getNodeId());
        else
            lteInfo->setDestId(getNextHopNodeId(destAddr, false, lteInfo->getSourceId()));
    }
}

void LtePdcpBase::fromDataPort(cPacket *pktAux)
{
    emit(receivedPacketFromUpperLayerSignal_, pktAux);

    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    analyzePacket(pkt);

    auto lteInfo = pkt->getTag<FlowControlInfo>();
    verifyControlInfo(lteInfo.get());

    MacCid cid = MacCid(lteInfo->getDestId(), lteInfo->getLcid());

    if (isDualConnectivityEnabled() && lteInfo->getMulticastGroupId() == NODEID_NONE) {
        // Handle DC setup: Assume packet arrives in Master nodeB (LTE), and wants to use Secondary nodeB (NR).
        // Packet is processed by local PDCP entity, then needs to be tunneled over X2 to Secondary for transmission.
        // However, local PDCP entity is keyed on LTE nodeIds, so we need to tweak the cid and replace NR nodeId
        // with LTE nodeId so that lookup succeeds.
        if (getNodeTypeById(nodeId_) == NODEB && binder_->isGNodeB(nodeId_) != isNrUe(lteInfo->getDestId()) ) {
            // use another CID whose technology matches the nodeB
            MacNodeId otherDestId = binder_->getUeNodeId(lteInfo->getDestId(), !isNrUe(lteInfo->getDestId()));
            ASSERT(otherDestId != NODEID_NONE);
            cid = MacCid(otherDestId, lteInfo->getLcid());
        }

        // Handle DC setup on UE side: both legs should use the *same* key for entity lookup
        if (getNodeTypeById(nodeId_) == UE && getNodeTypeById(lteInfo->getDestId()) == NODEB)  {
            MacNodeId lteNodeB = binder_->getServingNode(nodeId_);
            cid = MacCid(lteNodeB, lteInfo->getLcid());
        }
    }

    LteTxPdcpEntity *entity = lookupTxEntity(cid);

    // get the PDCP entity for this LCID and process the packet
    EV << "fromDataPort in " << getFullPath() << " event #" << getSimulation()->getEventNumber()
       << ": Processing packet " << pkt->getName() << " src=" << lteInfo->getSourceId() << " dest=" << lteInfo->getDestId()
       << " multicast=" << lteInfo->getMulticastGroupId() << " direction=" << dirToA((Direction)lteInfo->getDirection())
       << " ---> CID " << cid << (entity == nullptr ? " (NEW)" : " (existing)") << std::endl;

    if (entity == nullptr) {
        binder_->establishUnidirectionalDataConnection((FlowControlInfo *)lteInfo.get());
        entity = lookupTxEntity(cid);
        ASSERT(entity != nullptr);
    }

    entity->handlePacketFromUpperLayer(pkt);
}

/*
 * Lower layer handlers
 */

void LtePdcpBase::fromLowerLayer(cPacket *pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);
    emit(receivedPacketFromLowerLayerSignal_, pkt);

    ASSERT(pkt->findTag<PdcpTrackingTag>() == nullptr);

    auto lteInfo = pkt->getTag<FlowControlInfo>();
    MacCid cid = MacCid(lteInfo->getSourceId(), lteInfo->getLcid());

    if (isDualConnectivityEnabled()) {
        // Handle DC setup: Assume packet arrives at this Master nodeB (LTE) from Secondary (NR) over X2.
        // Packet needs to be processed by local PDCP entity. However, local PDCP entity is keyed on LTE nodeIds,
        // so we need to tweak the cid and replace NR nodeId with LTE nodeId so that lookup succeeds.
        if (getNodeTypeById(nodeId_) == NODEB && binder_->isGNodeB(nodeId_) != isNrUe(lteInfo->getSourceId()) ) {
            // use another CID whose technology matches the nodeB
            MacNodeId otherSourceId = binder_->getUeNodeId(lteInfo->getSourceId(), !isNrUe(lteInfo->getSourceId()));
            ASSERT(otherSourceId != NODEID_NONE);
            cid = MacCid(otherSourceId, lteInfo->getLcid());
        }

        // Handle DC setup on UE side: both legs should use the *same* key for entity lookup
        if (getNodeTypeById(nodeId_) == UE && getNodeTypeById(lteInfo->getSourceId()) == NODEB)  {
            MacNodeId lteNodeB = binder_->getServingNode(nodeId_);
            cid = MacCid(lteNodeB, lteInfo->getLcid());
        }
    }

    // Handle DC setup on UE side: UE receives packet from base station
    // and needs to use the correct PDCP entity based on technology matching
    if (getNodeTypeById(nodeId_) == UE && lteInfo->getMulticastGroupId() == NODEID_NONE && isDualConnectivityEnabled()) {
        MacNodeId servingNodeId = binder_->getServingNode(nodeId_);

        // Check if there's a technology mismatch between packet source and UE's serving base station
        if (servingNodeId != NODEID_NONE &&
            binder_->isGNodeB(lteInfo->getSourceId()) != binder_->isGNodeB(servingNodeId)) {

            // Use alternate base station nodeId (Master or Secondary) for proper PDCP entity lookup
            MacNodeId otherSrcId;
            if (binder_->isGNodeB(lteInfo->getSourceId())) {
                // Packet came from gNodeB, get its master (LTE eNodeB)
                otherSrcId = binder_->getMasterNodeOrSelf(lteInfo->getSourceId());
            } else {
                // Packet came from eNodeB, get its secondary (NR gNodeB)
                otherSrcId = binder_->getSecondaryNode(lteInfo->getSourceId());
            }

            if (otherSrcId != NODEID_NONE && otherSrcId != lteInfo->getSourceId()) {
                cid = MacCid(otherSrcId, lteInfo->getLcid());

                EV << "LtePdcp: UE DC RX - Using alternate base station ID " << otherSrcId
                   << " instead of " << lteInfo->getSourceId()
                   << " for technology match with serving node " << servingNodeId << endl;
            }
        }
    }

    LteRxPdcpEntity *entity = lookupRxEntity(cid);

    EV << "fromLowerLayer in " << getFullPath() << " event #" << getSimulation()->getEventNumber()
       <<  ": Processing packet " << pkt->getName() << " src=" << lteInfo->getSourceId() << " dest=" << lteInfo->getDestId()
       << " multicast=" << lteInfo->getMulticastGroupId() << " direction=" << dirToA((Direction)lteInfo->getDirection())
       << " ---> CID " << cid << (entity == nullptr ? " (NEW)" : " (existing)") << std::endl;

    ASSERT(entity != nullptr);

    entity->handlePacketFromLowerLayer(pkt);
}

void LtePdcpBase::toDataPort(cPacket *pktAux)
{
    Enter_Method_Silent("LtePdcpBase::toDataPort");

    auto pkt = check_and_cast<Packet *>(pktAux);
    take(pkt);

    EV << "LtePdcp : Sending packet " << pkt->getName() << " on port upperLayerOut\n";

    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);

    EV << "LtePdcp : Sending packet " << pkt->getName() << " on port upperLayerOut\n";

    // Send message
    send(pkt, upperLayerOutGate_);
    emit(sentPacketToUpperLayerSignal_, pkt);
}

/*
 * Forwarding Handlers
 */

void LtePdcpBase::sendToLowerLayer(Packet *pkt)
{
    auto lteInfo = pkt->getTag<FlowControlInfo>();

    EV << "LtePdcp : Sending packet " << pkt->getName() << " on port " << rlcOutGate_->getFullName() << endl;

    /*
     * @author Alessandro Noferi
     *
     * Since the other methods, e.g. fromData, are overridden
     * in many classes, this method is the only one used by
     * all the classes (except the NRPdcpUe that has its
     * own sendToLowerLayer method).
     * So, the notification about the new PDCP to the pfm
     * is done here.
     *
     * packets sent in D2D mode are not considered
     */

    if (lteInfo->getDirection() != D2D_MULTI && lteInfo->getDirection() != D2D) {
        if (packetFlowObserver_ != nullptr)
            packetFlowObserver_->insertPdcpSdu(pkt);
    }

    // Send message
    send(pkt, rlcOutGate_);
    emit(sentPacketToLowerLayerSignal_, pkt);
}

/*
 * Main functions
 */

void LtePdcpBase::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        upperLayerInGate_ = gate("upperLayerIn");
        upperLayerOutGate_ = gate("upperLayerOut");
        rlcInGate_ = gate("rlcIn");
        rlcOutGate_ = gate("rlcOut");

        binder_.reference(this, "binderModule", true);

        nodeId_ = MacNodeId(getContainingNode(this)->par("macNodeId").intValue());

        packetFlowObserver_.reference(this, "packetFlowObserverModule", false);
        NRpacketFlowObserver_.reference(this, "nrPacketFlowObserverModule", false);

        if (packetFlowObserver_) {
            EV << "LtePdcpBase::initialize - PacketFlowObserver present" << endl;
        }
        if (NRpacketFlowObserver_) {
            EV << "LtePdcpBase::initialize - NRpacketFlowObserver present" << endl;
        }

        conversationalRlc_ = aToRlcType(par("conversationalRlc"));
        interactiveRlc_ = aToRlcType(par("interactiveRlc"));
        streamingRlc_ = aToRlcType(par("streamingRlc"));
        backgroundRlc_ = aToRlcType(par("backgroundRlc"));

        const char *rxEntityModuleTypeName = par("rxEntityModuleType").stringValue();
        rxEntityModuleType_ = cModuleType::get(rxEntityModuleTypeName);

        const char *txEntityModuleTypeName = par("txEntityModuleType").stringValue();
        txEntityModuleType_ = cModuleType::get(txEntityModuleTypeName);

        // TODO WATCH_MAP(gatemap_);
        WATCH(nodeId_);
        WATCH(lcid_);
    }
}

void LtePdcpBase::handleMessage(cMessage *msg)
{
    cPacket *pkt = check_and_cast<cPacket *>(msg);
    EV << "LtePdcp : Received packet " << pkt->getName() << " from port "
       << pkt->getArrivalGate()->getName() << endl;

    cGate *incoming = pkt->getArrivalGate();
    if (incoming == upperLayerInGate_) {
        fromDataPort(pkt);
    }
    else {
        fromLowerLayer(pkt);
    }
}

LteTxPdcpEntity *LtePdcpBase::lookupTxEntity(MacCid cid)
{
    auto it = txEntities_.find(cid);
    return it != txEntities_.end() ? it->second : nullptr;
}

LteTxPdcpEntity *LtePdcpBase::createTxEntity(MacCid cid)
{
    std::stringstream buf;
    buf << "tx-" << cid.getNodeId() << "-" << cid.getLcid();
    LteTxPdcpEntity *txEnt = check_and_cast<LteTxPdcpEntity *>(txEntityModuleType_->createScheduleInit(buf.str().c_str(), this));
    txEntities_[cid] = txEnt;

    EV << "LtePdcpBase::createTxEntity - Added new TxPdcpEntity for Cid: " << cid << "\n";

    return txEnt;
}


LteRxPdcpEntity *LtePdcpBase::lookupRxEntity(MacCid cid)
{
    auto it = rxEntities_.find(cid);
    return it != rxEntities_.end() ? it->second : nullptr;
}

LteRxPdcpEntity *LtePdcpBase::createRxEntity(MacCid cid)
{
    std::stringstream buf;
    buf << "rx-" << cid.getNodeId() << "-" << cid.getLcid();
    LteRxPdcpEntity *rxEnt = check_and_cast<LteRxPdcpEntity *>(rxEntityModuleType_->createScheduleInit(buf.str().c_str(), this));
    rxEntities_[cid] = rxEnt;

    EV << "LtePdcpBase::createRxEntity - Added new RxPdcpEntity for Cid: " << cid << "\n";

    return rxEnt;
}

void LtePdcpEnb::deleteEntities(MacNodeId nodeId)
{
    Enter_Method_Silent();

    // delete connections related to the given UE
    for (auto tit = txEntities_.begin(); tit != txEntities_.end(); ) {
        auto& [cid, txEntity] = *tit;
        if (cid.getNodeId() == nodeId) {
            txEntity->deleteModule();
            tit = txEntities_.erase(tit);
        }
        else {
            ++tit;
        }
    }

    for (auto rit = rxEntities_.begin(); rit != rxEntities_.end(); ) {
        auto& [cid, rxEntity] = *rit;
        if (cid.getNodeId() == nodeId) {
            rxEntity->deleteModule();
            rit = rxEntities_.erase(rit);
        }
        else {
            ++rit;
        }
    }
}

void LtePdcpUe::deleteEntities(MacNodeId nodeId)
{
    // delete all connections TODO: check this (for NR dual connectivity)
    for (auto& [txId, txEntity] : txEntities_) {
        txEntity->deleteModule();  // Delete Entity
    }
    txEntities_.clear(); // Clear all entities after deletion

    for (auto& [rxId, rxEntity] : rxEntities_) {
        rxEntity->deleteModule();  // Delete Entity
    }
    rxEntities_.clear(); // Clear all entities after deletion
}

} //namespace
