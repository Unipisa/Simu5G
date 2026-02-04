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
#include <iostream>
#include <inet/common/ModuleAccess.h>
#include <inet/common/IInterfaceRegistrationListener.h>
#include <inet/common/socket/SocketTag_m.h>

#include <inet/transportlayer/tcp_common/TcpHeader.h>
#include <inet/transportlayer/udp/Udp.h>
#include <inet/transportlayer/udp/UdpHeader_m.h>
#include <inet/transportlayer/common/L4Tools.h>

#include <inet/networklayer/common/NetworkInterface.h>
#include <inet/networklayer/ipv4/Ipv4InterfaceData.h>
#include <inet/networklayer/ipv4/Ipv4Route.h>
#include <inet/networklayer/ipv4/IIpv4RoutingTable.h>
#include <inet/networklayer/common/L3Tools.h>
#include <inet/networklayer/ipv4/Ipv4Header_m.h>

#include <inet/linklayer/common/InterfaceTag_m.h>

#include "simu5g/stack/ip2nic/HandoverPacketFilter.h"
#include "simu5g/stack/mac/LteMacBase.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/common/cellInfo/CellInfo.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

using namespace std;
using namespace inet;
using namespace omnetpp;

Define_Module(HandoverPacketFilter);

void HandoverPacketFilter::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        stackGateOut_ = gate("stackOut");

        setNodeType(par("nodeType").stdstringValue());

        hoManager_.reference(this, "handoverManagerModule", false);

        binder_.reference(this, "binderModule", true);

        networkIf = getContainingNicModule(this);
    }
    else if (stage == INITSTAGE_SIMU5G_REGISTRATIONS) {
        if (nodeType_ == NODEB) {
            cModule *bs = getContainingNode(this);
            nodeId_ = MacNodeId(bs->par("macNodeId").intValue());
        }
        else if (nodeType_ == UE) {
            cModule *ue = getContainingNode(this);
            servingNodeId_ = MacNodeId(ue->par("servingNodeId").intValue());
            nodeId_ = MacNodeId(ue->par("macNodeId").intValue());

            if (ue->hasPar("nrServingNodeId") && ue->par("nrServingNodeId").intValue() != 0) { // register also the NR MacNodeId
                nrServingNodeId_ = MacNodeId(ue->par("nrServingNodeId").intValue());
                nrNodeId_ = MacNodeId(ue->par("nrMacNodeId").intValue());
            }
        }
    }
}

void HandoverPacketFilter::handleMessage(cMessage *msg)
{
    if (msg->getArrivalGate()->isName("x2In")) {
        ASSERT(nodeType_ == NODEB);
        auto datagram = check_and_cast<Packet *>(msg);
        receiveTunneledPacketOnHandover(datagram);
        return;
    }
    else if (msg->getArrivalGate()->isName("upperLayerIn")) {
        auto ipDatagram = check_and_cast<Packet *>(msg);
        if (nodeType_ == NODEB)
            fromIpBs(ipDatagram);
        else if (nodeType_ == UE)
            fromIpUe(ipDatagram);
    }
    else {
        throw cRuntimeError("Message received on wrong gate %s", msg->getArrivalGate()->getFullName());
    }
}

void HandoverPacketFilter::setNodeType(std::string s)
{
    nodeType_ = aToNodeType(s);
    EV << "Node type: " << s << endl;
}

void HandoverPacketFilter::fromIpUe(Packet *datagram)
{
    EV << "HandoverPacketFilter::fromIpUe - message from IP layer: send to stack: " << datagram->str() << std::endl;
    // Remove control info from IP datagram
    datagram->removeTagIfPresent<SocketInd>();
    removeAllSimu5GTags(datagram);

    // Remove InterfaceReq Tag (we already are on an interface now)
    datagram->removeTagIfPresent<InterfaceReq>();

    if (ueHold_) {
        // hold packets until handover is complete
        ueHoldFromIp_.push_back(datagram);
    }
    else {
        if (servingNodeId_ == NODEID_NONE && nrServingNodeId_ == NODEID_NONE) { // UE is detached
            EV << "HandoverPacketFilter::fromIpUe - UE is not attached to any serving node. Delete packet." << endl;
            delete datagram;
        }
        else
            toStackUe(datagram);
    }
}

