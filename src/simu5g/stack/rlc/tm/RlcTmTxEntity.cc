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

#include "simu5g/stack/rlc/tm/RlcTmTxEntity.h"
#include "simu5g/stack/rlc/packet/LteRlcNewDataTag_m.h"
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"
#include "simu5g/stack/pdcp/packet/LtePdcpPdu_m.h"
#include "simu5g/stack/mac/packet/LteMacSduRequest.h"

namespace simu5g {

Define_Module(RlcTmTxEntity);

using namespace omnetpp;
using namespace inet;

simsignal_t RlcTmTxEntity::rlcPacketLossDlSignal_ = registerSignal("rlcPacketLossDl");
simsignal_t RlcTmTxEntity::rlcPacketLossUlSignal_ = registerSignal("rlcPacketLossUl");

void RlcTmTxEntity::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        queueSize_ = par("queueSize");

        WATCH(flowControlInfo_);
        WATCH(queuedPdus_);
    }
}

void RlcTmTxEntity::handleMessage(cMessage *msg)
{
    cGate *incoming = msg->getArrivalGate();
    if (incoming->isName("in")) {
        handleSdu(check_and_cast<Packet *>(msg));
    }
    else if (incoming->isName("macIn")) {
        handleMacSduRequest(check_and_cast<Packet *>(msg));
    }
    else {
        throw cRuntimeError("RlcTmTxEntity: unexpected message from gate %s", incoming->getFullName());
    }
}

void RlcTmTxEntity::handleSdu(Packet *pkt)
{
    auto lteInfo = pkt->getTag<FlowControlInfo>();

    // check if space is available or queue size is unlimited (queueSize_ == 0)
    if (queuedPdus_.getLength() >= queueSize_ && queueSize_ != 0) {
        EV << "RlcTmTxEntity : Dropping packet " << pkt->getName() << " (queue full)\n";

        simsignal_t signal = (lteInfo->getDirection() == DL) ? rlcPacketLossDlSignal_ : rlcPacketLossUlSignal_;
        emit(signal, 1.0);
        delete pkt;
        return;
    }

    // Extract sequence number from PDCP header
    auto pdcpHeader = pkt->peekAtFront<LtePdcpHeader>();
    unsigned int sequenceNumber = pdcpHeader->getSequenceNumber();

    // Add PDCP tracking information
    auto pdcpTag = pkt->addTag<PdcpTrackingTag>();
    pdcpTag->setPdcpSequenceNumber(sequenceNumber);
    pdcpTag->setOriginalPacketLength(pkt->getByteLength());

    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&LteProtocol::rlc);

    // buffer the PDU
    queuedPdus_.insert(pkt);

    // statistics: packet was not lost
    simsignal_t signal = (lteInfo->getDirection() == DL) ? rlcPacketLossDlSignal_ : rlcPacketLossUlSignal_;
    emit(signal, 0.0);

    // notify MAC of new data availability
    auto pktDup = pkt->dup();
    pktDup->addTag<LteRlcNewDataTag>();

    EV << "RlcTmTxEntity::handleSdu - Sending new data indication\n";
    send(pktDup, "out");
}

void RlcTmTxEntity::handleMacSduRequest(Packet *pkt)
{
    if (queuedPdus_.getLength() > 0) {
        auto rlcPduPkt = queuedPdus_.pop();
        EV << "RlcTmTxEntity::handleMacSduRequest - sending packet " << rlcPduPkt->getName() << "\n";
        send(rlcPduPkt, "out");
    }
    else {
        EV << "RlcTmTxEntity::handleMacSduRequest - no PDUs buffered, nothing to send\n";
    }
    delete pkt;
}

} // namespace simu5g
