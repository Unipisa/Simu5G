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

#include "HandoverPacketHolderUe.h"

#include <inet/linklayer/common/InterfaceTag_m.h>
#include <inet/common/ProtocolTag_m.h>
#include <inet/common/socket/SocketTag_m.h>
#include "simu5g/common/L3Utils.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

using namespace inet;
using namespace omnetpp;

Define_Module(HandoverPacketHolderUe);

HandoverPacketHolderUe::~HandoverPacketHolderUe()
{
    while (!ueHoldFromIp_.empty()) {
        Packet *pkt = ueHoldFromIp_.front();
        ueHoldFromIp_.pop_front();
        delete pkt;
    }
}

void HandoverPacketHolderUe::initialize()
{
    stackGateOut_ = gate("stackOut");
    sessionType_ = aToSessionType(par("sessionType").stdstringValue());
}

void HandoverPacketHolderUe::setServingNodeIds(MacNodeId servingNodeId, MacNodeId nrServingNodeId)
{
    Enter_Method_Silent("setServingNodeIds");
    servingNodeId_ = servingNodeId;
    nrServingNodeId_ = nrServingNodeId;
}

void HandoverPacketHolderUe::handleMessage(cMessage *msg)
{
    if (!msg->getArrivalGate()->isName("upperLayerIn"))
        throw cRuntimeError("Message received on wrong gate %s", msg->getArrivalGate()->getFullName());

    auto pkt = check_and_cast<Packet *>(msg);
    fromIpUe(pkt);
}

void HandoverPacketHolderUe::fromIpUe(Packet *datagram)
{
    EV << "HandoverPacketHolder::fromIpUe - message from IP layer: send to stack: " << datagram->str() << std::endl;
    // Remove control info from IP datagram
    datagram->removeTagIfPresent<SocketInd>();
    removeAllSimu5GTags(datagram);

    // Remove InterfaceReq Tag (we already are on an interface now)
    datagram->removeTagIfPresent<InterfaceReq>();

    // the UE's uplink user-plane entry: an IP session's datagram is parsed for the
    // modules after it; the payload of another session is carried as is
    if (isIpSessionType(sessionType_))
        attachIpHeaderFields(datagram);
    else if (datagram->getTag<PacketProtocolTag>()->getProtocol() != &sessionPayloadProtocol(sessionType_, datagram))
        throw cRuntimeError("HandoverPacketHolderUe: a packet of protocol %s is sent on a \"%s\" session",
                datagram->getTag<PacketProtocolTag>()->getProtocol()->getName(), sessionTypeToA(sessionType_).c_str());

    if (ueHold_) {
        // hold packets until handover is complete
        ueHoldFromIp_.push_back(datagram);
    }
    else {
        if (servingNodeId_ == NODEID_NONE && nrServingNodeId_ == NODEID_NONE) { // UE is detached
            EV << "HandoverPacketHolder::fromIpUe - UE is not attached to any serving node. Delete packet." << endl;
            delete datagram;
        }
        else
            toStackUe(datagram);
    }
}

void HandoverPacketHolderUe::toStackUe(Packet *pkt)
{
    send(pkt, stackGateOut_);
}

void HandoverPacketHolderUe::triggerHandoverUe(MacNodeId newMasterId)
{
    EV << NOW << " HandoverPacketHolder::triggerHandoverUe - start holding packets" << endl;

    if (newMasterId != NODEID_NONE)
        ueHold_ = true;
}

void HandoverPacketHolderUe::signalHandoverCompleteUe(bool isNr)
{
    Enter_Method("signalHandoverCompleteUe");

    if ((isNr ? nrServingNodeId_ : servingNodeId_) != NODEID_NONE) {
        // send held packets
        while (!ueHoldFromIp_.empty()) {
            auto pkt = ueHoldFromIp_.front();
            ueHoldFromIp_.pop_front();
            toStackUe(pkt);
        }
        ueHold_ = false;
    }
}

} //namespace