void HandoverPacketFilter::toStackUe(Packet *pkt)
{
    send(pkt, stackGateOut_);
}

void HandoverPacketFilter::prepareForIpv4(Packet *datagram, const Protocol *protocol) {
    // add DispatchProtocolRequest so that the packet is handled by the specified protocol
    datagram->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(protocol);
    datagram->addTagIfAbsent<PacketProtocolTag>()->setProtocol(protocol);
    // add Interface-Indication to indicate which interface this packet was received from
    datagram->addTagIfAbsent<InterfaceInd>()->setInterfaceId(networkIf->getInterfaceId());
}

void HandoverPacketFilter::fromIpBs(Packet *pkt)
{
    EV << "HandoverPacketFilter::fromIpBs - message from IP layer: send to stack" << endl;
    // Remove control info from IP datagram
    pkt->removeTagIfPresent<SocketInd>();
    removeAllSimu5GTags(pkt);

    // Remove InterfaceReq Tag (we already are on an interface now)
    pkt->removeTagIfPresent<InterfaceReq>();

    // TODO: Add support for IPv6
    auto ipHeader = pkt->peekAtFront<Ipv4Header>();
    const Ipv4Address& destAddr = ipHeader->getDestAddress();

    // handle "forwarding" of packets during handover
    MacNodeId destId = binder_->getMacNodeId(destAddr);
    if (destId == NODEID_NONE)
        destId = binder_->getNrMacNodeId(destAddr);

    if (hoForwarding_.find(destId) != hoForwarding_.end()) {
        // data packet must be forwarded (via X2) to another eNB
        MacNodeId targetEnb = hoForwarding_.at(destId);
        sendTunneledPacketOnHandover(pkt, targetEnb);
        return;
    }

    // handle incoming packets destined to UEs that are completing handover
    if (hoHolding_.find(destId) != hoHolding_.end()) {
        // hold packets until handover is complete
        if (hoFromIp_.find(destId) == hoFromIp_.end()) {
            IpDatagramQueue queue;
            hoFromIp_[destId] = queue;
        }

        hoFromIp_[destId].push_back(pkt);
        return;
    }

    // if UE has moved to another gNB (e.g. late packet after handover), forward via X2
    MacNodeId servingNode = binder_->getServingNodeOrSelf(destId);
    if (servingNode != NODEID_NONE && servingNode != nodeId_) {
        EV << "Ip2Nic::fromIpBs - UE " << destId << " is served by gNB " << servingNode << ", forwarding via X2" << endl;
        sendTunneledPacketOnHandover(pkt, servingNode);
        return;
    }

    toStackBs(pkt);
}

void HandoverPacketFilter::toStackBs(Packet *pkt)
{
    send(pkt, stackGateOut_);
}

void HandoverPacketFilter::printControlInfo(Packet *pkt)
{
    EV << "Src IP : " << pkt->getTag<IpFlowInd>()->getSrcAddr() << endl;
    EV << "Dst IP : " << pkt->getTag<IpFlowInd>()->getDstAddr() << endl;
    EV << "ToS : " << pkt->getTag<IpFlowInd>()->getTypeOfService() << endl;
}

void HandoverPacketFilter::triggerHandoverSource(MacNodeId ueId, MacNodeId targetEnb)
{
    EV << NOW << " HandoverPacketFilter::triggerHandoverSource - start tunneling of packets destined to " << ueId << " towards eNB " << targetEnb << endl;

    hoForwarding_[ueId] = targetEnb;

    if (!hoManager_)
        hoManager_.reference(this, "handoverManagerModule", true);

    if (targetEnb != NODEID_NONE)
        hoManager_->sendHandoverCommand(ueId, targetEnb, true);
}

void HandoverPacketFilter::triggerHandoverTarget(MacNodeId ueId, MacNodeId sourceEnb)
{
    EV << NOW << " HandoverPacketFilter::triggerHandoverTarget - start holding packets destined to " << ueId << endl;

    // reception of handover command from X2
    hoHolding_.insert(ueId);
}

void HandoverPacketFilter::sendTunneledPacketOnHandover(Packet *datagram, MacNodeId targetEnb)
{
    EV << "HandoverPacketFilter::sendTunneledPacketOnHandover - destination is handing over to eNB " << targetEnb << ". Forward packet via X2." << endl;
    if (!hoManager_)
        hoManager_.reference(this, "handoverManagerModule", true);
    hoManager_->forwardDataToTargetEnb(datagram, targetEnb);
}

