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
#include <inet/common/socket/SocketTag_m.h>
#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/linklayer/common/InterfaceTag_m.h>
#include "simu5g/stack/ip2nic/Ip2Nic.h"
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
        // Initialize flags from PDCP module's NED parameters
        cModule *pdcpModule = networkIf->getSubmodule("pdcp");
        isNR_ = pdcpModule->par("isNR").boolValue();
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

    // TechnologyReq tag (useNR) is already set by TechnologyDecision

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

    // TechnologyReq tag (useNR) is already set by TechnologyDecision

    // Classify the packet and fill FlowControlInfo tag
    analyzePacket(pkt, srcAddr, destAddr, tos);

    send(pkt, stackGateOut_);
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

DrbId Ip2Nic::lookupOrAssignDrbId(const ConnectionKey& key)
{
    auto it = drbIdTable_.find(key);
    if (it != drbIdTable_.end())
        return it->second;
    else {
        DrbId drbId = DrbId(drbId_++);
        drbIdTable_[key] = drbId;
        EV << "Connection not found, new DRB ID created: " << drbId << "\n";
        return drbId;
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
    else {
        // UE variants
        if (!hasD2DSupport_) {
            // UE is subject to handovers, master may change
            return binder_->getNextHop(nodeId_);
        }

        // D2D-capable UE: check if D2D communication is possible
        MacNodeId destId = binder_->getMacNodeId(destAddr);
        MacNodeId srcId = isNR_ ? (useNR ? nrNodeId_ : nodeId_) : nodeId_;

        // check whether the destination is inside the LTE network and D2D is active
        if (destId == NODEID_NONE ||
            !(binder_->getD2DCapability(srcId, destId) && binder_->getD2DMode(srcId, destId) == DM)) {
            // packet is destined to the eNB; UE is subject to handovers: master may change
            return binder_->getNextHop(sourceId);
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
        ConnectionKey key{srcAddr, destAddr, typeOfService, Direction(0xFFFF)};
        DrbId drbId = lookupOrAssignDrbId(key);

        // assign DRB ID and node IDs
        lteInfo->setDrbId(drbId);
        lteInfo->setSourceId(nodeId_);
        lteInfo->setDestId(destId);

        lteInfo->setSourceId(nodeId_);   // TODO CHANGE HERE!!! Must be the NR node ID if this is an NR connection
        if (lteInfo->getMulticastGroupId() != NODEID_NONE)  // destId is meaningless for multicast D2D (we use the id of the source for statistic purposes at lower levels)
            lteInfo->setDestId(nodeId_);
        else
            lteInfo->setDestId(getNextHopNodeId(destAddr, false, lteInfo->getSourceId()));

        if (establishedConnections_.insert({drbId, lteInfo->getDestId()}).second)
            binder_->establishUnidirectionalDataConnection(lteInfo.get());
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
        lteInfo->setD2dTxPeerId(NODEID_NONE); // nem kell
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
        // For PDCP entity dispatch, always use technology-neutral (LTE/master-leg) IDs.
        // The TechnologyReq::useNR flag carries the LTE-vs-NR routing decision separately;
        // NrTxPdcpEntity reads it in deliverPdcpPdu() to decide local RLC vs X2 forwarding.
        if (isEnb) {
            lteInfo->setSourceId(nodeId_);
            if (lteInfo->getMulticastGroupId() != NODEID_NONE)
                lteInfo->setDestId(nodeId_);
            else
                lteInfo->setDestId(getNextHopNodeId(destAddr, !dualConnectivityEnabled_ && useNR, nodeId_));
        }
        else {
            // UE: use LTE UE ID when DC is enabled (both legs share one PDCP entity),
            // NR UE ID when non-DC NR (entity was created with NR IDs)
            MacNodeId ueSourceId = (dualConnectivityEnabled_ ? nodeId_ : (useNR ? nrNodeId_ : nodeId_));
            lteInfo->setSourceId(ueSourceId);
            if (lteInfo->getMulticastGroupId() != NODEID_NONE)
                lteInfo->setDestId(nodeId_);
            else
                lteInfo->setDestId(getNextHopNodeId(destAddr, !dualConnectivityEnabled_ && useNR, ueSourceId));
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

    // --- DRB ID assignment (all D2D subclasses use direction in key) ---
    ConnectionKey key{srcAddr, destAddr, typeOfService, lteInfo->getDirection()};
    DrbId drbId = lookupOrAssignDrbId(key);
    lteInfo->setDrbId(drbId);

    if (establishedConnections_.insert({drbId, lteInfo->getDestId()}).second)
        binder_->establishUnidirectionalDataConnection(lteInfo.get());

    // Debug logging (UE subclasses only)
    if (!isEnb) {
        if (isNR_) {
            EV << "NrPdcpUe : Assigned DRB ID: " << drbId << "\n";
            EV << "NrPdcpUe : Assigned Node ID: " << localNodeId << "\n";
        }
        else {
            EV << "LtePdcpUeD2D : Assigned DRB ID: " << drbId << "\n";
            EV << "LtePdcpUeD2D : Assigned Node ID: " << nodeId_ << "\n";
        }
    }
}

} //namespace
