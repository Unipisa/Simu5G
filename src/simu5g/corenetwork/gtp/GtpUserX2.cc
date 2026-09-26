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

#include "simu5g/corenetwork/gtp/GtpUserX2.h"
#include "simu5g/stack/dcX2Forwarder/X2DcTunnelInd_m.h"
#include "simu5g/common/LteControlInfo_m.h"

#include <iostream>

#include <inet/common/ProtocolTag_m.h>
#include <inet/networklayer/common/L3Address.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include "simu5g/x2/packet/X2ControlInfo_m.h"

namespace simu5g {

Define_Module(GtpUserX2);

using namespace inet;

void GtpUserX2::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        // register this tunnel endpoint with the Binder, from which the bearer
        // configurator, standing in for the SMF, takes the endpoints it tells about the
        // PDU sessions' tunnels
        MacNodeId bsId = MacNodeId(getContainingNode(this)->par("macNodeId").intValue());
        binder_.reference(this, "binderModule", true);
        binder_->registerX2GtpEndpoint(bsId, this);
        return;
    }

    // wait until all the IP addresses are configured
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;
    localPort_ = par("localPort");

    socket_.setOutputGate(gate("socketOut"));
    socket_.bind(localPort_);

    tunnelPeerPort_ = par("tunnelPeerPort");
}

void GtpUserX2::handleMessage(cMessage *msg)
{
    if (msg->arrivedOn("lteStackIn")) {
        EV << "GtpUserX2::handleMessage - message from X2 Manager" << endl;
        auto pkt = check_and_cast<Packet *>(msg);
        handleFromStack(pkt);
    }
    else if (msg->arrivedOn("socketIn")) {
        EV << "GtpUserX2::handleMessage - message from UDP layer" << endl;
        auto pkt = check_and_cast<Packet *>(msg);
        handleFromUdp(pkt);
    }
}

void GtpUserX2::handleFromStack(Packet *pkt)
{
    // extract destination from the message
    auto x2Msg = pkt->peekAtFront<LteX2Message>();
    X2NodeId destId = x2Msg->getDestinationId();
    X2NodeId srcId = x2Msg->getSourceId();
    ASSERT(getNodeTypeById(srcId) == NODEB);
    ASSERT(getNodeTypeById(destId) == NODEB);
    ASSERT(srcId != destId);
    EV << "GtpUserX2::handleFromStack - Received a LteX2Message with destId[" << destId << "]" << endl;

    auto gtpMsg = makeGtpUserHeader(TEID_NONE, QFI_NONE, PDU_SESSION_CONTAINER_NONE, pkt->getDataLength());
    // forwarded downlink goes on the downlink tunnel of its PDU session at the target,
    // a dual connectivity PDU on its bearer's tunnel at the peer for its direction
    if (x2Msg->getType() == X2_HANDOVER_DATA_MSG) {
        gtpMsg->setTeid(getForwardingTeid(pkt->removeTag<SessionTag>().get(), destId));
        // the End Marker the source relays after the forwarded downlink
        if (pkt->removeTagIfPresent<GtpEndMarkerInd>() != nullptr)
            gtpMsg->setMessageType(GTPU_END_MARKER);
    }
    else if (x2Msg->getType() == X2_DUALCONNECTIVITY_DATA_MSG) {
        auto flow = pkt->removeTag<FlowControlInfo>();
        MacNodeId ueNodeId = flow->getDirection() == DL ? flow->getDestId() : flow->getSourceId();
        auto it = dcTxTeids_.find(std::make_tuple(ueNodeId, flow->getDrbId(), (Direction)flow->getDirection()));
        if (it == dcTxTeids_.end())
            throw cRuntimeError("GtpUserX2: the dual connectivity bearer of UE %d with DRB %d has no X2-U tunnel for direction %d",
                    num(ueNodeId), (int)num(flow->getDrbId()), (int)flow->getDirection());
        gtpMsg->setTeid(it->second);
    }
    pkt->insertAtFront(gtpMsg);
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&LteProtocol::gtp);

    // get the IP address of the destination X2 interface from the Binder
    L3Address peerAddress = binder_->getX2PeerAddress(srcId, destId);
    ASSERT(!peerAddress.isUnspecified());
    socket_.sendTo(pkt, peerAddress, tunnelPeerPort_);
}