void HandoverPacketFilter::receiveTunneledPacketOnHandover(Packet *datagram)
{
    EV << "HandoverPacketFilter::receiveTunneledPacketOnHandover - received packet via X2" << endl;
    const auto& hdr = datagram->peekAtFront<Ipv4Header>();
    const Ipv4Address& destAddr = hdr->getDestAddress();
    MacNodeId destId = binder_->getMacNodeId(destAddr);
    if (destId == NODEID_NONE)
        destId = binder_->getNrMacNodeId(destAddr);

    if (hoFromX2_.find(destId) == hoFromX2_.end()) {
        IpDatagramQueue queue;
        hoFromX2_[destId] = queue;
    }

    hoFromX2_[destId].push_back(datagram);
}

void HandoverPacketFilter::signalHandoverCompleteSource(MacNodeId ueId, MacNodeId targetEnb)
{
    EV << NOW << " HandoverPacketFilter::signalHandoverCompleteSource - handover of UE " << ueId << " to eNB " << targetEnb << " completed!" << endl;
    hoForwarding_.erase(ueId);
}

void HandoverPacketFilter::signalHandoverCompleteTarget(MacNodeId ueId, MacNodeId sourceEnb)
{
    Enter_Method("signalHandoverCompleteTarget");

    // signal the event to the source eNB
    if (!hoManager_)
        hoManager_.reference(this, "handoverManagerModule", true);
    hoManager_->sendHandoverCommand(ueId, sourceEnb, false);

    // send down buffered packets in the following order:
    // 1) packets received from X2
    // 2) packets received from IP

    if (hoFromX2_.find(ueId) != hoFromX2_.end()) {
        IpDatagramQueue& queue = hoFromX2_[ueId];
        while (!queue.empty()) {
            Packet *pkt = queue.front();
            queue.pop_front();

            // send pkt down
            take(pkt);
            pkt->trim();
            toStackBs(pkt);
        }
    }

    if (hoFromIp_.find(ueId) != hoFromIp_.end()) {
        IpDatagramQueue& queue = hoFromIp_[ueId];
        while (!queue.empty()) {
            Packet *pkt = queue.front();
            queue.pop_front();

            // send pkt down
            take(pkt);
            toStackBs(pkt);
        }
    }

    hoHolding_.erase(ueId);
}

void HandoverPacketFilter::triggerHandoverUe(MacNodeId newMasterId, bool isNr)
{
    EV << NOW << " HandoverPacketFilter::triggerHandoverUe - start holding packets" << endl;

    if (newMasterId != NODEID_NONE) {
        ueHold_ = true;
        if (isNr)
            nrServingNodeId_ = newMasterId;
        else
            servingNodeId_ = newMasterId;
    }
    else {
        if (isNr)
            nrServingNodeId_ = NODEID_NONE;
        else
            servingNodeId_ = NODEID_NONE;
    }
}

void HandoverPacketFilter::signalHandoverCompleteUe(bool isNr)
{
    Enter_Method("signalHandoverCompleteUe");

    if ((!isNr && servingNodeId_ != NODEID_NONE) || (isNr && nrServingNodeId_ != NODEID_NONE)) {
        // send held packets
        while (!ueHoldFromIp_.empty()) {
            auto pkt = ueHoldFromIp_.front();
            ueHoldFromIp_.pop_front();

            // send pkt down
            take(pkt);
            toStackUe(pkt);
        }
        ueHold_ = false;
    }
}

HandoverPacketFilter::~HandoverPacketFilter()
{
    for (auto &[macNodeId, ipDatagramQueue] : hoFromX2_) {
        while (!ipDatagramQueue.empty()) {
            Packet *pkt = ipDatagramQueue.front();
            ipDatagramQueue.pop_front();
            delete pkt;
        }
    }

    for (auto &[macNodeId, ipDatagramQueue] : hoFromIp_) {
        while (!ipDatagramQueue.empty()) {
            Packet *pkt = ipDatagramQueue.front();
            ipDatagramQueue.pop_front();
            delete pkt;
        }
    }
}

} //namespace
