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

#include "RlcUmTransmitterBuffer.h"

namespace simu5g {

RlcUmTransmitterBuffer::RlcUmTransmitterBuffer(uint32_t snBits) {
    snModulus = (1 << snBits);

}

RlcUmTransmitterBuffer::~RlcUmTransmitterBuffer() {
}
PendingSegmentUM RlcUmTransmitterBuffer::getSegmentForGrant(uint32_t grantSize) {
    PendingSegmentUM seg;
    if (txBuffer.empty() || grantSize == 0) return seg;

    auto& currentSdu = txBuffer.front();

    // 5.2.2.1.1: If it's a segment, it gets TX_Next
    if (currentSdu.getNextSegment(grantSize, seg.start, seg.end)) {
        seg.sn = TX_Next;
        seg.ptr = currentSdu.sduPointer;
        seg.totalLength = currentSdu.totalLength;
        seg.isValid = true;

        currentSdu.markTransmitted(seg.start, seg.end);

        // 5.2.2.1.1: If segment maps to the last byte, increment TX_Next
        if (seg.end == currentSdu.totalLength - 1) {

            seg.isLastSegment = true;
            TX_Next = (TX_Next + 1) % snModulus;

            // Once fully transmitted, UM removes it from buffer (no retransmissions)
            txBuffer.pop_front();
        }
    } else {
        //This is a full SDU
        seg.ptr = currentSdu.sduPointer;
        seg.totalLength = currentSdu.totalLength;
        seg.isValid = true;
        seg.isFull=true;
        currentSdu.markTransmitted(seg.start, seg.end);
        ASSERT(seg.start==0 && seg.end==(currentSdu.totalLength - 1));
        // Once fully transmitted, UM removes it from buffer (no retransmissions)
        txBuffer.pop_front();

    }

    return seg;
}

} /* namespace simu5g */
