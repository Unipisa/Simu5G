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

#include "RlcSduSlidingWindowTransmissionBuffer.h"

namespace simu5g {

using namespace inet;

RlcSduSlidingWindowTransmissionBuffer::RlcSduSlidingWindowTransmissionBuffer(uint32_t windowSize, const std::string &name)
    : amWindowSize_(windowSize), name_(name)
{
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
