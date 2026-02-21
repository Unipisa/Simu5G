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
#include <inet/common/socket/SocketTag_m.h>
#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/linklayer/common/InterfaceTag_m.h>
#include "simu5g/stack/ip2nic/Ip2Nic.h"
#include "simu5g/stack/ip2nic/HandoverPacketHolderUe.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

using namespace inet;
using namespace omnetpp;

Define_Module(Ip2Nic);

Ip2Nic::~Ip2Nic()
{
}

void Ip2Nic::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        stackGateOut_ = gate("stackOut");
        ipGateOut_ = gate("upperLayerOut");

        nodeType_ = aToNodeType(par("nodeType").stdstringValue());

        binder_.reference(this, "binderModule", true);

        networkIf = getContainingNicModule(this);
        dualConnectivityEnabled_ = networkIf->par("dualConnectivityEnabled").boolValue();

        if (nodeType_ == NODEB) {
            cModule *bs = getContainingNode(this);
            nodeId_ = MacNodeId(bs->par("macNodeId").intValue());
        }
        else if (nodeType_ == UE) {
            cModule *ue = getContainingNode(this);
            nodeId_ = MacNodeId(ue->par("macNodeId").intValue());
            if (ue->hasPar("nrMacNodeId"))
                nrNodeId_ = MacNodeId(ue->par("nrMacNodeId").intValue());
        }
    }
    else if (stage == INITSTAGE_SIMU5G_BINDER_ACCESS) {
        if (nodeType_ == UE) {
            // get serving node IDs -- note the this is STLL not late enough to pick up the result of dynamic cell association
            if (nodeId_ != NODEID_NONE)
                servingNodeId_ = binder_->getServingNode(nodeId_);
            if (nrNodeId_ != NODEID_NONE)
                nrServingNodeId_ = binder_->getServingNode(nrNodeId_);
        }

        // Initialize flags using the same method as PDCP subclasses
        cModule *pdcpModule = networkIf->getSubmodule("pdcp");
        isNR_ = (strcmp(pdcpModule->getNedTypeName(), "simu5g.stack.pdcp.NrPdcpEnb") == 0)
             || (strcmp(pdcpModule->getNedTypeName(), "simu5g.stack.pdcp.NrPdcpUe") == 0);
        hasD2DSupport_ = networkIf->par("d2dCapable").boolValue() || isNR_;

        conversationalRlc_ = aToRlcType(par("conversationalRlc"));
        interactiveRlc_ = aToRlcType(par("interactiveRlc"));
        streamingRlc_ = aToRlcType(par("streamingRlc"));
        backgroundRlc_ = aToRlcType(par("backgroundRlc"));
    }
}

void Ip2Nic::handleMessage(cMessage *msg)
{
    if (msg->getArrivalGate()->isName("upperLayerIn")) {
        if (nodeType_ == NODEB)
            toStackBs(check_and_cast<Packet*>(msg));
        else {
            toStackUe(check_and_cast<Packet*>(msg));
        }
    }
    else if (msg->getArrivalGate()->isName("stackIn")) {
        EV << "Ip2Nic: message from stack: sending up" << endl;
        auto pkt = check_and_cast<Packet *>(msg);
        pkt->removeTagIfPresent<SocketInd>();
        removeAllSimu5GTags(pkt);
        if (nodeType_ == NODEB)
            toIpBs(pkt);
        else
            toIpUe(pkt);
    }
    else {
        throw cRuntimeError("Message received on wrong gate %s", msg->getArrivalGate()->getFullName());
    }
}

