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

#include "simu5g/stack/rlc/RlcEntityManager.h"
#include "simu5g/stack/rlc/um/UmTxEntity.h"
#include "simu5g/stack/mac/LteMacBase.h"
#include "simu5g/stack/mac/packet/LteMacSduRequest.h"
#include "simu5g/stack/rlc/packet/LteRlcPdu_m.h"
#include "simu5g/stack/rlc/packet/LteRlcNewDataTag_m.h"
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"
#include "simu5g/stack/pdcp/packet/LtePdcpPdu_m.h"

#include "simu5g/stack/packetFlowObserver/PacketFlowSignals.h"

namespace simu5g {

Define_Module(UmTxEntity);

simsignal_t UmTxEntity::rlcPduCreatedSignal_ = registerSignal("rlcPduCreated");

using namespace inet;

/*
 * Main functions
 */

void UmTxEntity::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        LteMacBase *mac = inet::getConnectedModule<LteMacBase>(getParentModule()->gate("macOut"), 0);

        // store the node id of the owner module
        ownerNodeId_ = mac->getMacNodeId();

        // get the reference to the RLC module
        lteRlc_.reference(this, "umModule", true);
        queueSize_ = lteRlc_->par("queueSize");

        burstStatus_ = INACTIVE;
    }
}

void UmTxEntity::handleMessage(cMessage *msg)
{
    cPacket *pkt = check_and_cast<cPacket *>(msg);
    cGate *incoming = pkt->getArrivalGate();
    if (incoming->isName("in")) {
        handleSdu(check_and_cast<inet::Packet *>(pkt));
    }
    else if (incoming->isName("macIn")) {
        handleMacSduRequest(check_and_cast<inet::Packet *>(pkt));
    }
    else {
        throw cRuntimeError("UmTxEntity: unexpected message from gate %s", incoming->getFullName());
    }
}

void UmTxEntity::handleSdu(inet::Packet *pkt)
{
    EV << NOW << " UmTxEntity::handleSdu - Received SDU from upper layer, size " << pkt->getByteLength() << "\n";

    // Extract sequence number from PDCP header
    auto pdcpHeader = pkt->peekAtFront<LtePdcpHeader>();
    unsigned int sequenceNumber = pdcpHeader->getSequenceNumber();

    // Add PDCP tracking information
    auto pdcpTag = pkt->addTag<PdcpTrackingTag>();
    pdcpTag->setPdcpSequenceNumber(sequenceNumber);
    pdcpTag->setOriginalPacketLength(pkt->getByteLength());

    if (holdingDownstreamInPackets_) {
        // do not store in the TX buffer and do not signal the MAC layer
        EV << "UmTxEntity::handleSdu - Enqueue packet into the Holding Buffer\n";
        enqueHoldingPackets(pkt);
    }
    else {
        if (enque(pkt)) {
            EV << "UmTxEntity::handleSdu - Enqueue packet into the Tx Buffer\n";

            // create a message to notify the MAC layer that the queue contains new data
            auto pktDup = pkt->dup();
            pktDup->addTag<LteRlcNewDataTag>();

            EV << "UmTxEntity::handleSdu - Sending new data indication to MAC\n";
            send(pktDup, "out");
        }
        else {
            // Queue is full - drop SDU
            dropBufferOverflow(pkt);
        }
    }
}

void UmTxEntity::handleMacSduRequest(inet::Packet *pkt)
{
    // Enter_Method + take needed for context switch when called via gate from LowerMux
    Enter_Method("handleMacSduRequest()");
    if (pkt->getOwner() != this)
        take(pkt);

    auto macSduRequest = pkt->peekAtFront<LteMacSduRequest>();
    unsigned int size = macSduRequest->getSduSize();

    EV << NOW << " UmTxEntity::handleMacSduRequest - MAC requests PDU of size " << size << "\n";

    // do segmentation/concatenation and send a PDU to the lower layer
    rlcPduMake(size);

    delete pkt;
}

