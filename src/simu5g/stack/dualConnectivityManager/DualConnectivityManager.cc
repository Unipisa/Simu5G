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

#include "stack/dualConnectivityManager/DualConnectivityManager.h"

#include <inet/common/ProtocolTag_m.h>

namespace simu5g {

using namespace omnetpp;

Define_Module(DualConnectivityManager);

void DualConnectivityManager::initialize()
{
    pdcp_.reference(this, "pdcpRrcModule", true);

    // get the node id
    nodeId_ = MacNodeId(inet::getContainingNode(this)->par("macCellId").intValue());

    // get reference to the gates
    x2ManagerInGate_ = gate("x2ManagerIn");
    x2ManagerOutGate_ = gate("x2ManagerOut");

    // register to the X2 Manager
    auto x2packet = new inet::Packet("X2DualConnectivityDataMsg");
    auto initMsg = inet::makeShared<X2DualConnectivityDataMsg>();
    auto ctrlInfo = x2packet->addTagIfAbsent<X2ControlInfoTag>();
    ctrlInfo->setInit(true);
    x2packet->insertAtFront(initMsg);

    send(x2packet, x2ManagerOutGate_);
}

void DualConnectivityManager::handleMessage(cMessage *msg)
{
    if (msg->getArrivalGate() == x2ManagerInGate_) {
        // incoming data from X2 Manager
        EV << "DualConnectivityManager::handleMessage - Received message from X2 manager" << endl;
        handleX2Message(msg);
    }
    else
        delete msg;
}

void DualConnectivityManager::handleX2Message(cMessage *msg)
{
    inet::Packet *packet = check_and_cast<inet::Packet *>(msg);

    packet->trim();
    auto x2Msg = packet->peekAtFront<LteX2Message>();
    if (x2Msg->getType() == X2_DUALCONNECTIVITY_DATA_MSG) {
        auto dcMsg = packet->removeAtFront<X2DualConnectivityDataMsg>();

        // forward packet to the PDCP layer
        X2NodeId sourceId = dcMsg->getSourceId();

        // copy FlowControlInfo Tag to the original packet
        auto dcMsgTag = dcMsg->getTag<FlowControlInfo>();
        auto pktTag = packet->addTagIfAbsent<FlowControlInfo>();
        *pktTag = *dcMsgTag;

        receiveDataFromSourceNode(packet, sourceId);
        return;
    }
    else
        throw cRuntimeError("DualConnectivityManager::handleX2Message - Message type not valid. Abort.");

    delete packet;
}

void DualConnectivityManager::forwardDataToTargetNode(inet::Packet *pkt, MacNodeId targetNode)
{
    Enter_Method("forwardDataToTargetNode");
    take(pkt);

    DestinationIdList destList;
    destList.push_back(targetNode);
    pkt->addTagIfAbsent<X2ControlInfoTag>()->setDestIdList(destList);
    pkt->addTagIfAbsent<X2ControlInfoTag>()->setSourceId(nodeId_);
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&LteProtocol::x2ap);

    // insert X2 Dual Connectivity Msg header
    auto dcMsg = inet::makeShared<X2DualConnectivityDataMsg>();

    // copy FlowControlInfo Tag to the dcMsg region
    // this is necessary because otherwise it will be removed during the transmission over the X2
    auto pktTag = pkt->removeTag<FlowControlInfo>();
    auto dcMsgTag = dcMsg->addTagIfAbsent<FlowControlInfo>();
    *dcMsgTag = *pktTag;

    pkt->insertAtFront(dcMsg);

    EV << NOW << " DualConnectivityManager::forwardDataToTargetNode - Send packet to node " << targetNode << endl;

    // send to X2 Manager
    send(pkt, x2ManagerOutGate_);
}

void DualConnectivityManager::receiveDataFromSourceNode(inet::Packet *pkt, MacNodeId sourceNode)
{
    EV << NOW << " DualConnectivityManager::receiveDataFromSourceNode - Received packet from node " << sourceNode << endl;
    // send data to PDCP
    pdcp_->receiveDataFromSourceNode(pkt, sourceNode);
}

} //namespace