void Ip2Nic::toStackUe(Packet *pkt)
{
    EV << "Ip2Nic::fromIpUe - message from IP layer: send to stack: " << pkt->str() << std::endl;
    auto ipHeader = pkt->peekAtFront<Ipv4Header>();
    auto srcAddr = ipHeader->getSrcAddress();
    auto destAddr = ipHeader->getDestAddress();
    short int tos = ipHeader->getTypeOfService();

    // TODO: Add support for IPv6 (=> see L3Tools.cc of INET)

    auto ipFlowInd = pkt->addTagIfAbsent<IpFlowInd>();
    ipFlowInd->setSrcAddr(srcAddr);
    ipFlowInd->setDstAddr(destAddr);
    ipFlowInd->setTypeOfService(tos);
    // mark packet for using NR
    bool useNR;
    if (!markPacket(srcAddr, destAddr, tos, useNR)) {
        EV << "Ip2Nic::toStackUe - UE is not attached to any serving node. Delete packet." << endl;
        delete pkt;
        return;
    }

    // Set useNR on the packet control info
    pkt->addTagIfAbsent<TechnologyReq>()->setUseNR(useNR);

    // Classify the packet and fill FlowControlInfo tag
    analyzePacket(pkt, srcAddr, destAddr, tos);

    // Send datagram to LTE stack or LteIp peer
    send(pkt, stackGateOut_);
}

void Ip2Nic::prepareForIpv4(Packet *datagram, const Protocol *protocol) {
    // add DispatchProtocolRequest so that the packet is handled by the specified protocol
    datagram->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(protocol);
    datagram->addTagIfAbsent<PacketProtocolTag>()->setProtocol(protocol);
    // add Interface-Indication to indicate which interface this packet was received from
    datagram->addTagIfAbsent<InterfaceInd>()->setInterfaceId(networkIf->getInterfaceId());
}

void Ip2Nic::toIpUe(Packet *pkt)
{
    auto ipHeader = pkt->peekAtFront<Ipv4Header>();
    auto networkProtocolInd = pkt->addTagIfAbsent<NetworkProtocolInd>();
    networkProtocolInd->setProtocol(&Protocol::ipv4);
    networkProtocolInd->setNetworkProtocolHeader(ipHeader);
    prepareForIpv4(pkt);
    EV << "Ip2Nic::toIpUe - message from stack: send to IP layer" << endl;
    send(pkt, ipGateOut_);
}

void Ip2Nic::toIpBs(Packet *pkt)
{
    auto ipHeader = pkt->peekAtFront<Ipv4Header>();
    auto networkProtocolInd = pkt->addTagIfAbsent<NetworkProtocolInd>();
    networkProtocolInd->setProtocol(&Protocol::ipv4);
    networkProtocolInd->setNetworkProtocolHeader(ipHeader);
    prepareForIpv4(pkt, &LteProtocol::ipv4uu);
    EV << "Ip2Nic::toIpBs - message from stack: send to IP layer" << endl;
    send(pkt, ipGateOut_);
}

void Ip2Nic::toStackBs(Packet *pkt)
{
    EV << "Ip2Nic::toStackBs - message from IP layer: send to stack" << endl;
    removeAllSimu5GTags(pkt);
    auto ipHeader = pkt->peekAtFront<Ipv4Header>();
    auto srcAddr = ipHeader->getSrcAddress();
    auto destAddr = ipHeader->getDestAddress();
    short int tos = ipHeader->getTypeOfService();

    // prepare flow info for NIC
    auto ipFlowInd = pkt->addTagIfAbsent<IpFlowInd>();
    ipFlowInd->setSrcAddr(srcAddr);
    ipFlowInd->setDstAddr(destAddr);
    ipFlowInd->setTypeOfService(tos);

    // mark packet for using NR
    bool useNR;
    if (!markPacket(srcAddr, destAddr, tos, useNR)) {
        EV << "Ip2Nic::toStackBs - UE is not attached to any serving node. Delete packet." << endl;
        delete pkt;
    }
    else {
        // Set useNR on the packet control info
        pkt->addTagIfAbsent<TechnologyReq>()->setUseNR(useNR);

        // Classify the packet and fill FlowControlInfo tag
        analyzePacket(pkt, srcAddr, destAddr, tos);

        send(pkt, stackGateOut_);
    }
}

