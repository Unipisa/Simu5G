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

#include "simu5g/stack/handoverX2Forwarder/HandoverX2Forwarder.h"

#include <inet/common/ProtocolTag_m.h>

#include "simu5g/common/L3Utils.h"
#include "simu5g/common/LteControlInfoTags_m.h"
#include "simu5g/common/SessionTag_m.h"
#include "simu5g/common/QfiTag_m.h"

namespace simu5g {

Define_Module(HandoverX2Forwarder);

using namespace inet;
using namespace omnetpp;

void HandoverX2Forwarder::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        // get the node id
        nodeId_ = MacNodeId(inet::getContainingNode(this)->par("macCellId").intValue());
        ASSERT(nodeId_ != MacNodeId(-1));  // i.e. already set programmatically

        // get reference to the gates
        x2ManagerInGate_ = gate("x2ManagerIn");
        x2ManagerOutGate_ = gate("x2ManagerOut");

        losslessHandover_ = par("losslessHandover").boolValue();

        // register with the X2 Manager for the messages this module receives
        auto x2Packet = new Packet("X2HandoverDataMsg");
        auto initMsg = makeShared<X2HandoverDataMsg>();
        auto ctrlInfo = x2Packet->addTagIfAbsent<X2ControlInfoTag>();
        ctrlInfo->setInit(true);
        x2Packet->insertAtFront(initMsg);

        send(x2Packet, x2ManagerOutGate_);
    }
}

void HandoverX2Forwarder::handleMessage(cMessage *msg)
{
    cPacket *pkt = check_and_cast<cPacket *>(msg);
    cGate *incoming = pkt->getArrivalGate();
    if (incoming == x2ManagerInGate_) {
        // incoming data from X2 Manager
        EV << "HandoverX2Forwarder::handleMessage - Received message from X2 Manager" << endl;
        handleX2Message(pkt);
    }
    else if (incoming->isName("dataIn")) {
        // incoming data from HandoverPacketHolder to forward via X2
        EV << "HandoverX2Forwarder::handleMessage - Received data packet from HandoverPacketHolder for X2 forwarding" << endl;
        auto datagram = check_and_cast<Packet*>(pkt);
        auto tag = datagram->removeTag<X2TargetReq>();
        MacNodeId targetEnb = tag->getTargetNode();
        forwardDataToTargetEnb(datagram, targetEnb);
    }
    else
        delete msg;
}

void HandoverX2Forwarder::handleX2Message(cPacket *pkt)
{
    inet::Packet *datagram = check_and_cast<inet::Packet *>(pkt);

    auto x2msg = datagram->removeAtFront<LteX2Message>();
    datagram->removeTagIfPresent<X2ControlInfoTag>();

    X2NodeId sourceId = x2msg->getSourceId();

    if (x2msg->getType() == X2_HANDOVER_DATA_MSG) {
        // The payload is the forwarded datagram of the session again; restore the
        // protocol the forwarding side overwrote with x2ap, so downstream consumers
        // (e.g. the packet-filter dissection of bearer definitions) see the packet for
        // what it is. The forwarding tunnel named the session (see GtpUserX2).
        if (datagram->findTag<GtpEndMarkerInd>() == nullptr)   // an End Marker carries no datagram
            datagram->addTagIfAbsent<inet::PacketProtocolTag>()->setProtocol(&sessionPayloadProtocol(datagram->getTag<SessionTag>()->getSessionType(), datagram));

        // Restore the datagram's QoS flow, carried alongside it the way the 3GPP
        // forwarding tunnel carries the QFI (TS 38.425): the datagram re-enters the
        // stack without passing GtpUser, and the target's SDAP needs the QfiReq tag
        // to map it back onto a DRB. QFI_NONE = the bearer has no QoS flow (EPC).
        auto hoData = inet::dynamicPtrCast<const X2HandoverDataMsg>(x2msg);
        ASSERT(hoData != nullptr);
        if (hoData->getQfi() != QFI_NONE)
            datagram->addTagIfAbsent<QfiReq>()->setQfi(hoData->getQfi());

        receiveDataFromSourceEnb(datagram, sourceId);
    }
    else
        throw cRuntimeError("HandoverX2Forwarder: unexpected X2 message of type %d from base station %d", (int)x2msg->getType(), (int)num(sourceId));
}

void HandoverX2Forwarder::forwardDataToTargetEnb(Packet *datagram, MacNodeId targetEnb)
{
    Enter_Method("forwardDataToTargetEnb");
    take(datagram);

    // build control info
    auto ctrlInfo = datagram->addTagIfAbsent<X2ControlInfoTag>();
    ctrlInfo->setSourceId(nodeId_);
    DestinationIdList destList;
    destList.push_back(targetEnb);
    ctrlInfo->setDestIdList(destList);

    // build X2 Handover Msg; the datagram's QoS flow travels with it (see
    // handleX2Message() for the restore side)
    auto hoMsg = makeShared<X2HandoverDataMsg>();
    if (auto qfiReq = datagram->findTag<QfiReq>())
        hoMsg->setQfi(qfiReq->getQfi());
    datagram->insertAtFront(hoMsg);
    datagram->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&LteProtocol::x2ap);

    EV << NOW << " HandoverX2Forwarder::forwardDataToTargetEnb - Send IP datagram to eNB " << targetEnb << endl;

    // send to X2 Manager
    send(datagram, x2ManagerOutGate_);
}

void HandoverX2Forwarder::receiveDataFromSourceEnb(Packet *datagram, MacNodeId sourceEnb)
{
    EV << NOW << " HandoverX2Forwarder::receiveDataFromSourceEnb - Received IP datagram from eNB " << sourceEnb << endl;

    // send data to HandoverPacketHolder for transmission
    send(datagram, "tunnelOut");
}

} //namespace

