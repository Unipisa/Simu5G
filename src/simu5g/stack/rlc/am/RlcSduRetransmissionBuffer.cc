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

#include "RlcSduRetransmissionBuffer.h"

namespace simu5g {

RlcSduRetransmissionBuffer::RlcSduRetransmissionBuffer(uint32_t threshold)
    : maxRetxThreshold_(threshold)
{
}

RlcSduRetransmissionBuffer::~RlcSduRetransmissionBuffer()
{
}


bool RlcSduRetransmissionBuffer::addNack(uint32_t sn, bool isWhole, uint32_t start, uint32_t end)
{
    RetxTask task{sn, start, end, isWhole};
    bool alreadyPending = (pendingRetx_.find(task) != pendingRetx_.end());

    if (sduRetxCounters_.find(sn) == sduRetxCounters_.end()) {
        sduRetxCounters_[sn].retxCount = 0;
    }
    else if (!alreadyPending && !sduRetxCounters_[sn].incrementedInCurrentStatusPdu) {
        sduRetxCounters_[sn].retxCount++;
        sduRetxCounters_[sn].incrementedInCurrentStatusPdu = true;

        if (sduRetxCounters_[sn].retxCount >= maxRetxThreshold_) {
            std::cerr << "[RLC RETX] Max retransmissions reached for SN=" << sn << std::endl;
            return false;
        }
    }

    pendingRetx_.insert(task);
    return true;
}
void RlcSduRetransmissionBuffer::clearSdu(uint32_t sn)
{
    sduRetxCounters_.erase(sn);
    for (auto it = pendingRetx_.begin(); it != pendingRetx_.end(); ) {
        if (it->sn == sn)
            it = pendingRetx_.erase(it);
        else
            ++it;
    }
}
uint64_t RlcSduRetransmissionBuffer::getRetxPendingBytes()
{
    uint64_t total = 0;
    for (const auto &task : pendingRetx_)
        total += (task.soEnd - task.soStart + 1);
    return total;
}

} // namespace simu5g
