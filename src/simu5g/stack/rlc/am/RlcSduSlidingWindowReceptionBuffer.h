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

#ifndef STACK_RLC_AM_BUFFER_RLCSDUSLIDINGWINDOWRECEPTIONBUFFER_H_
#define STACK_RLC_AM_BUFFER_RLCSDUSLIDINGWINDOWRECEPTIONBUFFER_H_

#include <omnetpp.h>
#include <inet/common/packet/Packet.h>

#include <algorithm>
#include <list>
#include <map>

#include "simu5g/stack/rlc/LteRlcDefs.h"

namespace simu5g {

/**
 * @brief Represents a range of bytes [start, end] within an SDU.
 */
struct ByteInterval {
    uint32_t start;
    uint32_t end;

    // Sort by start position for easy merging
    bool operator<(const ByteInterval& other) const {
        return start < other.start;
    }
};

/**
 * @brief Manages the reassembly state of a single SDU.
 */
struct SduReassemblyState {
    uint32_t totalLength;
    inet::Packet *sduPointer;
    std::list<ByteInterval> receivedIntervals;
    bool isComplete;

    SduReassemblyState(uint32_t length, inet::Packet *ptr)
        : totalLength(length), sduPointer(ptr), isComplete(false) {}

    /**
     * @brief Adds a new byte interval and merges overlapping or adjacent intervals.
     * @return A pair: <bool newlyBytesAdded, bool isNowComplete>
     */
    std::pair<bool, bool> addSegment(uint32_t start, uint32_t end) {
        if (isComplete)
            return {false, true};
        if (end >= totalLength)
            end = totalLength - 1;

        // Check if already fully covered
        for (const auto &interval : receivedIntervals) {
            if (start >= interval.start && end <= interval.end)
                return {false, isComplete};
        }

        ByteInterval newInterval{start, end};
        auto it = std::lower_bound(receivedIntervals.begin(), receivedIntervals.end(), newInterval);
        receivedIntervals.insert(it, newInterval);

        // Merge overlapping/adjacent intervals
        for (auto current = receivedIntervals.begin(); current != receivedIntervals.end(); ) {
            auto next = std::next(current);
            if (next != receivedIntervals.end() && next->start <= current->end + 1) {
                current->end = std::max(current->end, next->end);
                receivedIntervals.erase(next);
            }
            else {
                ++current;
            }
        }

        // Check completion: single interval covering [0, totalLength-1]
        if (!receivedIntervals.empty()) {
            const auto &first = receivedIntervals.front();
            if (first.start == 0 && first.end == totalLength - 1)
                isComplete = true;
        }

        return {true, isComplete};
    }
    /**
     * @brief Checks if there is a missing byte segment before the last byte of all received segments.
     * This corresponds to the check needed for t-Reassembly stopping/starting logic.
     */
    bool hasMissingByteSegmentBeforeLast() const {
        if (receivedIntervals.empty())
            return false;
        if (receivedIntervals.size() > 1)
            return true;
        return receivedIntervals.front().start != 0;
    }
};

class RlcSduSlidingWindowReceptionBuffer : public omnetpp::cObject
{
  public:
    RlcSduSlidingWindowReceptionBuffer(uint32_t windowSize, const std::string &name);
    ~RlcSduSlidingWindowReceptionBuffer() override;

    /**
     * @brief Process a received segment of an SDU.
     * @return pair<completeSdu, discarded>
     */
    std::pair<bool, bool> handleSegment(uint32_t sn, uint32_t totalSduLength,
                                        uint32_t start, uint32_t end,
                                        inet::Packet *sduDataPtr);

    /** @brief Retrieve the assembled SDU pointer and remove it from the buffer. */
    inet::Packet *consumeSdu(uint32_t sn);

    /** @brief Check if SN has missing byte segments before the last received byte. */
    bool hasMissingByteSegmentBeforeLast(uint32_t sn);

    /** @brief Handle reassembly timer expiry per TS 38.322 5.2.3.2.4. */
    void handleReassemblyTimerExpiry(uint32_t rxNextStatusTrigger);

    /** @brief Generate data for a STATUS PDU. */
    StatusPduData generateStatusPduData();

    /** @brief Advance rxHighestStatus_ past completed SDUs. */
    void updateRxHighestStatus();

    /** @brief Advance rxNext_ past completed SDUs (window slide). */
    void updateRxNext();

    bool isReady(uint32_t sn) const {
        auto it = sduBuffer_.find(sn);
        return it != sduBuffer_.end() && it->second.isComplete;
    }

    bool inWindow(uint32_t sn) const {
        return sn >= rxNext_ && sn < rxNext_ + amWindowSize_;
    }

    bool aboveWindow(uint32_t sn) const {
        return sn >= rxNext_ + amWindowSize_;
    }

    void discardSdu(uint32_t sn) { sduBuffer_.erase(sn); }

    uint32_t getRxNext() const { return rxNext_; }
    uint32_t getRxNextHighest() const { return rxNextHighest_; }
    uint32_t getRxHighestStatus() const { return rxHighestStatus_; }

  protected:
    std::map<uint32_t, SduReassemblyState> sduBuffer_;

    // RLC AM reception state variables
    uint32_t rxNext_ = 0;           // Lower edge of the receiving window
    uint32_t rxNextHighest_ = 0;    // SN following highest SN received
    uint32_t rxHighestStatus_ = 0;  // SN following highest delivered SDU
    uint32_t amWindowSize_;
    uint32_t consumed_ = 0;
    std::string name_;
};

} // namespace simu5g

#endif // STACK_RLC_AM_BUFFER_RLCSDUSLIDINGWINDOWRECEPTIONBUFFER_H_
