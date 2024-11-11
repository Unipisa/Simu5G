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

#include "stack/handoverManager/LteHandoverManager.h"

#include <inet/common/ProtocolTag_m.h>

#include "stack/handoverManager/X2HandoverCommandIE.h"

namespace simu5g {

Define_Module(LteHandoverManager);

using namespace inet;
using namespace omnetpp;

void LteHandoverManager::initialize()
{
    // get the node id
    nodeId_ = MacNodeId(inet::getContainingNode(this)->par("macCellId").intValue());

    // get reference to the gates
    x2ManagerInGate_ = gate("x2ManagerIn");
    x2ManagerOutGate_ = gate("x2ManagerOut");

    // get reference to the IP2Nic layer
    ip2nic_.reference(this, "ip2nicModule", true);

    losslessHandover_ = par("losslessHandover").boolValue();

    // register to the X2 Manager
    auto x2Packet = new Packet("X2HandoverControlMsg");
    auto initMsg = makeShared<X2HandoverControlMsg>();
    auto ctrlInfo = x2Packet->addTagIfAbsent<X2ControlInfoTag>();
    ctrlInfo->setInit(true);
    x2Packet->insertAtFront(initMsg);

    send(x2Packet, x2ManagerOutGate_);
}

void LteHandoverManager::handleMessage(cMessage *msg)
{
    cPacket *pkt = check_and_cast<cPacket *>(msg);
    cGate *incoming = pkt->getArrivalGate();
    if (incoming == x2ManagerInGate_) {
        // incoming data from X2 Manager
        EV << "LteHandoverManager::handleMessage - Received message from X2 Manager" << endl;
        handleX2Message(pkt);
    }
    else
        delete msg;
}

void LteHandoverManager::handleX2Message(cPacket *pkt)
{
    inet::Packet *datagram = check_and_cast<inet::Packet *>(pkt);

    auto x2msg = datagram->removeAtFront<LteX2Message>();
    datagram->removeTagIfPresent<X2ControlInfoTag>();

    X2NodeId sourceId = x2msg->getSourceId();

    if (x2msg->getType() == X2_HANDOVER_DATA_MSG) {
        receiveDataFromSourceEnb(datagram, sourceId);
    }
    else { // X2_HANDOVER_CONTROL_MSG
        X2HandoverCommandIE *hoCommandIe = check_and_cast<X2HandoverCommandIE *>(x2msg->popIe());
        receiveHandoverCommand(hoCommandIe->getUeId(), x2msg->getSourceId(), hoCommandIe->isStartHandover());

        delete hoCommandIe;
        delete pkt;
    }
}

void LteHandoverManager::sendHandoverCommand(MacNodeId ueId, MacNodeId enb, bool startHo)
{
    Enter_Method("sendHandoverCommand");

    EV << NOW << " LteHandoverManager::sendHandoverCommand - Send handover command over X2 to eNB " << enb << " for UE " << ueId << endl;

    auto pkt = new Packet("X2HandoverControlMsg");

    // build control info
    auto ctrlInfo = pkt->addTagIfAbsent<X2ControlInfoTag>();
    ctrlInfo->setSourceId(nodeId_);
    DestinationIdList destList;
    destList.push_back(enb);
    ctrlInfo->setDestIdList(destList);

    // build X2 Handover Msg
    X2HandoverCommandIE *hoCommandIe = new X2HandoverCommandIE();
    hoCommandIe->setUeId(ueId);
    if (startHo)
        hoCommandIe->setStartHandover();

    auto hoMsg = makeShared<X2HandoverControlMsg>();
    hoMsg->pushIe(hoCommandIe);
    pkt->insertAtFront(hoMsg);
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&LteProtocol::x2ap);

    // send to X2 Manager
    send(pkt, x2ManagerOutGate_);
}

void LteHandoverManager::receiveHandoverCommand(MacNodeId ueId, MacNodeId enb, bool startHo)
{
    EV << NOW << " LteHandoverManager::receiveHandoverCommand - Received handover command over X2 from eNB " << enb << " for UE " << ueId << endl;

    // send command to IP2Nic
    if (startHo)
        ip2nic_->triggerHandoverTarget(ueId, enb);
    else
        ip2nic_->signalHandoverCompleteSource(ueId, enb);
}

void LteHandoverManager::forwardDataToTargetEnb(Packet *datagram, MacNodeId targetEnb)
{
    Enter_Method("forwardDataToTargetEnb");
    take(datagram);

    // build control info
    auto ctrlInfo = datagram->addTagIfAbsent<X2ControlInfoTag>();
    ctrlInfo->setSourceId(nodeId_);
    DestinationIdList destList;
    destList.push_back(targetEnb);
    ctrlInfo->setDestIdList(destList);

    // build X2 Handover Msg
    auto hoMsg = makeShared<X2HandoverDataMsg>();
    datagram->insertAtFront(hoMsg);
    datagram->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&LteProtocol::x2ap);

    EV << NOW << " LteHandoverManager::forwardDataToTargetEnb - Send IP datagram to eNB " << targetEnb << endl;

    // send to X2 Manager
    send(datagram, x2ManagerOutGate_);
}

void LteHandoverManager::receiveDataFromSourceEnb(Packet *datagram, MacNodeId sourceEnb)
{
    EV << NOW << " LteHandoverManager::receiveDataFromSourceEnb - Received IP datagram from eNB " << sourceEnb << endl;

    // send data to IP2Nic for transmission
    ip2nic_->receiveTunneledPacketOnHandover(datagram, sourceEnb);
}

} //namespace