bool UmTxEntity::enque(cPacket *pkt)
{
    EV << NOW << " UmTxEntity::enque - buffering new SDU  " << endl;
    if (queueSize_ == 0 || queueLength_ + pkt->getByteLength() < queueSize_) {
        // Buffer the SDU in the TX buffer
        sduQueue_.insert(pkt);
        queueLength_ += pkt->getByteLength();
        // Packet was successfully enqueued
        return true;
    }
    else {
        // Buffer is full - cannot enqueue packet
        return false;
    }
}

void UmTxEntity::rlcPduMake(int pduLength)
{
    EV << NOW << " UmTxEntity::rlcPduMake - PDU with size " << pduLength << " requested from MAC" << endl;

    // create the RLC PDU
    auto pkt = new inet::Packet("lteRlcFragment");
    auto rlcPdu = inet::makeShared<LteRlcUmDataPdu>();

    // the request from MAC takes into account also the size of the RLC header
    pduLength -= RLC_HEADER_UM;

    int len = 0;

    bool startFrag = firstIsFragment_;
    bool endFrag = false;

    while (!sduQueue_.isEmpty() && pduLength > 0) {
        // detach data from the SDU buffer
        auto pkt = check_and_cast<inet::Packet *>(sduQueue_.front());
        auto pdcpTag = pkt->getTag<PdcpTrackingTag>();
        unsigned int sduSequenceNumber = pdcpTag->getPdcpSequenceNumber();
        int sduLength = pdcpTag->getOriginalPacketLength();

        if (fragmentInfo != nullptr) {
            if (fragmentInfo->pkt != pkt)
                throw cRuntimeError("Packets are different");
            sduLength = fragmentInfo->size;
        }

        EV << NOW << " UmTxEntity::rlcPduMake - Next data chunk from the queue, sduSno[" << sduSequenceNumber
           << "], length[" << sduLength << "]" << endl;

        if (pduLength >= sduLength) {
            EV << NOW << " UmTxEntity::rlcPduMake - Add " << sduLength << " bytes to the new SDU, sduSno[" << sduSequenceNumber << "]" << endl;

            // add the whole SDU
            if (fragmentInfo) {
                delete fragmentInfo;
                fragmentInfo = nullptr;
            }
            pduLength -= sduLength;
            len += sduLength;

            pkt = check_and_cast<inet::Packet *>(sduQueue_.pop());
            queueLength_ -= pkt->getByteLength();

            rlcPdu->pushSdu(pkt, sduLength);
            pkt = nullptr;

            EV << NOW << " UmTxEntity::rlcPduMake - Pop data chunk from the queue, sduSno[" << sduSequenceNumber << "]" << endl;

            // now, the first SDU in the buffer is not a fragment
            firstIsFragment_ = false;

            EV << NOW << " UmTxEntity::rlcPduMake - The new SDU has length " << len << ", left space is " << pduLength << endl;
        }
        else {
            EV << NOW << " UmTxEntity::rlcPduMake - Add " << pduLength << " bytes to the new SDU, sduSno[" << sduSequenceNumber << "]" << endl;

            // add partial SDU

            len += pduLength;

            auto rlcSduDup = pkt->dup();
            if (fragmentInfo != nullptr) {
                fragmentInfo->size -= pduLength;
                if (fragmentInfo->size < 0)
                    throw cRuntimeError("Fragmentation error");
            }
            else {
                fragmentInfo = new FragmentInfo;
                fragmentInfo->pkt = pkt;
                fragmentInfo->size = sduLength - pduLength;
            }
            rlcPdu->pushSdu(rlcSduDup, pduLength);

            endFrag = true;

            // update SDU in the buffer
            int newLength = sduLength - pduLength;

            EV << NOW << " UmTxEntity::rlcPduMake - Data chunk in the queue is now " << newLength << " bytes, sduSno[" << sduSequenceNumber << "]" << endl;

            pduLength = 0;

            // now, the first SDU in the buffer is a fragment
            firstIsFragment_ = true;

            EV << NOW << " UmTxEntity::rlcPduMake - The new SDU has length " << len << ", left space is " << pduLength << endl;
        }
    }

    if (len == 0) {
        // send an empty (1-bit) message to notify the MAC that there is not enough space to send RLC PDU
        // (TODO: ugly, should be indicated in a better way)
        EV << NOW << " UmTxEntity::rlcPduMake - cannot send PDU with data, pdulength requested by MAC (" << pduLength << "B) is too small." << std::endl;
        pkt->setName("lteRlcFragment (empty)");
        rlcPdu->setChunkLength(inet::b(1)); // send only a bit, minimum size
    }
    else {
        // compute FI
        // the meaning of this field is specified in 3GPP TS 36.322
        FramingInfo fi;
        fi.firstIsFragment = startFrag;   // 10
        fi.lastIsFragment = endFrag;      // 01

        rlcPdu->setFramingInfo(fi);
        rlcPdu->setPduSequenceNumber(sno_++);
        rlcPdu->setChunkLength(inet::B(RLC_HEADER_UM + len));
    }

    *pkt->addTagIfAbsent<FlowControlInfo>() = *flowControlInfo_;

    /*
     * @author Alessandro Noferi
     *
     * Notify the packetFlowObserver about the new RLC PDU
     * only in UL or DL cases
     */
    if (flowControlInfo_->getDirection() == DL || flowControlInfo_->getDirection() == UL) {
        // notify packetFlowObserver via signal
        if (len != 0 && hasListeners(rlcPduCreatedSignal_)) {
            DrbId drbId = flowControlInfo_->getDrbId();

            /*
             * Burst management.
             *
             * If the buffer is empty, the burst, if ACTIVE,
             * now is finished. Tell the flow manager to STOP
             * keep track of burst RLCs (not the timer). Set burst as INACTIVE
             *
             * if the buffer is NOT empty,
             *      if burst is already ACTIVE, do not start the timer T2
             *      if burst is INACTIVE, START the timer T2 and set it as ACTIVE
             * Tell the flow manager to keep track of burst RLCs
             */

            if (sduQueue_.isEmpty()) {
                if (burstStatus_ == ACTIVE) {
                    EV << NOW << " UmTxEntity::burstStatus - ACTIVE -> INACTIVE" << endl;

                    RlcPduSignalInfo info(drbId, rlcPdu.get(), STOP);
                    emit(rlcPduCreatedSignal_, &info);
                    burstStatus_ = INACTIVE;
                }
                else {
                    EV << NOW << " UmTxEntity::burstStatus - " << burstStatus_ << endl;

                    RlcPduSignalInfo info(drbId, rlcPdu.get(), burstStatus_);
                    emit(rlcPduCreatedSignal_, &info);
                }
            }
            else {
                if (burstStatus_ == INACTIVE) {
                    burstStatus_ = ACTIVE;
                    EV << NOW << " UmTxEntity::burstStatus - INACTIVE -> ACTIVE" << endl;
                    //start a new burst
                    RlcPduSignalInfo info(drbId, rlcPdu.get(), START);
                    emit(rlcPduCreatedSignal_, &info);
                }
                else {
                    EV << NOW << " UmTxEntity::burstStatus - burstStatus: " << burstStatus_ << endl;

                    // burst is still active
                    RlcPduSignalInfo info(drbId, rlcPdu.get(), burstStatus_);
                    emit(rlcPduCreatedSignal_, &info);
                }
            }
        }
    }

    // send to MAC layer
    pkt->insertAtFront(rlcPdu);
    pkt->addTagIfAbsent<inet::PacketProtocolTag>()->setProtocol(&LteProtocol::rlc);
    EV << NOW << " UmTxEntity::rlcPduMake - send PDU " << rlcPdu->getPduSequenceNumber() << " with size " << pkt->getByteLength() << " bytes to lower layer" << endl;
    send(pkt, "out");

    // if incoming connection was halted
    if (notifyEmptyBuffer_ && sduQueue_.isEmpty()) {
        notifyEmptyBuffer_ = false;

        // tell the RLC UM to resume packets for the new mode
        lteRlc_->resumeDownstreamInPackets(flowControlInfo_->getD2dRxPeerId());
    }
}

