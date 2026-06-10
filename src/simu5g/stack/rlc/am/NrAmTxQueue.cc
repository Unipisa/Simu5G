//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "NrAmTxQueue.h"
#include "simu5g/stack/rlc/am/LteRlcAm.h"
#include "simu5g/stack/rlc/am/packet/NrRlcAmDataPdu.h"
#include "simu5g/stack/rlc/am/packet/NrRlcAmStatusPdu_m.h"
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"
#include "simu5g/common/LteControlInfoTags_m.h"
#include "simu5g/stack/mac/LteMacBase.h"
#include "simu5g/stack/mac/buffer/LteMacBuffer.h"


namespace simu5g {

using namespace omnetpp;
using namespace inet;

Define_Module(NrAmTxQueue);

simsignal_t NrAmTxQueue::wastedGrantedBytesSignal_ = registerSignal("wastedGrantedBytes");
simsignal_t NrAmTxQueue::enqueuedSduSizeSignal_ = registerSignal("enqueuedSduSize");
simsignal_t NrAmTxQueue::enqueuedSduRateSignal_ = registerSignal("enqueuedSduRate");
simsignal_t NrAmTxQueue::requestedPduSizeSignal_ = registerSignal("requestedPduSize");
simsignal_t NrAmTxQueue::txWindowOccupationSignal_ = registerSignal("txWindowOccupation");
simsignal_t NrAmTxQueue::txWindowFullSignal_ = registerSignal("txWindowFull");
simsignal_t NrAmTxQueue::retransmissionPduSignal_ = registerSignal("retransmissionPdu");



NrAmTxQueue::~NrAmTxQueue()
{
    cancelAndDelete(tPollRetransmitTimer_);
    delete txBuffer_;
    delete rtxBuffer_;
    while (!sduBuffer_.empty()) {
        delete sduBuffer_.front();
        sduBuffer_.pop_front();
    }
    while (!controlBuffer_.empty()) {
        delete controlBuffer_.front();
        controlBuffer_.pop_front();
    }
}
void NrAmTxQueue::initialize()
{
    lteRlc_.reference(this, "amModule", true);

    amWindowSize_ = par("AM_Window_Size");
    if (amWindowSize_ != 2048 && amWindowSize_ != 131072)
        throw cRuntimeError("NrAmTxQueue::initialize() AM_Window_Size=%u, but only 2048 or 131072 are valid", amWindowSize_);

    pollPdu_ = par("pollPDU");
    pollByte_ = par("pollByte");
    maxRtxThreshold_ = par("maxRtxThreshold");
    tPollRetransmit_ = par("t_PollRetransmit");
    tPollRetransmitTimer_ = new cMessage("t_PollRetransmit timer");

    mac_ = inet::getConnectedModule<LteMacBase>(getParentModule()->gate("RLC_to_MAC"), 0);
    if (mac_->getNodeType() == NODEB)
        nameEntity_ = "GNB-" + std::to_string(lteRlc_->getId());
    else
        nameEntity_ = "UE-" + std::to_string(lteRlc_->getId());

    txBuffer_ = new RlcSduSlidingWindowTransmissionBuffer(amWindowSize_, nameEntity_ + "-tx-sliding window:");
    rtxBuffer_ = new RlcSduRetransmissionBuffer(maxRtxThreshold_);
    lastSduSample_ = NOW;
}

void NrAmTxQueue::enque(Packet *sdu)
{
    Enter_Method("NrAmTxQueue::enque()");
    EV << NOW << " NrAmTxQueue::enque() - inserting new SDU " << sdu << endl;

    // TODO: we keep a single FlowControlInfo per queue; should take tag per SDU when transmitted
    lteInfo_ = sdu->getTag<FlowControlInfo>()->dup();
    infoCid_ = ctrlInfoToMacCid(sdu->getTagForUpdate<FlowControlInfo>().get());

    take(sdu);
    auto *si = new SduInfo();
    si->sdu = sdu;
    sduBuffer_.push_back(si);
    ++receivedSdus_;

    sduSampleBytes_ += sdu->getByteLength();
    lteRlc_->emit(enqueuedSduSizeSignal_, sdu->getByteLength());
    if ((NOW - lastSduSample_) >= 1) {
        lteRlc_->emit(enqueuedSduRateSignal_, sduSampleBytes_ / (NOW - lastSduSample_));
        sduSampleBytes_ = 0;
        lastSduSample_ = NOW;
    }

    lteRlc_->indicateNewDataToMac(sdu);
}

void NrAmTxQueue::sendPdus(int pduSize)
{
    Enter_Method("NrAmTxQueue::sendPdus()");
    EV << NOW << " NrAmTxQueue::sendPdus() - PDU with size " << pduSize << " requested from MAC" << endl;
    lteRlc_->emit(requestedPduSizeSignal_, pduSize);

    if (radioLinkFailureDetected_) {
        EV << NOW << " " << nameEntity_ << " NrAmTxQueue::sendPdus() RLF detected, stopping" << endl;
        return;
    }

    // TODO: RLC header size depends on SN length (12/18 bit) and segmentation
    int size = pduSize - RLC_HEADER_AM;

    if (size < 0) {
        // Grant too small — send empty PDU to notify MAC
        // TODO: should be indicated in a better way
        auto pkt = new inet::Packet("lteRlcFragment (empty)");
        auto rlcPdu = inet::makeShared<NrRlcAmDataPdu>();
        rlcPdu->setChunkLength(inet::b(1));
        pkt->insertAtFront(rlcPdu);
        lteRlc_->sendFragmented(pkt);
        return;
    }

    // TS 38.322: prioritize control PDUs
    if (!controlBuffer_.empty()) {
        auto *pktControl = check_and_cast<inet::Packet *>(controlBuffer_.front());
        controlBuffer_.pop_front();

        EV << NOW << " NrAmTxQueue::sendPdus() - sending Control PDU " << pktControl
           << " with size " << pktControl->getByteLength() << " bytes to lower layer" << endl;
        lteRlc_->sendFragmented(pktControl);

        // TODO: MAC only asks for one PDU; remaining grant bytes are wasted
        // Workaround: if we have more data, notify MAC so it schedules another grant
        unsigned int pendingData = getPendingDataVolume();
        if (pendingData > 0 && lteInfo_) {
            auto pkt = new inet::Packet("lteRlcFragment -Indicate new data");
            auto rlcPdu = inet::makeShared<NrRlcAmDataPdu>();
            rlcPdu->setChunkLength(B(pendingData));
            *(pkt->addTagIfAbsent<FlowControlInfo>()) = *lteInfo_;
            pkt->insertAtFront(rlcPdu);
            lteRlc_->indicateNewDataToMac(pkt);
            delete pkt;
        }
        return;
    }

    // Retransmissions have priority over new data
    if (sendRetransmission(size)) {
        reportBufferStatus();
        return;
    }

    // Try pending data in the TX sliding window
    PendingSegment segment;
    segment.isValid = false;
    if (txBuffer_->getTotalPendingBytes() > 0)
        segment = txBuffer_->getSegmentForGrant(size);

    if (!segment.isValid) {
        // No pending segments — try detaching a new SDU
        if (sduBuffer_.empty()) {
            EV << NOW << " NrAmTxQueue::sendPdus() buffer empty, wasting grant" << endl;
            lteRlc_->emit(wastedGrantedBytesSignal_, size);
            return;
        }

        SduInfo *si = sduBuffer_.front();
        auto *bufferedSdu = check_and_cast<inet::Packet *>(si->sdu);
        auto pdcpTag = bufferedSdu->getTag<PdcpTrackingTag>();
        int sduLength = pdcpTag->getOriginalPacketLength();

        if (txBuffer_->windowFull()) {
            lteRlc_->emit(txWindowFullSignal_, 1);
            return;
        }

        txBuffer_->addSdu(sduLength, bufferedSdu);
        sduBuffer_.pop_front();
        // SDU ownership transferred to txBuffer_; deleted when ACKed
        segment = txBuffer_->getSegmentForGrant(size);
    }

    if (!segment.isValid) {
        EV << NOW << " NrAmTxQueue::sendPdus() no segment fits grant" << endl;
        lteRlc_->emit(wastedGrantedBytesSignal_, size);
        return;
    }

    sendSegment(segment);
    lteRlc_->emit(txWindowOccupationSignal_, txBuffer_->getTxNext() - txBuffer_->getTxNextAck());
    reportBufferStatus();
}

void NrAmTxQueue::sendSegment(PendingSegment segment)
{
    auto rlcPdu = inet::makeShared<NrRlcAmDataPdu>();

    // Compute Segmentation Info (SI) — reuses FramingInfo field
    uint32_t segmentSize = segment.end - segment.start;
    FramingInfo fi;  // 00 = full SDU
    if (segmentSize != segment.totalLength) {
        if (segment.start == 0) {
            fi.lastIsFragment = true;  // 01 = first segment
        }
        else if (segment.end == segment.totalLength - 1) {
            fi.firstIsFragment = true;  // 10 = last segment
        }
        else {
            fi.firstIsFragment = true;
            fi.lastIsFragment = true;  // 11 = middle segment
        }
    }

    auto *bufferedSdu = check_and_cast<inet::Packet *>(segment.ptr);
    auto pdcpTag = bufferedSdu->getTag<PdcpTrackingTag>();
    int sduLength = pdcpTag->getOriginalPacketLength();
    unsigned int pduSequenceNumber = segment.sn;
    sn_ = std::max(sn_, pduSequenceNumber);
    int sduSequenceNumber = pdcpTag->getPdcpSequenceNumber();

    rlcPdu->pushSdu(bufferedSdu->dup(), segmentSize);
    rlcPdu->setFramingInfo(fi);
    rlcPdu->setPduSequenceNumber(pduSequenceNumber);
    rlcPdu->setChunkLength(inet::B(RLC_HEADER_AM + segmentSize));
    rlcPdu->setSnoMainPacket(sduSequenceNumber);
    rlcPdu->setLengthMainPacket(sduLength);
    rlcPdu->setStartOffset(segment.start);
    rlcPdu->setEndOffset(segment.end);

    // Polling
    ++pduWithoutPoll_;
    byteWithoutPoll_ += segmentSize;
    rlcPdu->setPollStatus(checkPolling());

    std::string name = "NR AM RLC Fragment -" + std::to_string(pduSequenceNumber);
    auto pkt = new inet::Packet(name.c_str());
    pkt->insertAtFront(rlcPdu);
    // TODO: each packet should carry the tag from its own SDU, not the shared lteInfo_
    if (lteInfo_)
        *(pkt->addTagIfAbsent<FlowControlInfo>()) = *lteInfo_;
    auto ipFlowTag = bufferedSdu->findTag<IpFlowInd>();
    if (ipFlowTag)
        *(pkt->addTagIfAbsent<IpFlowInd>()) = *ipFlowTag;

    lteRlc_->sendFragmented(pkt);
}
bool NrAmTxQueue::checkPolling()
{
    bool noPendingData = (txBuffer_->getTotalPendingBytes() == 0
            && sduBuffer_.empty() && rtxBuffer_->getRetxPendingBytes() == 0);

    if (pollPending_ || pduWithoutPoll_ > pollPdu_ || byteWithoutPoll_ >= pollByte_
            || noPendingData || txBuffer_->windowFull()) {
        pduWithoutPoll_ = 0;
        byteWithoutPoll_ = 0;
        pollSn_ = sn_;
        rescheduleAfter(tPollRetransmit_, tPollRetransmitTimer_);
        pollPending_ = false;
        return true;
    }
    return false;
}
void NrAmTxQueue::reportBufferStatus()
{
    // Use destId-based CID to match how MAC creates outgoing connections (MacCid(destId, lcid))
    MacCid macCid = MacCid(lteInfo_->getDestId(), lteInfo_->getLcid());
    unsigned int macOccupancy = mac_->getMacBuffer(macCid)->getQueueOccupancy();

    if (macOccupancy == 0) {
        unsigned int pendingData = getPendingDataVolume();
        // TODO: change to a more robust approach — CID comes from lteInfo_
        if (pendingData > 0 && lteInfo_) {
            auto pkt = new inet::Packet("lteRlcFragment Inform MAC");
            auto rlcPdu = inet::makeShared<NrRlcAmDataPdu>();
            rlcPdu->setChunkLength(B(pendingData));
            *(pkt->addTagIfAbsent<FlowControlInfo>()) = *lteInfo_;
            pkt->insertAtFront(rlcPdu);
            lteRlc_->indicateNewDataToMac(pkt);
            delete pkt;
        }
    }
}

bool NrAmTxQueue::sendRetransmission(int size)
{
    RetxTask next;
    if (!rtxBuffer_->getNextRetxTask(next))
        return false;

    uint32_t start = next.soStart;
    uint32_t end = next.soEnd;

    if (next.isWholeSdu) {
        Packet *ptr = nullptr;
        uint32_t totalLength = 0;
        if (txBuffer_->getSduData(next.sn, ptr, totalLength)) {
            start = 0;
            end = totalLength - 1;
        }
        else {
            if (radioLinkFailureDetected_)
                return false;
            throw cRuntimeError("NrAmTxQueue::sendRetransmission whole SDU sn=%u not found", next.sn);
        }
    }

    PendingSegment segment = txBuffer_->getRetransmissionSegment(next.sn, start, end, size);
    if (!segment.isValid) {
        if (radioLinkFailureDetected_)
            return false;
        throw cRuntimeError("NrAmTxQueue::sendRetransmission SDU sn=%u: invalid segment", next.sn);
    }
    if (!segment.ptr) {
        if (radioLinkFailureDetected_)
            return false;
        throw cRuntimeError("NrAmTxQueue::sendRetransmission SDU sn=%u: null pointer", next.sn);
    }

    sendSegment(segment);
    rtxBuffer_->markRetransmitted(next);
    lteRlc_->emit(retransmissionPduSignal_, 1);
    return true;
}
void NrAmTxQueue::handleControlPacket(omnetpp::cPacket *pkt)
{
    Enter_Method("handleControlPacket()");
    take(pkt);

    auto *pktPdu = check_and_cast<Packet *>(pkt);
    auto pdu = pktPdu->peekAtFront<NrRlcAmStatusPdu>();
    StatusPduData data = pdu->getData();

    rtxBuffer_->beginStatusPduProcessing();
    std::set<uint32_t> nacks;
    bool restartPoll = false;

    // Process NACKs
    for (size_t i = 0; i < data.nacks.size(); ++i) {
        const NackInfo &info = data.nacks[i];
        for (unsigned int j = 0; j < info.nackRange; ++j) {
            uint32_t nackedSn = info.sn + j;
            if (txBuffer_->isInRtxRange(nackedSn)) {
                // NackInfo::isSegment is inverted w.r.t. RetxTask::isWholeSdu
                bool isWhole = !info.isSegment;
                bool added = rtxBuffer_->addNack(nackedSn, isWhole, info.soStart, info.soEnd);
                if (!added) {
                    // Max retransmissions reached — Radio Link Failure
                    EV << nameEntity_ << " [CRITICAL] Radio Link Failure" << endl;
                    radioLinkFailureDetected_ = true;
                    if (lteInfo_) {
                        lteRlc_->handleRadioLinkFailure(lteInfo_);
                    }
                    else {
                        Packet *ptrAux = nullptr;
                        uint32_t totalLength = 0;
                        if (txBuffer_->getSduData(nackedSn, ptrAux, totalLength))
                            lteRlc_->handleRadioLinkFailure(ptrAux->getTag<FlowControlInfo>()->dup());
                    }
                    delete pkt;
                    return;
                }
            }
            nacks.insert(nackedSn);
            // TS 38.322 5.3.3
            if (nackedSn == pollSn_)
                restartPoll = true;
        }
    }

    // Process ACKs
    uint32_t next = txBuffer_->getTxNextAck();
    while (next < data.ackSn) {
        if (nacks.find(next) == nacks.end()) {
            uint32_t totalLength;
            Packet *sdu = nullptr;
            if (txBuffer_->getSduData(next, sdu, totalLength)) {
                std::set<uint32_t> acked = txBuffer_->handleAck(next, 0, totalLength - 1, pollSn_, restartPoll);
                for (uint32_t ackedSn : acked)
                    rtxBuffer_->clearSdu(ackedSn);
            }
        }
        ++next;
    }

    // TS 38.322 5.3.3
    if (tPollRetransmitTimer_->isScheduled() && restartPoll)
        rescheduleAfter(tPollRetransmit_, tPollRetransmitTimer_);

    lteRlc_->emit(txWindowOccupationSignal_, txBuffer_->getCurrentWindowSize());
    delete pkt;
}


void NrAmTxQueue::bufferControlPdu(omnetpp::cPacket *pkt)
{
    Enter_Method("NrAmTxQueue::bufferControlPdu()");
    take(pkt);
    controlBuffer_.push_back(pkt);
    lteRlc_->indicateNewDataToMac(pkt);
}

void NrAmTxQueue::finish()
{
}

void NrAmTxQueue::handleMessage(cMessage *msg)
{
    if (msg == tPollRetransmitTimer_) {
        pollPending_ = true;
        bool noPendingData = (txBuffer_->getTotalPendingBytes() == 0
                && sduBuffer_.empty() && rtxBuffer_->getRetxPendingBytes() == 0);

        if (noPendingData || txBuffer_->windowFull()) {
            uint32_t hsn = txBuffer_->getHighestSnTransmitted();
            Packet *ptr = nullptr;
            uint32_t totalLength = 0;
            bool found = txBuffer_->getSduData(hsn, ptr, totalLength);
            bool added = found && rtxBuffer_->addNack(hsn, true, 0, totalLength - 1);

            // Consider any unACKed PDU for retransmission
            if (!added) {
                uint32_t next = txBuffer_->getTxNextAck();
                while (txBuffer_->isInRtxRange(next)) {
                    if (!txBuffer_->isFullyAcknowledged(next)) {
                        if (txBuffer_->getSduData(next, ptr, totalLength)) {
                            if (rtxBuffer_->addNack(next, true, 0, totalLength - 1))
                                return;
                        }
                    }
                    ++next;
                }
            }
        }
    }
}

unsigned int NrAmTxQueue::getPendingDataVolume() const
{
    unsigned int size = 0;
    for (const auto *si : sduBuffer_) {
        auto *pkt = check_and_cast<inet::Packet *>(si->sdu);
        auto pdcpTag = pkt->getTag<PdcpTrackingTag>();
        size += pdcpTag->getOriginalPacketLength();
    }
    size += txBuffer_->getTotalPendingBytes();
    size += rtxBuffer_->getRetxPendingBytes();
    for (const auto *cpkt : controlBuffer_) {
        auto *p = check_and_cast<const Packet *>(cpkt);
        auto statusPdu = p->peekAtFront<NrRlcAmStatusPdu>();
        size += statusPdu->getChunkLength().get();
    }
    return size;
}

} //namespace
