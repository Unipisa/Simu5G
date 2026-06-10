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

#include "RlcSduSlidingWindowReceptionBuffer.h"
#include "simu5g/stack/rlc/am/packet/NrRlcAmDataPdu.h"

namespace simu5g {

using namespace inet;

RlcSduSlidingWindowReceptionBuffer::RlcSduSlidingWindowReceptionBuffer(
    uint32_t windowSize, const std::string &name)
    : amWindowSize_(windowSize), name_(name)
{
}

RlcSduSlidingWindowReceptionBuffer::~RlcSduSlidingWindowReceptionBuffer()
{
    for (auto &entry : sduBuffer_) {
        delete entry.second.sduPointer;
    }
    sduBuffer_.clear();
}
std::pair<bool, bool> RlcSduSlidingWindowReceptionBuffer::handleSegment(
    uint32_t sn, uint32_t totalSduLength, uint32_t start, uint32_t end, Packet *sduDataPtr)
{
    auto it = sduBuffer_.find(sn);
    bool previous = (it != sduBuffer_.end());
    if (!previous)
        it = sduBuffer_.emplace(sn, SduReassemblyState(totalSduLength, sduDataPtr)).first;

    auto result = it->second.addSegment(start, end);
    bool newBytesAdded = result.first;
    bool newlyCompleted = (newBytesAdded && result.second);

    if (!newBytesAdded)
        return {false, true};

    // TODO: remove when implementation changes to a proper byte-level reassembly buffer.
    // Currently the complete SDU is sent in every segment packet, so we replace the old pointer.
    if (previous) {
        delete it->second.sduPointer;
        it->second.sduPointer = sduDataPtr;
    }

    // TS 38.322 5.2.3.2.3: update RX_Next_Highest
    if (sn >= rxNextHighest_)
        rxNextHighest_ = sn + 1;

    return {newlyCompleted, false};
}
bool RlcSduSlidingWindowReceptionBuffer::hasMissingByteSegmentBeforeLast(uint32_t sn)
{
    auto it = sduBuffer_.find(sn);
    if (it == sduBuffer_.end())
        return false;
    return it->second.hasMissingByteSegmentBeforeLast();
}
void RlcSduSlidingWindowReceptionBuffer::updateRxHighestStatus()
{
    while (true) {
        auto it = sduBuffer_.find(rxHighestStatus_);
        if (it != sduBuffer_.end() && it->second.isComplete)
            rxHighestStatus_++;
        else
            break;
    }
}

void RlcSduSlidingWindowReceptionBuffer::updateRxNext()
{
    while (true) {
        auto it = sduBuffer_.find(rxNext_);
        if (it != sduBuffer_.end() && it->second.isComplete) {
            // Do not delete sduPointer — it was consumed and deleted by passUp()
            discardSdu(rxNext_);
            rxNext_++;
        }
        else {
            break;
        }
    }
}

Packet *RlcSduSlidingWindowReceptionBuffer::consumeSdu(uint32_t sn)
{
    auto it = sduBuffer_.find(sn);
    if (it != sduBuffer_.end() && it->second.isComplete) {
        Packet *ptr = it->second.sduPointer;
        it->second.sduPointer = nullptr;
        ++consumed_;
        return ptr;
    }
    return nullptr;
}

void RlcSduSlidingWindowReceptionBuffer::handleReassemblyTimerExpiry(uint32_t rxNextStatusTrigger)
{
    uint32_t searchSn = rxNextStatusTrigger;
    while (true) {
        auto it = sduBuffer_.find(searchSn);
        if (it == sduBuffer_.end() || !it->second.isComplete) {
            rxHighestStatus_ = searchSn;
            break;
        }
        searchSn++;
    }
}


StatusPduData RlcSduSlidingWindowReceptionBuffer::generateStatusPduData()
{
    StatusPduData data;

    // Identify NACKs for SDUs in [rxNext_, rxHighestStatus_)
    for (uint32_t sn = rxNext_; sn < rxHighestStatus_; ++sn) {
        auto it = sduBuffer_.find(sn);

        // Case A: no bytes received for this SN
        if (it == sduBuffer_.end()) {
            if (!data.nacks.empty() && !data.nacks.back().isSegment &&
                    data.nacks.back().sn + data.nacks.back().nackRange == sn) {
                data.nacks.back().nackRange++;
            }
            else {
                NackInfo nack;
                nack.sn = sn;
                nack.isSegment = false;
                data.nacks.push_back(nack);
            }
        }
        // Case B: partly received — report missing byte segments
        else if (!it->second.isComplete) {
            uint32_t currentByte = 0;
            it->second.receivedIntervals.sort();

            for (const auto &interval : it->second.receivedIntervals) {
                if (currentByte < interval.start) {
                    NackInfo nack;
                    nack.sn = sn;
                    nack.isSegment = true;
                    nack.soStart = currentByte;
                    nack.soEnd = interval.start - 1;
                    data.nacks.push_back(nack);
                }
                currentByte = interval.end + 1;
            }

            if (currentByte < it->second.totalLength) {
                NackInfo nack;
                nack.sn = sn;
                nack.isSegment = true;
                nack.soStart = currentByte;
                nack.soEnd = it->second.totalLength - 1;
                data.nacks.push_back(nack);
            }
        }
    }

    // Set ACK_SN to the first SN not fully received
    uint32_t ackSnCandidate = rxHighestStatus_;
    while (true) {
        auto it = sduBuffer_.find(ackSnCandidate);
        if (it != sduBuffer_.end() && it->second.isComplete)
            ackSnCandidate++;
        else {
            data.ackSn = ackSnCandidate;
            break;
        }
    }

    return data;
}

} // namespace simu5g
