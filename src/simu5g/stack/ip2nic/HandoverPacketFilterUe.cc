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

Define_Module(HandoverPacketFilterUe);

void HandoverPacketFilterUe::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        stackGateOut_ = gate("stackOut");
        binder_.reference(this, "binderModule", true);
    }
    else if (stage == INITSTAGE_SIMU5G_REGISTRATIONS) {
        cModule *ue = getContainingNode(this);
        servingNodeId_ = MacNodeId(ue->par("servingNodeId").intValue());
        nodeId_ = MacNodeId(ue->par("macNodeId").intValue());

        if (ue->hasPar("nrServingNodeId") && ue->par("nrServingNodeId").intValue() != 0) { // register also the NR MacNodeId
            nrServingNodeId_ = MacNodeId(ue->par("nrServingNodeId").intValue());
            nrNodeId_ = MacNodeId(ue->par("nrMacNodeId").intValue());
        }
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

            // send pkt down
            take(pkt);
            toStackUe(pkt);
        }
        ueHold_ = false;
    }
}

HandoverPacketFilterUe::~HandoverPacketFilterUe()
{
    //TODO
}

void HandoverPacketFilterUe::finish()
{
    //TODO into RRC!!!
    if (getSimulation()->getSimulationStage() != CTX_FINISH) {
        // do this only at deletion of the module during the simulation
        binder_->unregisterNode(nodeId_);
        binder_->unregisterNode(nrNodeId_);
    }
}

} //namespace
