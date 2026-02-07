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
#include "HandoverPacketFilterEnb.h"

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

#include "simu5g/stack/mac/LteMacBase.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/common/cellInfo/CellInfo.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

using namespace std;
using namespace inet;
using namespace omnetpp;

Define_Module(HandoverPacketFilterEnb);

void HandoverPacketFilterEnb::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        stackGateOut_ = gate("stackOut");
        hoManager_.reference(this, "handoverManagerModule", false);
        binder_.reference(this, "binderModule", true);
    }
    else if (stage == INITSTAGE_SIMU5G_REGISTRATIONS) {
        cModule *bs = getContainingNode(this);
        nodeId_ = MacNodeId(bs->par("macNodeId").intValue());
    }
}

void HandoverPacketFilterEnb::handleMessage(cMessage *msg)
{
    auto pkt = check_and_cast<Packet *>(msg);
    if (msg->getArrivalGate()->isName("x2In"))
        receiveTunneledPacketOnHandover(pkt);
    else if (msg->getArrivalGate()->isName("upperLayerIn"))
        fromIpBs(pkt);
    else
        throw cRuntimeError("Message received on wrong gate %s", msg->getArrivalGate()->getFullName());
}

void HandoverPacketFilterEnb::fromIpBs(Packet *pkt)
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

void HandoverPacketFilterEnb::toStackBs(Packet *pkt)
{
    send(pkt, stackGateOut_);
}

void HandoverPacketFilterEnb::triggerHandoverSource(MacNodeId ueId, MacNodeId targetEnb)
{
    EV << NOW << " HandoverPacketFilter::triggerHandoverSource - start tunneling of packets destined to " << ueId << " towards eNB " << targetEnb << endl;

    hoForwarding_[ueId] = targetEnb;

    if (!hoManager_)
        hoManager_.reference(this, "handoverManagerModule", true);

    if (targetEnb != NODEID_NONE)
        hoManager_->sendHandoverCommand(ueId, targetEnb, true);
}

void HandoverPacketFilterEnb::triggerHandoverTarget(MacNodeId ueId, MacNodeId sourceEnb)
{
    EV << NOW << " HandoverPacketFilter::triggerHandoverTarget - start holding packets destined to " << ueId << endl;

    // reception of handover command from X2
    hoHolding_.insert(ueId);
}

void HandoverPacketFilterEnb::sendTunneledPacketOnHandover(Packet *datagram, MacNodeId targetEnb)
{
    EV << "HandoverPacketFilter::sendTunneledPacketOnHandover - destination is handing over to eNB " << targetEnb << ". Forward packet via X2." << endl;
    if (!hoManager_)
        hoManager_.reference(this, "handoverManagerModule", true);
    hoManager_->forwardDataToTargetEnb(datagram, targetEnb);
}

void HandoverPacketFilterEnb::receiveTunneledPacketOnHandover(Packet *datagram)
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

void HandoverPacketFilterEnb::signalHandoverCompleteSource(MacNodeId ueId, MacNodeId targetEnb)
{
    EV << NOW << " HandoverPacketFilter::signalHandoverCompleteSource - handover of UE " << ueId << " to eNB " << targetEnb << " completed!" << endl;
    hoForwarding_.erase(ueId);
}

void HandoverPacketFilterEnb::signalHandoverCompleteTarget(MacNodeId ueId, MacNodeId sourceEnb)
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

HandoverPacketFilterEnb::~HandoverPacketFilterEnb()
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
