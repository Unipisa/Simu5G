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

#ifndef STACK_RLC_AM_BUFFER_RLCSDURETRANSMISSIONBUFFER_H_
#define STACK_RLC_AM_BUFFER_RLCSDURETRANSMISSIONBUFFER_H_

#include <omnetpp.h>

#include <map>
#include <set>

namespace simu5g {

/**
 * @brief Represents a specific segment or a whole SDU pending for retransmission.
 */
struct RetxTask {
    uint32_t sn;
    uint32_t soStart;
    uint32_t soEnd;
    bool isWholeSdu;

    bool operator<(const RetxTask &other) const {
        if (sn != other.sn) return sn < other.sn;
        if (soStart != other.soStart) return soStart < other.soStart;
        return soEnd < other.soEnd;
    }
};

class RlcSduRetransmissionBuffer
{
  public:
    RlcSduRetransmissionBuffer(uint32_t threshold);
    ~RlcSduRetransmissionBuffer();

    /** @brief Reset per-Status-PDU increment flags before processing a new STATUS PDU. */
    void beginStatusPduProcessing() {
        for (auto &[sn, state] : sduRetxCounters_)
            state.incrementedInCurrentStatusPdu = false;
    }

    /** @brief Add a NACKed SDU or segment to the retransmission pool. */
    bool addNack(uint32_t sn, bool isWhole, uint32_t start, uint32_t end);

    /** @brief Returns the next task to be retransmitted. */
    bool getNextRetxTask(RetxTask &outTask) {
        if (pendingRetx_.empty())
            return false;
        outTask = *pendingRetx_.begin();
        return true;
    }

    /** @brief Returns the total pending bytes for retransmissions. */
    uint64_t getRetxPendingBytes();

    /** @brief Remove a task once retransmission is submitted to lower layer. */
    void markRetransmitted(const RetxTask &task) { pendingRetx_.erase(task); }

    /** @brief Clear retx state for an SDU (e.g., when finally ACKed). */
    void clearSdu(uint32_t sn);

  protected:
    struct RetxState {
        uint32_t retxCount = 0;
        bool incrementedInCurrentStatusPdu = false;
    };

    std::map<uint32_t, RetxState> sduRetxCounters_;
    std::set<RetxTask> pendingRetx_;
    uint32_t maxRetxThreshold_;
};

} // namespace simu5g

#endif // STACK_RLC_AM_BUFFER_RLCSDURETRANSMISSIONBUFFER_H_
