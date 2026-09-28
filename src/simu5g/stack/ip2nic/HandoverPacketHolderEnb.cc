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

#include "HandoverPacketHolderEnb.h"

#include <inet/common/ModuleAccess.h>
#include <inet/common/IInterfaceRegistrationListener.h>
#include <inet/common/socket/SocketTag_m.h>
#include <inet/linklayer/common/InterfaceTag_m.h>
#include "simu5g/common/binder/Binder.h"
#include "simu5g/common/InitStages.h"
#include "simu5g/common/L3Utils.h"
#include "simu5g/common/LteControlInfoTags_m.h"
#include "simu5g/stack/handoverX2Forwarder/HandoverX2Forwarder.h"

namespace simu5g {

using namespace inet;
using namespace omnetpp;

Define_Module(HandoverPacketHolderEnb);


HandoverPacketHolderEnb::~HandoverPacketHolderEnb()
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

void HandoverPacketHolderEnb::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        stackGateOut_ = gate("stackOut");
        binder_.reference(this, "binderModule", true);

        cModule *bs = getContainingNode(this);
        nodeId_ = MacNodeId(bs->par("macNodeId").intValue());
    }
    else if (stage == INITSTAGE_SIMU5G_BINDER_ACCESS) {
        amNr_ = binder_->isNrNodeB(nodeId_);
    }
}

MacNodeId HandoverPacketHolderEnb::resolveUeNodeId(const SessionTag *session)
{
    return resolveUeNodeId(session->getLteNodeId(), session->getNrNodeId());
}

MacNodeId HandoverPacketHolderEnb::resolveUeNodeId(MacNodeId lteNodeId, MacNodeId nrNodeId)
{
    // The UE's id on this node's own cell group: an NR node serves, holds and forwards
    // NR ids, an LTE node LTE ids -- a dual-stack UE has both, and picking by this node's
    // technology (rather than LTE-first) is what lets an NR node be the master. The
    // other-stack id is the fallback for a single-stack UE of the other technology.
    MacNodeId destId = amNr_ ? nrNodeId : lteNodeId;
    if (destId == NODEID_NONE)
        destId = amNr_ ? lteNodeId : nrNodeId;
    return destId;
}

void HandoverPacketHolderEnb::handleMessage(cMessage *msg)
{
    auto pkt = check_and_cast<Packet *>(msg);
    if (msg->getArrivalGate()->isName("x2In"))
        receiveTunneledPacketOnHandover(pkt);
    else if (msg->getArrivalGate()->isName("upperLayerIn"))
        fromIpBs(pkt);
    else
        throw cRuntimeError("Message received on wrong gate %s", msg->getArrivalGate()->getFullName());
}

