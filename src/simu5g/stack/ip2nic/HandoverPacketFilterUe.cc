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

#include "HandoverPacketFilterUe.h"

#include <inet/linklayer/common/InterfaceTag_m.h>
#include <inet/common/socket/SocketTag_m.h>
#include "simu5g/common/binder/Binder.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

using namespace inet;
using namespace omnetpp;

Define_Module(HandoverPacketFilterUe);

HandoverPacketFilterUe::~HandoverPacketFilterUe()
{
    while (!ueHoldFromIp_.empty()) {
        Packet *pkt = ueHoldFromIp_.front();
        ueHoldFromIp_.pop_front();
        delete pkt;
    }
}

void HandoverPacketFilterUe::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        stackGateOut_ = gate("stackOut");
        binder_.reference(this, "binderModule", true);

        cModule *ue = getContainingNode(this);
        nodeId_ = MacNodeId(ue->par("macNodeId").intValue());
        if (ue->hasPar("nrMacNodeId"))
            nrNodeId_ = MacNodeId(ue->par("nrMacNodeId").intValue());
    }
    else if (stage == INITSTAGE_SIMU5G_BINDER_ACCESS) {
        // get serving node IDs -- note the this is STLL not late enough to pick up the result of dynamic cell association
        if (nodeId_ != NODEID_NONE)
            servingNodeId_ = binder_->getServingNode(nodeId_);
        if (nrNodeId_ != NODEID_NONE)
            nrServingNodeId_ = binder_->getServingNode(nrNodeId_);
    }
}

void HandoverPacketFilterUe::handleMessage(cMessage *msg)
{
    if (!msg->getArrivalGate()->isName("upperLayerIn"))
        throw cRuntimeError("Message received on wrong gate %s", msg->getArrivalGate()->getFullName());

    auto pkt = check_and_cast<Packet *>(msg);
    fromIpUe(pkt);
}

void HandoverPacketFilterUe::fromIpUe(Packet *datagram)
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

void HandoverPacketFilterUe::toStackUe(Packet *pkt)
{
    send(pkt, stackGateOut_);
}

void HandoverPacketFilterUe::triggerHandoverUe(MacNodeId newMasterId, bool isNr)
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

void HandoverPacketFilterUe::signalHandoverCompleteUe(bool isNr)
{
    Enter_Method("signalHandoverCompleteUe");

    if ((!isNr && servingNodeId_ != NODEID_NONE) || (isNr && nrServingNodeId_ != NODEID_NONE)) {
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
