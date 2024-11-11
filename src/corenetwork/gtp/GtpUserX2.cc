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

#include "corenetwork/gtp/GtpUserX2.h"

#include <iostream>

#include <inet/common/ProtocolTag_m.h>
#include <inet/networklayer/common/L3Address.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include "x2/packet/X2ControlInfo_m.h"

namespace simu5g {

Define_Module(GtpUserX2);

using namespace inet;

void GtpUserX2::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    // wait until all the IP addresses are configured
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;
    localPort_ = par("localPort");

    // get reference to the binder
    binder_.reference(this, "binderModule", true);

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
    X2NodeId destId = MacNodeId(x2Msg->getDestinationId());
    X2NodeId srcId = MacNodeId(x2Msg->getSourceId());
    EV << "GtpUserX2::handleFromStack - Received a LteX2Message with destId[" << destId << "]" << endl;

    auto gtpMsg = makeShared<GtpUserMsg>();
    pkt->insertAtFront(gtpMsg);
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&LteProtocol::gtp);

    // get the IP address of the destination X2 interface from the Binder
    L3Address peerAddress = binder_->getX2PeerAddress(srcId, destId);
    socket_.sendTo(pkt, peerAddress, tunnelPeerPort_);
}

void GtpUserX2::handleFromUdp(Packet *pkt)
{
    EV << "GtpUserX2::handleFromUdp - Decapsulating and sending to local connection." << endl;

    // obtain the original X2 message
    auto gtpMsg = pkt->popAtFront<GtpUserMsg>();
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&LteProtocol::x2ap);

    // send message to the X2 Manager
    send(pkt, "lteStackOut");
}

} //namespace