void HandoverPacketHolderEnb::fromIpBs(Packet *pkt)
{
    EV << "HandoverPacketHolder::fromIpBs - message from IP layer: send to stack" << endl;
    // Remove control info from IP datagram
    pkt->removeTagIfPresent<SocketInd>();
    removeAllSimu5GTags(pkt);

    // Remove InterfaceReq Tag (we already are on an interface now)
    pkt->removeTagIfPresent<InterfaceReq>();

    if (pkt->findTag<GtpEndMarkerInd>() != nullptr) {
        relayEndMarker(pkt);
        return;
    }

    // the base station's downlink user-plane entry
    attachIpHeaderFields(pkt);

    // handle "forwarding" of packets during handover
    MacNodeId destId = resolveUeNodeId(pkt->getTag<SessionTag>().get());

    if (hoForwarding_.find(destId) != hoForwarding_.end()) {
        // data packet must be forwarded (via X2) to another eNB
        MacNodeId targetEnb = hoForwarding_.at(destId);
        sendTunneledPacketOnHandover(pkt, targetEnb);
        return;
    }

    // handle incoming packets destined to UEs that are completing handover, or whose
    // handover completed before the End Marker of the old path arrived
    if (hoHolding_.find(destId) != hoHolding_.end() || awaitingEndMarker_.find(destId) != awaitingEndMarker_.end()) {
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

void HandoverPacketHolderEnb::toStackBs(Packet *pkt)
{
    send(pkt, stackGateOut_);
}

void HandoverPacketHolderEnb::triggerHandoverSource(MacNodeId ueId, MacNodeId targetEnb)
{
    EV << NOW << " HandoverPacketHolder::triggerHandoverSource - start tunneling of packets destined to " << ueId << " towards eNB " << targetEnb << endl;

    hoForwarding_[ueId] = targetEnb;

    // A UE that leaves before the End Marker of its previous handover arrived here: the
    // downlink held back for it goes on to the new target, ahead of what follows
    if (awaitingEndMarker_.erase(ueId) != 0) {
        auto it = hoFromIp_.find(ueId);
        if (it != hoFromIp_.end()) {
            IpDatagramQueue& queue = it->second;
            while (!queue.empty()) {
                Packet *pkt = queue.front();
                queue.pop_front();
                take(pkt);
                sendTunneledPacketOnHandover(pkt, targetEnb);
            }
        }
    }
}

void HandoverPacketHolderEnb::triggerHandoverTarget(MacNodeId ueId, MacNodeId sourceEnb)
{
    EV << NOW << " HandoverPacketHolder::triggerHandoverTarget - start holding packets destined to " << ueId << endl;

    // reception of handover command from X2
    hoHolding_.insert(ueId);
}

void HandoverPacketHolderEnb::sendTunneledPacketOnHandover(Packet *datagram, MacNodeId targetEnb)
{
    EV << "HandoverPacketHolder::sendTunneledPacketOnHandover - destination is handing over to eNB " << targetEnb << ". Forward packet via X2." << endl;

    // Add tag with target eNodeB information
    auto tag = datagram->addTagIfAbsent<X2TargetReq>();
    tag->setTargetNode(targetEnb);

    // Send packet to handover manager via gate instead of direct method call
    send(datagram, "hoManagerOut");
}

void HandoverPacketHolderEnb::receiveTunneledPacketOnHandover(Packet *datagram)
{
    EV << "HandoverPacketHolder::receiveTunneledPacketOnHandover - received packet via X2" << endl;
    if (datagram->findTag<GtpEndMarkerInd>() != nullptr) {
        receiveEndMarker(datagram);
        return;
    }

    // the base station's entry for downlink traffic forwarded by the handover source:
    // the forwarding tunnel named the PDU session, and so the UE, the datagram is for
    // (see GtpUserX2)
    attachIpHeaderFields(datagram);
    MacNodeId destId = resolveUeNodeId(datagram->getTag<SessionTag>().get());

    // A datagram the source forwards after the handover has completed here (it reached
    // the source late, see fromIpBs()) goes down right away; the queue below is drained
    // only at completion
    if (hoHolding_.find(destId) == hoHolding_.end() && binder_->getServingNodeOrSelf(destId) == nodeId_) {
        EV << "HandoverPacketHolder::receiveTunneledPacketOnHandover - UE " << destId << " is served here, sending the datagram down" << endl;
        datagram->trim();
        toStackBs(datagram);
        return;
    }

    if (hoFromX2_.find(destId) == hoFromX2_.end()) {
        IpDatagramQueue queue;
        hoFromX2_[destId] = queue;
    }

    hoFromX2_[destId].push_back(datagram);
}

void HandoverPacketHolderEnb::relayEndMarker(Packet *endMarker)
{
    // to the base station this one forwards the UE's downlink to, or else the one the
    // UE is served by now (the late-packet case of fromIpBs())
    MacNodeId ueId = resolveUeNodeId(endMarker->getTag<SessionTag>().get());
    auto it = hoForwarding_.find(ueId);
    MacNodeId targetEnb = (it != hoForwarding_.end()) ? it->second : binder_->getServingNodeOrSelf(ueId);
    if (targetEnb == NODEID_NONE || targetEnb == nodeId_) {
        EV << "HandoverPacketHolder::relayEndMarker - End Marker for UE " << ueId << ", which has not moved to another base station, discarded" << endl;
        delete endMarker;
        return;
    }
    EV << "HandoverPacketHolder::relayEndMarker - relaying the End Marker for UE " << ueId << " to eNB " << targetEnb << endl;
    sendTunneledPacketOnHandover(endMarker, targetEnb);
}

void HandoverPacketHolderEnb::receiveEndMarker(Packet *endMarker)
{
    MacNodeId ueId = resolveUeNodeId(endMarker->getTag<SessionTag>().get());
    delete endMarker;
    if (awaitingEndMarker_.erase(ueId) == 0) {
        // e.g. the UE moved on before it arrived, and the downlink held for it went along
        EV << "HandoverPacketHolder::receiveEndMarker - End Marker for UE " << ueId << ", which is not waiting for one, discarded" << endl;
        return;
    }
    EV << "HandoverPacketHolder::receiveEndMarker - End Marker for UE " << ueId << ": the forwarded downlink is complete" << endl;
    if (hoHolding_.find(ueId) == hoHolding_.end())
        releaseHeldDownlink(ueId);
}

void HandoverPacketHolderEnb::releaseHeldDownlink(MacNodeId ueId)
{
    auto it = hoFromIp_.find(ueId);
    if (it == hoFromIp_.end())
        return;
    IpDatagramQueue& queue = it->second;
    while (!queue.empty()) {
        Packet *pkt = queue.front();
        queue.pop_front();
        take(pkt);
        toStackBs(pkt);
    }
}

void HandoverPacketHolderEnb::signalHandoverCompleteSource(MacNodeId ueId, MacNodeId targetEnb)
{
    EV << NOW << " HandoverPacketHolder::signalHandoverCompleteSource - handover of UE " << ueId << " to eNB " << targetEnb << " completed!" << endl;
    hoForwarding_.erase(ueId);
}

void HandoverPacketHolderEnb::signalHandoverCompleteTarget(MacNodeId ueId)
{
    Enter_Method("signalHandoverCompleteTarget");

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

    // the downlink from the new path follows once the End Marker of the old one has
    // arrived, so none of the forwarded downlink still on its way is overtaken
    if (awaitingEndMarker_.find(ueId) == awaitingEndMarker_.end())
        releaseHeldDownlink(ueId);
    else
        EV << NOW << " HandoverPacketHolder::signalHandoverCompleteTarget - UE " << ueId << ": holding the downlink of the new path until the End Marker" << endl;

    hoHolding_.erase(ueId);
}

void HandoverPacketHolderEnb::switchDownlinkPath(MacNodeId ueLteId, MacNodeId ueNrId, MacNodeId fromBaseStation)
{
    Enter_Method("switchDownlinkPath");
    MacNodeId ueId = resolveUeNodeId(ueLteId, ueNrId);
    // In a handover, the old base station forwards the downlink it gets until the path
    // switch, and relays the End Marker after it (TS 23.502 4.9.1.2.2). Otherwise
    // nothing is forwarded, and a wait left over from an earlier handover is void.
    if (fromBaseStation != NODEID_NONE && fromBaseStation != nodeId_ && hoHolding_.find(ueId) != hoHolding_.end()) {
        EV << NOW << " HandoverPacketHolder::switchDownlinkPath - the downlink of UE " << ueId << " comes here instead of eNB " << fromBaseStation << ", which relays the End Marker" << endl;
        awaitingEndMarker_.insert(ueId);
    }
    else if (awaitingEndMarker_.erase(ueId) != 0 && hoHolding_.find(ueId) == hoHolding_.end())
        releaseHeldDownlink(ueId);
}

} //namespace
