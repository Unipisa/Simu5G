//
//                  Simu5G
//
// Authors: Esteban Egea Lopez (Universidad Politecnica de Cartagena)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "RlcSduSlidingWindowTransmissionBuffer.h"

namespace simu5g {

using namespace inet;

RlcSduSlidingWindowTransmissionBuffer::RlcSduSlidingWindowTransmissionBuffer(uint32_t windowSize, const std::string &name)
    : amWindowSize_(windowSize), name_(name)
{
    WATCH(txBuffer_);
    WATCH(hasTransmitted_);
    WATCH(highestSnTransmitted_);
    WATCH(txNext_);
    WATCH(txNextAck_);
}

RlcSduSlidingWindowTransmissionBuffer::~RlcSduSlidingWindowTransmissionBuffer()
{
    for (auto &entry : txBuffer_) {
        delete entry.second.sduPointer;
    }
    txBuffer_.clear();
}
uint32_t RlcSduSlidingWindowTransmissionBuffer::addSdu(uint32_t length, Packet *sduPtr)
{
    uint32_t assignedSn = txNext_;
    txBuffer_.emplace(assignedSn, SduTxState(assignedSn, length, sduPtr));
    txNext_++;
    return assignedSn;
}

PendingSegment RlcSduSlidingWindowTransmissionBuffer::getSegmentForGrant(uint32_t grantSize)
{
    PendingSegment result;
    for (auto &[sn, state] : txBuffer_) {
        // SN must be within [txNextAck_, txNextAck_ + amWindowSize_)
        if (sn < txNextAck_ || sn >= txNextAck_ + amWindowSize_)
            continue;

        uint32_t start, end;
        if (state.getNextSegment(grantSize, start, end)) {
            result.sn = sn;
            result.start = start;
            result.end = end;
            result.ptr = state.sduPointer;
            result.totalLength = state.totalLength;
            result.isValid = true;
            hasTransmitted_ = true;
            state.markTransmitted(start, end);
            highestSnTransmitted_ = std::max(highestSnTransmitted_, sn);
            return result;
        }
    }
    return result;
}

bool RlcSduSlidingWindowTransmissionBuffer::peekNextSegmentStart(uint32_t &outStart) const
{
    // Same SDU selection order as getSegmentForGrant(), but non-consuming: the next new-data
    // PDU carves the front not-fully-sent SDU in window, starting at its next un-sent byte.
    for (const auto &[sn, state] : txBuffer_) {
        if (sn < txNextAck_ || sn >= txNextAck_ + amWindowSize_)
            continue;
        uint32_t nextByte = state.getNextByteToTx();
        if (nextByte < state.totalLength) {
            outStart = nextByte;
            return true;
        }
    }
    return false;
}

std::set<uint32_t> RlcSduSlidingWindowTransmissionBuffer::handleAck(
    uint32_t sn, uint32_t start, uint32_t end, unsigned int pollSn, bool &restartPoll)
{
    std::set<uint32_t> acked;
    auto it = txBuffer_.find(sn);
    if (it == txBuffer_.end())
        return acked;

    it->second.markAcked(start, end);

    // Advance txNextAck_: find smallest SN not fully ACKed
    while (txNextAck_ < txNext_) {
        auto currentIt = txBuffer_.find(txNextAck_);
        if (currentIt == txBuffer_.end() || currentIt->second.isFullyAcked()) {
            if (currentIt != txBuffer_.end()) {
                acked.insert(currentIt->first);
                delete currentIt->second.sduPointer;
                txBuffer_.erase(currentIt);
            }
            if (txNextAck_ == pollSn)
                restartPoll = true;
            txNextAck_++;
        }
        else {
            break;
        }
    }
    return acked;
}
int RlcSduSlidingWindowTransmissionBuffer::getTotalPendingBytes() const
{
    int totalPending = 0;
    for (const auto &[sn, state] : txBuffer_) {
        uint32_t transmitted = state.getBytesTransmitted();
        if (transmitted < state.totalLength)
            totalPending += (state.totalLength - transmitted);
    }
    return totalPending;
}

PendingSegment RlcSduSlidingWindowTransmissionBuffer::getRetransmissionSegment(
    uint32_t sn, uint32_t start, uint32_t end, uint32_t grantSize)
{
    PendingSegment segment;
    auto it = txBuffer_.find(sn);
    if (it == txBuffer_.end() || !hasTransmitted_)
        return segment;

    ASSERT(end >= start);
    uint32_t taskLen = end - start;
    uint32_t bytesToTransfer = std::min(grantSize, taskLen);

    segment.sn = sn;
    segment.start = start;
    segment.end = start + bytesToTransfer;
    ASSERT(segment.end >= segment.start);
    segment.ptr = it->second.sduPointer;
    segment.totalLength = it->second.totalLength;
    segment.isValid = true;
    it->second.markTransmitted(segment.start, segment.end);
    return segment;
}

} /* namespace simu5g */