bool Ip2Nic::markPacket(inet::Ipv4Address srcAddr, inet::Ipv4Address dstAddr, uint16_t typeOfService, bool& useNR)
{
    // In the current version, the Ip2Nic module of the master eNB (the UE) selects which path
    // to follow based on the Type of Service (TOS) field:
    // - use master eNB if tos < 10
    // - use secondary gNB if 10 <= tos < 20
    // - use split bearer if tos >= 20
    //
    // To change the policy, change the implementation of the Ip2Nic::markPacket() function
    //
    // TODO use a better policy
    // TODO make it configurable from INI or XML?

    if (nodeType_ == NODEB) {
        MacNodeId ueId = binder_->getMacNodeId(dstAddr);
        MacNodeId nrUeId = binder_->getNrMacNodeId(dstAddr);
        bool ueLteStack = (binder_->getServingNodeOrSelf(ueId) != NODEID_NONE);
        bool ueNrStack = (binder_->getServingNodeOrSelf(nrUeId) != NODEID_NONE);

        if (dualConnectivityEnabled_ && ueLteStack && ueNrStack && typeOfService >= 20) { // use split bearer TODO fix threshold
            // even packets go through the LTE eNodeB
            // odd packets go through the gNodeB

            FlowKey key{srcAddr.getInt(), dstAddr.getInt(), typeOfService};
            int sentPackets = splitBearersTable_[key]++;

            if (sentPackets % 2 == 0)
                useNR = false;
            else
                useNR = true;
        }
        else {
            if (ueLteStack && ueNrStack) {
                if (typeOfService >= 10)                                                     // use secondary cell group bearer TODO fix threshold
                    useNR = true;
                else                                                  // use master cell group bearer
                    useNR = false;
            }
            else if (ueLteStack)
                useNR = false;
            else if (ueNrStack)
                useNR = true;
            else
                return false;
        }
    }

    if (nodeType_ == UE) {
        bool ueLteStack = (binder_->getServingNodeOrSelf(nodeId_) != NODEID_NONE);
        bool ueNrStack = (binder_->getServingNodeOrSelf(nrNodeId_) != NODEID_NONE);
        if (dualConnectivityEnabled_ && ueLteStack && ueNrStack && typeOfService >= 20) { // use split bearer TODO fix threshold
            FlowKey key{srcAddr.getInt(), dstAddr.getInt(), typeOfService};
            int sentPackets = splitBearersTable_[key]++;

            if (sentPackets % 2 == 0)
                useNR = false;
            else
                useNR = true;
        }
        else {
            // KLUDGE: this is necessary to prevent a runtime error in one of the simulations:
            //
            //    test_numerology, multicell_CBR_UL, ue[9], t=0.001909132428, event #475 (HandoverPacketHolder), #476 (Ip2Nic)
            //    after: ueLteStack=true, ueNrStack=true, servingNodeId=1, nrServingNodeId=2, typeOfService=10 --> useNr = true (ip2nic.cc#319: UE branch, not dualconn("else") )
            //    before: ueLteStack=true, ueNrStack=true,servingNodeId=1, nrServingNodeId=0, typeOfService=10 --> useNr = false, (HandoverPacketHolder.cc#360)
            //
            auto handoverPacketHolder = check_and_cast<HandoverPacketHolderUe*>(getParentModule()->getSubmodule("handoverPacketHolder"));
            servingNodeId_ = handoverPacketHolder->getServingNodeId();
            nrServingNodeId_ = handoverPacketHolder->getNrServingNodeId();
            // servingNodeId_ = binder_->getServingNode(nodeId_);
            // nrServingNodeId_ = binder_->getServingNode(nrNodeId_);

            if (servingNodeId_ == NODEID_NONE && nrServingNodeId_ != NODEID_NONE)  // 5G-connected only
                useNR = true;
            else if (servingNodeId_ != NODEID_NONE && nrServingNodeId_ == NODEID_NONE)  // LTE-connected only
                useNR = false;
            else // both
                useNR = (typeOfService >= 10);   // use secondary cell group bearer TODO fix threshold
        }
    }
    return true;
}

LteTrafficClass Ip2Nic::getTrafficCategory(cPacket *pkt)
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

LteRlcType Ip2Nic::getRlcType(LteTrafficClass trafficCategory)
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

LogicalCid Ip2Nic::lookupOrAssignLcid(const ConnectionKey& key)
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