void UmTxEntity::dropBufferOverflow(cPacket *pkt)
{
    EV << "UmTxEntity : Dropping packet " << pkt->getName() << " (queue full) \n";
    delete pkt;
}

void UmTxEntity::removeDataFromQueue()
{
    EV << NOW << " UmTxEntity::removeDataFromQueue - removed SDU " << endl;

    // get the last packet...
    cPacket *pkt = sduQueue_.back();

    // ...and remove it
    cPacket *retPkt = sduQueue_.remove(pkt);
    queueLength_ -= retPkt->getByteLength();
    ASSERT(queueLength_ >= 0);
    delete retPkt;
}

void UmTxEntity::clearQueue()
{
    // empty buffer
    while (!sduQueue_.isEmpty())
        delete sduQueue_.pop();

    if (fragmentInfo) {
        delete fragmentInfo;
        fragmentInfo = nullptr;
    }

    queueLength_ = 0;

    // reset variables except for sequence number
    firstIsFragment_ = false;
}

bool UmTxEntity::isHoldingDownstreamInPackets()
{
    return holdingDownstreamInPackets_;
}

void UmTxEntity::enqueHoldingPackets(cPacket *pkt)
{
    EV << NOW << " UmTxEntity::enqueHoldingPackets - storing new SDU into the holding buffer " << endl;
    sduHoldingQueue_.insert(pkt);
}

