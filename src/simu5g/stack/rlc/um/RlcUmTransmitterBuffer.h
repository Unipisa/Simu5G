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

#ifndef STACK_RLC_UM_RLCUMTRANSMITTERBUFFER_H_
#define STACK_RLC_UM_RLCUMTRANSMITTERBUFFER_H_


#include <omnetpp.h>
#include <inet/common/packet/Packet.h>
namespace simu5g {
using namespace inet;

/**
 * @brief Represents a range of bytes [start, end] for transmission tracking.
 */
struct TransmitInterval {
    uint32_t start;
    uint32_t end;

    bool operator<(const TransmitInterval& other) const {
        return start < other.start;
    }
};

/**
 * @brief Manages the transmission state of a single SDU in UM mode.
 */
struct SduUmTxState {
    uint32_t sn;
    uint32_t totalLength;
    inet::Packet* sduPointer;
    std::list<TransmitInterval> transmittedIntervals;

    SduUmTxState(uint32_t s, uint32_t len, Packet* ptr)
        : sn(s), totalLength(len), sduPointer(ptr) {}

    /**
     * @brief Finds the next available byte range for UM transmission.
     */
    bool getNextSegment(uint32_t grantSize, uint32_t& outStart, uint32_t& outEnd) {
        uint32_t nextByteToTx = 0;
        transmittedIntervals.sort();

        for (const auto& interval : transmittedIntervals) {
            if (nextByteToTx < interval.start) break;
            nextByteToTx = interval.end + 1;
        }

        if (nextByteToTx >= totalLength) return false;

        outStart = nextByteToTx;
        uint32_t remainingInSdu = totalLength - nextByteToTx;
        uint32_t bytesToTransfer = std::min(grantSize, remainingInSdu);
        outEnd = outStart + bytesToTransfer - 1;

        return true;
    }

    void markTransmitted(uint32_t start, uint32_t end) {
        transmittedIntervals.push_back({start, end});
        mergeIntervals(transmittedIntervals);
    }

    bool isFullyTransmitted() {
        if (transmittedIntervals.empty()) return false;
        mergeIntervals(transmittedIntervals);
        return (transmittedIntervals.front().start == 0 &&
                transmittedIntervals.front().end == totalLength - 1);
    }
     /**
     * @brief Returns the number of bytes in this SDU that have not been transmitted yet.
     */
    uint32_t getPendingBytes() const {
        uint32_t transmittedCount = 0;
        for (const auto& interval : transmittedIntervals) {
            transmittedCount += (interval.end - interval.start + 1);
        }
        return (totalLength > transmittedCount) ? (totalLength - transmittedCount) : 0;
    }
    private:
    void mergeIntervals(std::list<TransmitInterval>& intervals) {
        if (intervals.empty()) return;
        intervals.sort();
        for (auto it = intervals.begin(); it != intervals.end(); ) {
            auto next = std::next(it);
            if (next != intervals.end() && next->start <= it->end + 1) {
                it->end = std::max(it->end, next->end);
                intervals.erase(next);
            } else {
                ++it;
            }
        }
    }
};

struct PendingSegmentUM {
    uint32_t sn;
    uint32_t start;
    uint32_t end;
    uint32_t totalLength;
    Packet* ptr;
    bool isValid = false;
    bool isLastSegment = false; // Added to help UM SN logic
    bool isFull=false;
};

class RlcUmTransmitterBuffer {
protected:
    std::list<SduUmTxState> txBuffer; // Using list for FIFO order
    uint32_t TX_Next = 0;             // Next SN to be assigned
    uint32_t snModulus;
public:
    RlcUmTransmitterBuffer(uint32_t snBits);
    virtual ~RlcUmTransmitterBuffer();

    /**
     * @brief Adds a new SDU from the upper layer.
     * Note: In UM, we don't necessarily associate the SN immediately
     * because SN is associated when submitting to lower layer.
     */
    void addSdu(uint32_t length, Packet* sduPtr) {
        // In UM, we buffer the SDU and assign SN when it's picked for transmission
        txBuffer.emplace_back(0, length, sduPtr);
    }

    /**
     * @brief Forms a UMD PDU segment based on the MAC grant.
     * Implements logic from 5.2.2.1.1.
     */
    PendingSegmentUM getSegmentForGrant(uint32_t grantSize);

    bool hasData() const {
        return !txBuffer.empty();
    }
     /**
     * @brief Calculates the total number of bytes pending for transmission across all SDUs in the buffer.
     * This is useful for reporting Buffer Status Reports (BSR) to the MAC layer.
     */
    uint64_t getTotalPendingBytes() const {
        uint64_t total = 0;
        for (const auto& sdu : txBuffer) {
            total += sdu.getPendingBytes();
        }
        return total;
    }

    /**
     * @brief Clears the buffer, deleting all stored SDUs to prevent memory leaks.
     */
    void clearBuffer() {
        for (auto& sdu : txBuffer) {
            if (sdu.sduPointer) {
                delete sdu.sduPointer;
                sdu.sduPointer = nullptr;
            }
        }
        txBuffer.clear();
    }


    uint32_t getTxNext() const { return TX_Next; }
    void resetTxNext() {TX_Next=0;}
};

} /* namespace simu5g */

#endif /* STACK_RLC_UM_RLCUMTRANSMITTERBUFFER_H_ */