MacNodeId Ip2Nic::getNextHopNodeId(const Ipv4Address& destAddr, bool useNR, MacNodeId sourceId)
{
    bool isEnb = (nodeType_ == NODEB);

    if (isEnb) {
        // ENB variants
        MacNodeId destId;
        if (isNR_ && (!dualConnectivityEnabled_ || useNR))
            destId = binder_->getNrMacNodeId(destAddr);
        else
            destId = binder_->getMacNodeId(destAddr);

        // master of this UE (myself)
        MacNodeId master = binder_->getServingNodeOrSelf(destId);
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
    else {
        // UE variants
        if (!hasD2DSupport_) {
            // UE is subject to handovers, master may change
            return binder_->getServingNodeOrSelf(nodeId_);
        }

        // D2D-capable UE: check if D2D communication is possible
        MacNodeId destId = binder_->getMacNodeId(destAddr);
        MacNodeId srcId = isNR_ ? (useNR ? nrNodeId_ : nodeId_) : nodeId_;

        // check whether the destination is inside the LTE network and D2D is active
        if (destId == NODEID_NONE ||
            !(binder_->getD2DCapability(srcId, destId) && binder_->getD2DMode(srcId, destId) == DM)) {
            // packet is destined to the eNB; UE is subject to handovers: master may change
            return binder_->getServingNodeOrSelf(sourceId);
        }

        return destId;
    }
}

void Ip2Nic::analyzePacket(inet::Packet *pkt, Ipv4Address srcAddr, Ipv4Address destAddr, uint16_t typeOfService)
{
    // --- Common preamble ---
    auto lteInfo = pkt->addTagIfAbsent<FlowControlInfo>();

    // Traffic category, RLC type
    LteTrafficClass trafficCategory = getTrafficCategory(pkt);
    LteRlcType rlcType = getRlcType(trafficCategory);
    lteInfo->setTraffic(trafficCategory);
    lteInfo->setRlcType(rlcType);

    // direction of transmitted packets depends on node type
    Direction dir = (nodeType_ == UE) ? UL : DL;
    lteInfo->setDirection(dir);

    bool useNR = pkt->getTag<TechnologyReq>()->getUseNR();
    bool isEnb = (dir == DL);

    // --- Base LtePdcpEnb/LtePdcpUe (no D2D support) ---
    if (!hasD2DSupport_) {
        EV << "Received packet from data port, src= " << srcAddr << " dest=" << destAddr << " ToS=" << typeOfService << endl;

        MacNodeId destId = getNextHopNodeId(destAddr, useNR, lteInfo->getSourceId());

        // TODO: Since IP addresses can change when we add and remove nodes, maybe node IDs should be used instead of them
        ConnectionKey key{srcAddr, destAddr, typeOfService, 0xFFFF};
        LogicalCid lcid = lookupOrAssignLcid(key);

        // assign LCID and node IDs
        lteInfo->setLcid(lcid);
        lteInfo->setSourceId(nodeId_);
        lteInfo->setDestId(destId);

        lteInfo->setSourceId(nodeId_);   // TODO CHANGE HERE!!! Must be the NR node ID if this is an NR connection
        if (lteInfo->getMulticastGroupId() != NODEID_NONE)  // destId is meaningless for multicast D2D (we use the id of the source for statistic purposes at lower levels)
            lteInfo->setDestId(nodeId_);
        else
            lteInfo->setDestId(getNextHopNodeId(destAddr, false, lteInfo->getSourceId()));
        return;
    }

    // --- D2D-capable subclasses (LtePdcpEnbD2D, LtePdcpUeD2D, NrPdcpEnb, NrPdcpUe) ---

    // For NrPdcpUe, the effective local node ID depends on useNR flag
    MacNodeId localNodeId = (isNR_ && !isEnb) ? (useNR ? nrNodeId_ : nodeId_) : nodeId_;

    // EV log (all D2D subclasses except NrPdcpUe)
    if (isEnb || !isNR_)
        EV << "Received packet from data port, src= " << srcAddr << " dest=" << destAddr << " ToS=" << typeOfService << endl;

    if (isEnb) {
        // ENB: set D2D peer IDs to none
        lteInfo->setD2dTxPeerId(NODEID_NONE);
        lteInfo->setD2dRxPeerId(NODEID_NONE);
    }
    else {
        // UE: D2D multicast/unicast handling (unified for NrPdcpUe and LtePdcpUeD2D)
        if (isNR_)
            lteInfo->setSourceId(localNodeId);

        if (destAddr.isMulticast()) {
            binder_->addD2DMulticastTransmitter(localNodeId);
            lteInfo->setDirection(D2D_MULTI);
            MacNodeId groupId = binder_->getOrAssignDestIdForMulticastAddress(destAddr);
            lteInfo->setMulticastGroupId(groupId);
        }
        else {
            MacNodeId destId = binder_->getMacNodeId(destAddr);
            if (destId != NODEID_NONE) { // the destination is a UE within the LTE network
                if (binder_->checkD2DCapability(localNodeId, destId)) {
                    // this way, we record the ID of the endpoints even if the connection is currently in IM
                    // this is useful for mode switching
                    lteInfo->setD2dTxPeerId(localNodeId);
                    lteInfo->setD2dRxPeerId(destId);
                }
                else {
                    lteInfo->setD2dTxPeerId(NODEID_NONE);
                    lteInfo->setD2dRxPeerId(NODEID_NONE);
                }

                // set actual flow direction (D2D/UL) based on the current mode (DM/IM) of this peering
                if (binder_->getD2DCapability(localNodeId, destId) && binder_->getD2DMode(localNodeId, destId) == DM)
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
    }

    // --- Source and Dest IDs ---
    if (isNR_) {
        lteInfo->setSourceId(isEnb ? nodeId_ : (useNR ? nrNodeId_ : nodeId_));

        if (lteInfo->getMulticastGroupId() != NODEID_NONE)
            lteInfo->setDestId(nodeId_);
        else
            lteInfo->setDestId(getNextHopNodeId(destAddr, useNR, lteInfo->getSourceId()));

        // NrPdcpEnb only: Dual Connectivity adjustment
        if (isEnb) {
            MacNodeId secondaryNodeId = binder_->getSecondaryNode(nodeId_);
            if (dualConnectivityEnabled_ && secondaryNodeId != NODEID_NONE && useNR) {
                lteInfo->setSourceId(secondaryNodeId);
                lteInfo->setDestId(binder_->getNrMacNodeId(destAddr));
            }
        }
    }
    else {
        // LtePdcpEnbD2D / LtePdcpUeD2D
        lteInfo->setSourceId(nodeId_);
        if (!isEnb) // LtePdcpUeD2D: dead getNextHopNodeId call (result unused in original code)
            (void)getNextHopNodeId(destAddr, useNR, lteInfo->getSourceId());

        lteInfo->setSourceId(nodeId_);   // TODO CHANGE HERE!!! Must be the NR node ID if this is an NR connection
        if (lteInfo->getMulticastGroupId() != NODEID_NONE)  // destId is meaningless for multicast D2D
            lteInfo->setDestId(nodeId_);
        else
            lteInfo->setDestId(getNextHopNodeId(destAddr, false, lteInfo->getSourceId()));
    }

    // --- LCID assignment (all D2D subclasses use direction in key) ---
    ConnectionKey key{srcAddr, destAddr, typeOfService, lteInfo->getDirection()};
    LogicalCid lcid = lookupOrAssignLcid(key);
    lteInfo->setLcid(lcid);

    // Debug logging (UE subclasses only)
    if (!isEnb) {
        if (isNR_) {
            EV << "NrPdcpUe : Assigned Lcid: " << lcid << "\n";
            EV << "NrPdcpUe : Assigned Node ID: " << localNodeId << "\n";
        }
        else {
            EV << "LtePdcpUeD2D : Assigned Lcid: " << lcid << "\n";
            EV << "LtePdcpUeD2D : Assigned Node ID: " << nodeId_ << "\n";
        }
    }
}

} //namespace
