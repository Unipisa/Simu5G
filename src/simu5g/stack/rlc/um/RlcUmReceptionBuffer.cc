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

#include "RlcUmReceptionBuffer.h"

namespace simu5g {

using namespace inet;

RlcUmReceptionBuffer::RlcUmReceptionBuffer(uint32_t snBits) {

    snModulus = (1 << snBits);
    UM_Window_Size = (1 << (snBits - 1)); // Half the SN space


}

RlcUmReceptionBuffer::~RlcUmReceptionBuffer() {
    // TODO Auto-generated destructor stub
}
void RlcUmReceptionBuffer::handleSegment(uint32_t sn, uint32_t totalLen, uint32_t start, uint32_t end, Packet* ptr) {
       // 5.2.2.2.2: Basic Window Check
       uint32_t lowerBound = (RX_Next_Highest + snModulus - UM_Window_Size) % snModulus;

       // Discard if (RX_Next_Highest – UM_Window_Size) <= SN < RX_Next_Reassembly
       bool isDiscardable = false;
       if (lowerBound < RX_Next_Reassembly) {
           isDiscardable = (sn >= lowerBound && sn < RX_Next_Reassembly);
       } else {
           isDiscardable = (sn >= lowerBound || sn < RX_Next_Reassembly);
       }

       if (isDiscardable) {
           std::cout << "[RLC UM RX] Discarding SN=" << sn << " (Outside valid range)" << std::endl;
           return;
       }

       // Place in buffer
       auto it = sduBuffer.find(sn);
       if (it == sduBuffer.end()) {
           it = sduBuffer.emplace(sn, SduReassemblyState(totalLen, ptr)).first;
       }

       auto result = it->second.addSegment(start, end);
       bool isNowComplete = result.second;

       // 5.2.2.2.3: Actions when placed in buffer
       if (isNowComplete) {
           deliverToUpperLayer(sn);
           if (sn == RX_Next_Reassembly) {
               updateNextReassembly();
           }
       } else if (!isWithinWindow(sn)) {
           RX_Next_Highest = (sn + 1) % snModulus;
           discardOutsideWindow();

           if (!isWithinWindow(RX_Next_Reassembly)) {
               setNextReassemblyToFirstInWindow();
           }
       }

       manageTimer();
   }

} /* namespace simu5g */
