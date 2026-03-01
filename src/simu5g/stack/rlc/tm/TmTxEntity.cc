#include <inet/common/ProtocolTag_m.h>

#include "simu5g/stack/rlc/tm/TmTxEntity.h"
#include "simu5g/stack/rlc/packet/LteRlcNewDataTag_m.h"
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"
#include "simu5g/stack/pdcp/packet/LtePdcpPdu_m.h"
#include "simu5g/stack/mac/packet/LteMacSduRequest.h"

namespace simu5g {

Define_Module(TmTxEntity);

using namespace omnetpp;
using namespace inet;

simsignal_t TmTxEntity::rlcPacketLossDlSignal_ = registerSignal("rlcPacketLossDl");
simsignal_t TmTxEntity::rlcPacketLossUlSignal_ = registerSignal("rlcPacketLossUl");

void TmTxEntity::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        queueSize_ = par("queueSize");
    }
}

void TmTxEntity::handleMessage(cMessage *msg)
{
    cGate *incoming = msg->getArrivalGate();
    if (incoming->isName("in")) {
        handleSdu(check_and_cast<Packet *>(msg));
    }
    else if (incoming->isName("macIn")) {
        handleMacSduRequest(check_and_cast<Packet *>(msg));
    }
    else {
        throw cRuntimeError("TmTxEntity: unexpected message from gate %s", incoming->getFullName());
    }
}

void TmTxEntity::handleSdu(Packet *pkt)
{
    auto lteInfo = pkt->getTag<FlowControlInfo>();

    // check if space is available or queue size is unlimited (queueSize_ == 0)
    if (queuedPdus_.getLength() >= queueSize_ && queueSize_ != 0) {
        EV << "TmTxEntity : Dropping packet " << pkt->getName() << " (queue full)\n";

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

    EV << "TmTxEntity::handleSdu - Sending new data indication\n";
    send(pktDup, "out");
}

void TmTxEntity::handleMacSduRequest(Packet *pkt)
{
    if (queuedPdus_.getLength() > 0) {
        auto rlcPduPkt = queuedPdus_.pop();
        EV << "TmTxEntity::handleMacSduRequest - sending packet " << rlcPduPkt->getName() << "\n";
        send(rlcPduPkt, "out");
    }
    else {
        EV << "TmTxEntity::handleMacSduRequest - no PDUs buffered, nothing to send\n";
    }
    delete pkt;
}

} // namespace simu5g
