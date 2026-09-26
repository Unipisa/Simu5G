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

#include <inet/networklayer/common/NetworkInterface.h>
#include <inet/common/ProtocolTag_m.h>

#include "simu5g/stack/rlc/um/RlcUmTxEntityBase.h"
#include "simu5g/stack/mac/LteMacBase.h"
#include "simu5g/stack/mac/packet/LteMacSduRequest.h"
#include "simu5g/stack/rlc/packet/LteRlcNewDataTag_m.h"
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"
#include "simu5g/stack/pdcp/packet/LtePdcpPdu_m.h"

namespace simu5g {

using namespace inet;

// Registered here (single translation unit) so the signal-id assignment order is
// independent of the LTE/NR concrete .cc files.
simsignal_t RlcUmTxEntityBase::rlcPduCreatedSignal_ = registerSignal("rlcPduCreated");
simsignal_t RlcUmTxEntityBase::wastedGrantedBytesSignal_ = registerSignal("wastedGrantedBytes");
simsignal_t RlcUmTxEntityBase::requestedPduSizeSignal_ = registerSignal("requestedPduSize");
simsignal_t RlcUmTxEntityBase::sentPduSizeSignal_ = registerSignal("sentPduSize");
simsignal_t RlcUmTxEntityBase::receivedPacketFromUpperLayerSignal_ = registerSignal("receivedPacketFromUpperLayer");
simsignal_t RlcUmTxEntityBase::sentPacketToLowerLayerSignal_ = registerSignal("sentPacketToLowerLayer");

void RlcUmTxEntityBase::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        LteMacBase *mac = getModuleFromPar<LteMacBase>(par("macModule"), this);
        ownerNodeId_ = mac->getMacNodeId();

        initMode();

        WATCH(flowControlInfo_);
    }
}

void RlcUmTxEntityBase::handleMessage(cMessage *msg)
{
    auto pkt = check_and_cast<inet::Packet *>(msg);
    cGate *incoming = pkt->getArrivalGate();
    if (incoming->isName("in")) {
        handleSdu(pkt);
    }
    else if (incoming->isName("macIn")) {
        handleMacSduRequest(pkt);
    }
    else {
        throw cRuntimeError("RlcUmTxEntity: unexpected message from gate %s", incoming->getFullName());
    }
}

void RlcUmTxEntityBase::handleSdu(inet::Packet *pkt)
{
    EV << NOW << " RlcUmTxEntity::handleSdu - Received SDU from upper layer, size " << pkt->getByteLength() << "\n";

    // Add PDCP tracking information (SN + original length), read back in rlcPduMake().
    auto pdcpHeader = pkt->peekAtFront<LtePdcpHeader>();
    auto pdcpTag = pkt->addTag<PdcpTrackingTag>();
    pdcpTag->setPdcpSequenceNumber(pdcpHeader->getSequenceNumber());
    pdcpTag->setOriginalPacketLength(pkt->getByteLength());

    // give the hook a chance to consume the SDU (e.g. hold it during a D2D mode switch)
    if (interceptSdu(pkt))
        return;

    bufferSduAndNotifyMac(pkt);
}

void RlcUmTxEntityBase::bufferSduAndNotifyMac(inet::Packet *pkt)
{
    if (storeSdu(pkt)) {
        // Notify the MAC that the queue has new data. The dup carries the full SDU
        // length, so the MAC tracks remaining bytes itself across grant requests.
        auto pktDup = pkt->dup();
        pktDup->addTag<LteRlcNewDataTag>();
        EV << "RlcUmTxEntity::bufferSduAndNotifyMac - Sending new data indication to MAC\n";
        send(pktDup, "out");
    }
    else {
        // Queue is full - drop SDU (LTE mode only; NR buffer is unbounded)
        dropBufferOverflow(pkt);
    }
}

void RlcUmTxEntityBase::handleMacSduRequest(inet::Packet *pkt)
{
    // Enter_Method + take needed for context switch when called via gate from the RlcMux
    Enter_Method("handleMacSduRequest()");
    if (pkt->getOwner() != this)
        take(pkt);

    auto macSduRequest = pkt->peekAtFront<LteMacSduRequest>();
    unsigned int size = macSduRequest->getSduSize();

    EV << NOW << " RlcUmTxEntity::handleMacSduRequest - MAC requests PDU of size " << size << "\n";

    // do segmentation/concatenation and send a PDU to the lower layer
    rlcPduMake(size);

    delete pkt;
}

void RlcUmTxEntityBase::dropBufferOverflow(cPacket *pkt)
{
    EV << "RlcUmTxEntity : Dropping packet " << pkt->getName() << " (queue full) \n";
    delete pkt;
}

void RlcUmTxEntityBase::sendPduToMac(inet::Packet *pkt)
{
    pkt->addTagIfAbsent<inet::PacketProtocolTag>()->setProtocol(&LteProtocol::rlc);
    send(pkt, "out");
}

} //namespace
