//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa), Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include <inet/common/ProtocolTag_m.h>

#include "simu5g/stack/rlc/um/LteRlcUmTxEntity.h"
#include "simu5g/stack/rlc/packet/LteRlcPdu_m.h"
#include "simu5g/stack/rlc/packet/LteRlcNewDataTag_m.h"
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"
#include "simu5g/stack/packetFlowObserver/PacketFlowSignals.h"

namespace simu5g {

Define_Module(LteRlcUmTxEntity);

using namespace inet;

void LteRlcUmTxEntity::initMode()
{
    queueSize_ = par("queueSize");
    burstStatus_ = INACTIVE;

    WATCH(burstStatus_);
    WATCH(sduQueue_);
    WATCH(firstIsFragment_);
    WATCH(queueLength_);
    WATCH(sno_);
}

bool LteRlcUmTxEntity::storeSdu(inet::Packet *pkt)
{
    return enque(pkt);
}

bool LteRlcUmTxEntity::enque(cPacket *pkt)
{
    EV << NOW << " LteRlcUmTxEntity::enque - buffering new SDU  " << endl;
    if (queueSize_ == 0 || queueLength_ + pkt->getByteLength() < queueSize_) {
        // Buffer the SDU in the TX buffer
        sduQueue_.insert(pkt);
        queueLength_ += pkt->getByteLength();
        return true;
    }
    else {
        // Buffer is full - cannot enqueue packet
        return false;
    }
}

void LteRlcUmTxEntity::rlcPduMake(int pduLength)
{
    EV << NOW << " LteRlcUmTxEntity::rlcPduMake - PDU with size " << pduLength << " requested from MAC" << endl;

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

        EV << NOW << " LteRlcUmTxEntity::rlcPduMake - Next data chunk from the queue, sduSno[" << sduSequenceNumber
           << "], length[" << sduLength << "]" << endl;

        if (pduLength >= sduLength) {
            EV << NOW << " LteRlcUmTxEntity::rlcPduMake - Add " << sduLength << " bytes to the new SDU, sduSno[" << sduSequenceNumber << "]" << endl;

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

            EV << NOW << " LteRlcUmTxEntity::rlcPduMake - Pop data chunk from the queue, sduSno[" << sduSequenceNumber << "]" << endl;

            // now, the first SDU in the buffer is not a fragment
            firstIsFragment_ = false;

            EV << NOW << " LteRlcUmTxEntity::rlcPduMake - The new SDU has length " << len << ", left space is " << pduLength << endl;
        }
        else {
            EV << NOW << " LteRlcUmTxEntity::rlcPduMake - Add " << pduLength << " bytes to the new SDU, sduSno[" << sduSequenceNumber << "]" << endl;

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

            EV << NOW << " LteRlcUmTxEntity::rlcPduMake - Data chunk in the queue is now " << newLength << " bytes, sduSno[" << sduSequenceNumber << "]" << endl;

            pduLength = 0;

            // now, the first SDU in the buffer is a fragment
            firstIsFragment_ = true;

            EV << NOW << " LteRlcUmTxEntity::rlcPduMake - The new SDU has length " << len << ", left space is " << pduLength << endl;
        }
    }

    if (len == 0) {
        // send an empty (1-bit) message to notify the MAC that there is not enough space
        EV << NOW << " LteRlcUmTxEntity::rlcPduMake - cannot send PDU with data, pdulength requested by MAC (" << pduLength << "B) is too small." << std::endl;
        pkt->setName("lteRlcFragment (empty)");
        rlcPdu->setChunkLength(inet::b(1)); // send only a bit, minimum size
    }
    else {
        // compute FI (3GPP TS 36.322)
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
     * Notify the packetFlowObserver about the new RLC PDU only in UL or DL cases
     */
    if (flowControlInfo_->getDirection() == DL || flowControlInfo_->getDirection() == UL) {
        if (len != 0 && hasListeners(rlcPduCreatedSignal_)) {
            DrbKey drbKey = ctrlInfoToTxDrbKey(flowControlInfo_);

            /*
             * Burst management. If the buffer is empty, an ACTIVE burst is now
             * finished (STOP). If not empty, START a burst when INACTIVE.
             */
            if (sduQueue_.isEmpty()) {
                if (burstStatus_ == ACTIVE) {
                    EV << NOW << " LteRlcUmTxEntity::burstStatus - ACTIVE -> INACTIVE" << endl;
                    RlcPduSignalInfo info(drbKey, rlcPdu.get(), STOP);
                    emit(rlcPduCreatedSignal_, &info);
                    burstStatus_ = INACTIVE;
                }
                else {
                    EV << NOW << " LteRlcUmTxEntity::burstStatus - " << burstStatus_ << endl;
                    RlcPduSignalInfo info(drbKey, rlcPdu.get(), burstStatus_);
                    emit(rlcPduCreatedSignal_, &info);
                }
            }
            else {
                if (burstStatus_ == INACTIVE) {
                    burstStatus_ = ACTIVE;
                    EV << NOW << " LteRlcUmTxEntity::burstStatus - INACTIVE -> ACTIVE" << endl;
                    RlcPduSignalInfo info(drbKey, rlcPdu.get(), START);
                    emit(rlcPduCreatedSignal_, &info);
                }
                else {
                    EV << NOW << " LteRlcUmTxEntity::burstStatus - burstStatus: " << burstStatus_ << endl;
                    RlcPduSignalInfo info(drbKey, rlcPdu.get(), burstStatus_);
                    emit(rlcPduCreatedSignal_, &info);
                }
            }
        }
    }

    // send to MAC layer
    pkt->insertAtFront(rlcPdu);
    pkt->addTagIfAbsent<inet::PacketProtocolTag>()->setProtocol(&LteProtocol::rlc);
    EV << NOW << " LteRlcUmTxEntity::rlcPduMake - send PDU " << rlcPdu->getPduSequenceNumber() << " with size " << pkt->getByteLength() << " bytes to lower layer" << endl;
    send(pkt, "out");

    // signal the hook that a PDU has been built (used to notify the D2D mode
    // controller when the TX buffer of the old mode has drained)
    onTxBufferEmptied();
}

void LteRlcUmTxEntity::removeDataFromQueue()
{
    EV << NOW << " LteRlcUmTxEntity::removeDataFromQueue - removed SDU " << endl;
    cPacket *pkt = sduQueue_.back();
    cPacket *retPkt = sduQueue_.remove(pkt);
    queueLength_ -= retPkt->getByteLength();
    ASSERT(queueLength_ >= 0);
    delete retPkt;
}

void LteRlcUmTxEntity::clearQueue()
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

} //namespace