void UmTxEntity::resumeDownstreamInPackets()
{
    EV << NOW << " UmTxEntity::resumeDownstreamInPackets - resume buffering incoming downstream packets of the RLC entity associated with the new mode" << endl;

    holdingDownstreamInPackets_ = false;

    // move all SDUs in the holding buffer to the TX buffer
    while (!sduHoldingQueue_.isEmpty()) {
        auto pktRlc = check_and_cast<inet::Packet *>(sduHoldingQueue_.front());

        sduHoldingQueue_.pop();

        // store the SDU in the TX buffer
        if (enque(pktRlc)) {
            // create a message to notify the MAC layer that the queue contains new data
            // make a copy of the RLC SDU
            auto pktRlcdup = pktRlc->dup();
            // add tag to indicate new data availability to MAC
            pktRlcdup->addTag<LteRlcNewDataTag>();
            // send the new data indication to the MAC
            send(pktRlcdup, "out");
        }
        else {
            // Queue is full - drop SDU
            EV << "UmTxEntity::resumeDownstreamInPackets - cannot buffer SDU (queue is full), dropping" << std::endl;
            dropBufferOverflow(pktRlc);
        }
    }
}

void UmTxEntity::rlcHandleD2DModeSwitch(bool oldConnection, bool clearBuffer)
{
    if (oldConnection) {
        if (getNodeTypeById(ownerNodeId_) == NODEB) {
            EV << NOW << " UmRxEntity::rlcHandleD2DModeSwitch - nothing to do on DL leg of IM flow" << endl;
            return;
        }

        if (clearBuffer) {
            EV << NOW << " UmTxEntity::rlcHandleD2DModeSwitch - clear TX buffer of the RLC entity associated with the old mode" << endl;
            clearQueue();
        }
        else {
            if (!sduQueue_.isEmpty()) {
                EV << NOW << " UmTxEntity::rlcHandleD2DModeSwitch - check when the TX buffer of the RLC entity associated with the old mode becomes empty - queue length[" << sduQueue_.getLength() << "]" << endl;
                notifyEmptyBuffer_ = true;
            }
            else {
                EV << NOW << " UmTxEntity::rlcHandleD2DModeSwitch - TX buffer of the RLC entity associated with the old mode is already empty" << endl;
            }
        }
    }
    else {
        EV << " UmTxEntity::rlcHandleD2DModeSwitch - reset numbering of the RLC TX entity corresponding to the new mode" << endl;
        sno_ = 0;

        if (!clearBuffer) {
            if (lteRlc_->isEmptyingTxBuffer(flowControlInfo_->getD2dRxPeerId())) {
                // stop incoming connections, until
                EV << NOW << " UmTxEntity::rlcHandleD2DModeSwitch - halt incoming downstream connections of the RLC entity associated with the new mode" << endl;
                startHoldingDownstreamInPackets();
            }
        }
    }
}

} //namespace