void GtpUserX2::handleFromUdp(Packet *pkt)
{
    EV << "GtpUserX2::handleFromUdp - Decapsulating and sending to local connection." << endl;

    // obtain the original X2 message
    auto gtpMsg = pkt->popAtFront<GtpUserMsg>();
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&LteProtocol::x2ap);

    // the tunnel names what the G-PDU carries: forwarded downlink of a PDU session, or a
    // PDCP PDU of a dual connectivity bearer
    Teid teid = gtpMsg->getTeid();
    if (auto it = rxTunnels_.find(teid); it != rxTunnels_.end()) {
        EV << "GtpUserX2::handleFromUdp - Forwarded " << (gtpMsg->getMessageType() == GTPU_END_MARKER ? "End Marker" : "datagram") << " of " << it->second << endl;
        attachSessionTag(pkt, it->second);
        if (gtpMsg->getMessageType() == GTPU_END_MARKER)
            pkt->addTag<GtpEndMarkerInd>();
    }
    else if (auto jt = dcRxTunnels_.find(teid); jt != dcRxTunnels_.end()) {
        auto tunnelInd = pkt->addTag<X2DcTunnelInd>();
        tunnelInd->setUeNodeId(jt->second.ueNodeId);
        tunnelInd->setDrbId(jt->second.drbId);
        tunnelInd->setDirection(jt->second.direction);
    }
    else
        throw cRuntimeError("GtpUserX2: a G-PDU arrived with TEID %u, which is no tunnel ending here", num(teid));

    // send message to the X2 Manager
    send(pkt, "lteStackOut");
}

Teid GtpUserX2::getForwardingTeid(const SessionTag *session, MacNodeId targetBs)
{
    auto it = forwardingTeids_.find(session->getLteNodeId());
    if (it != forwardingTeids_.end()) {
        auto jt = it->second.find(targetBs);
        if (jt != it->second.end())
            return jt->second;
    }
    throw cRuntimeError("GtpUserX2: the PDU session of UE %d has no downlink tunnel at base station %d to forward to",
            num(session->getLteNodeId()), num(targetBs));
}

void GtpUserX2::addTunnel(Teid teid, const SessionRef& session)
{
    Enter_Method_Silent("addTunnel");
    ASSERT(teid != TEID_NONE);
    if (!rxTunnels_.emplace(teid, session).second)
        throw cRuntimeError("GtpUserX2::addTunnel - TEID %u is already in use", num(teid));
}

void GtpUserX2::setForwardingTeid(const SessionRef& session, MacNodeId bsId, Teid teid)
{
    Enter_Method_Silent("setForwardingTeid");
    for (MacNodeId nodeId : {session.lteNodeId, session.nrNodeId})
        if (nodeId != NODEID_NONE)
            forwardingTeids_[nodeId][bsId] = teid;
    EV_INFO << "GtpUserX2::setForwardingTeid - " << session << " is forwarded to base station " << bsId << " with TEID " << teid << endl;
}

void GtpUserX2::removeSession(const SessionRef& session)
{
    Enter_Method_Silent("removeSession");
    for (MacNodeId nodeId : {session.lteNodeId, session.nrNodeId})
        forwardingTeids_.erase(nodeId);
}

void GtpUserX2::addDcTunnel(Teid teid, MacNodeId ueNodeId, DrbId drbId, Direction direction)
{
    Enter_Method_Silent("addDcTunnel");
    ASSERT(teid != TEID_NONE);
    if (rxTunnels_.count(teid) != 0 || !dcRxTunnels_.emplace(teid, DcTunnel{ueNodeId, drbId, direction}).second)
        throw cRuntimeError("GtpUserX2::addDcTunnel - TEID %u is already in use", num(teid));
    EV_INFO << "GtpUserX2::addDcTunnel - TEID " << teid << " carries " << (direction == DL ? "DL" : "UL")
            << " PDCP PDUs of UE " << ueNodeId << ", DRB " << drbId << endl;
}

void GtpUserX2::setDcTunnelTeid(MacNodeId ueLteId, MacNodeId ueNrId, DrbId drbId, Direction direction, Teid teid)
{
    Enter_Method_Silent("setDcTunnelTeid");
    for (MacNodeId nodeId : {ueLteId, ueNrId})
        if (nodeId != NODEID_NONE)
            dcTxTeids_[std::make_tuple(nodeId, drbId, direction)] = teid;
}

} //namespace

