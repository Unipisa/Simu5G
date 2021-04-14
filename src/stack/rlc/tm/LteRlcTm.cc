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

#include <inet/common/ProtocolTag_m.h>

#include "stack/rlc/tm/LteRlcTm.h"
#include "stack/mac/packet/LteMacSduRequest.h"

Define_Module(LteRlcTm);

using namespace omnetpp;

void LteRlcTm::handleUpperMessage(cPacket *pktAux)
{
    emit(receivedPacketFromUpperLayer, pktAux);

    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto lteInfo = pkt->getTag<FlowControlInfo>();

    // check if space is available or queue size is unlimited (queueSize_ is set to 0)
    if(queuedPdus_.getLength() >= queueSize_ && queueSize_ != 0){
        // cannot queue - queue is full
        EV << "LteRlcTm : Dropping packet " << pkt->getName() << " (queue full) \n";

        // statistics: packet was lost
        if (lteInfo->getDirection()==DL)
            emit(rlcPacketLossDl, 1.0);
        else
            emit(rlcPacketLossUl, 1.0);

        drop(pkt);
        delete pkt;
        delete lteInfo;

        return;
    }

    // build the PDU itself
    auto rlcSdu = inet::makeShared<LteRlcSdu>();
    auto rlcPdu = inet::makeShared<LteRlcPdu>();
    pkt->insertAtFront(rlcSdu);
    pkt->insertAtFront(rlcPdu);
    pkt->addTagIfAbsent<inet::PacketProtocolTag>()->setProtocol(&LteProtocol::rlc);

    // buffer the PDU
    queuedPdus_.insert(pkt);

    // statistics: packet was not lost
    if (lteInfo->getDirection()==DL)
        emit(rlcPacketLossDl, 0.0);
    else
        emit(rlcPacketLossUl, 0.0);


    // create a message so as to notify the MAC layer that the queue contains new data
    auto newDataPkt = inet::makeShared<LteRlcPduNewData>();
    // make a copy of the RLC SDU
    // the MAC will only be interested in the size of this packet
    auto pktDup = pkt->dup();
    pktDup->insertAtFront(newDataPkt);

    EV << "LteRlcTm::handleUpperMessage - Sending message " << newDataPkt->getName() << " to port TM_Sap_down$o\n";
    emit(sentPacketToLowerLayer,pktDup);
    send(pktDup, down_[OUT_GATE]);
}

void LteRlcTm::handleLowerMessage(cPacket *pkt)
{
    emit(receivedPacketFromLowerLayer, pkt);

    if (strcmp(pkt->getName(), "LteMacSduRequest") == 0) {
        if(queuedPdus_.getLength() > 0){
            auto rlcPduPkt = queuedPdus_.pop();
            EV << "LteRlcTm : Received " << pkt->getName() << " - sending packet " << rlcPduPkt->getName() << " to port TM_Sap_down$o\n";
            emit(sentPacketToLowerLayer,pkt);
            drop (rlcPduPkt);

            send(rlcPduPkt, down_[OUT_GATE]);
        } else
            EV << "LteRlcTm : Received " << pkt->getName() << " but no PDUs buffered - nothing to send to MAC.\n";

    } else {
        // FIXME: needs to be changed to use inet::Packet
        FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(pkt->removeControlInfo());
        cPacket* upPkt = check_and_cast<cPacket *>(pkt->decapsulate());
        cPacket* upUpPkt = check_and_cast<cPacket *>(upPkt->decapsulate());
        upUpPkt->setControlInfo(lteInfo);
        delete upPkt;
        // pkt->addTagIfAbsent<inet::PacketProtocolTag>()->setProtocol(&LteProtocol::pdcp);

        EV << "LteRlcTm : Sending packet " << upUpPkt->getName() << " to port TM_Sap_up$o\n";
        emit(sentPacketToUpperLayer, upUpPkt);
        send(upUpPkt, up_[OUT_GATE]);
    }

    drop(pkt);
    delete pkt;
}

/*
 * Main functions
 */

void LteRlcTm::initialize()
{
    up_[IN_GATE] = gate("TM_Sap_up$i");
    up_[OUT_GATE] = gate("TM_Sap_up$o");
    down_[IN_GATE] = gate("TM_Sap_down$i");
    down_[OUT_GATE] = gate("TM_Sap_down$o");

    queueSize_ = par("queueSize");

    // statistics
    receivedPacketFromUpperLayer = registerSignal("receivedPacketFromUpperLayer");
    receivedPacketFromLowerLayer = registerSignal("receivedPacketFromLowerLayer");
    sentPacketToUpperLayer = registerSignal("sentPacketToUpperLayer");
    sentPacketToLowerLayer = registerSignal("sentPacketToLowerLayer");
    rlcPacketLossDl = registerSignal("rlcPacketLossDl");
    rlcPacketLossDl = registerSignal("rlcPacketLossUl");
}

void LteRlcTm::handleMessage(cMessage* msg)
{
    cPacket* pkt = check_and_cast<cPacket *>(msg);
    EV << "LteRlcTm : Received packet " << pkt->getName() <<
    " from port " << pkt->getArrivalGate()->getName() << endl;

    cGate* incoming = pkt->getArrivalGate();
    if (incoming == up_[IN_GATE])
    {
        handleUpperMessage(pkt);
    }
    else if (incoming == down_[IN_GATE])
    {
        handleLowerMessage(pkt);
    }
    return;
}
