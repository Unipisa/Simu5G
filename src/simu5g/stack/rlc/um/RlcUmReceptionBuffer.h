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

#ifndef STACK_RLC_UM_RLCUMRECEPTIONBUFFER_H_
#define STACK_RLC_UM_RLCUMRECEPTIONBUFFER_H_

#include <omnetpp.h>
#include "simu5g/stack/rlc/am/RlcSduSlidingWindowReceptionBuffer.h"

namespace simu5g {

class RlcUmReceptionBuffer {
protected:
    std::map<uint32_t, SduReassemblyState> sduBuffer;

    // State Variables (Standard 7.1)
    uint32_t RX_Next_Reassembly = 0;
    uint32_t RX_Next_Highest = 0;
    uint32_t RX_Timer_Trigger = 0;

    uint32_t UM_Window_Size;
    uint32_t snModulus;
    bool isTimerRunning = false;
public:
    RlcUmReceptionBuffer(uint32_t snBits );
    virtual ~RlcUmReceptionBuffer();


    /**
     * @brief Check if SN falls within the reassembly window.
     * Standard: (RX_Next_Highest – UM_Window_Size) <= SN < RX_Next_Highest
     */
    bool isWithinWindow(uint32_t sn) const {
        uint32_t lowerBound = (RX_Next_Highest + snModulus - UM_Window_Size) % snModulus;

        if (lowerBound < RX_Next_Highest) {
            return (sn >= lowerBound && sn < RX_Next_Highest);
        } else if (lowerBound > RX_Next_Highest) {
            return (sn >= lowerBound || sn < RX_Next_Highest);
        } else {
            // Initialization case (0 == 0): SN 0 is NOT < 0.
            return false;
        }
    }

    bool isEmpty() const {
        return sduBuffer.empty();
    }


    void reset() {
        RX_Next_Reassembly = 0;
        RX_Next_Highest = 0;
        RX_Timer_Trigger = 0;
    }
    /**
     * @brief Processes an incoming UMD PDU segment.
     * returns true is the SDU is complete
     */
    bool handleSegment(uint32_t sn, uint32_t totalLen, uint32_t start, uint32_t end, inet::Packet* ptr);

    void onTimerExpiry();


    bool isSnLessThan(uint32_t sn1, uint32_t sn2) const {
        // Modular comparison logic
        uint32_t diff = (sn2 + snModulus - sn1) % snModulus;
        return (diff > 0 && diff < UM_Window_Size);
    }

    void updateNextReassembly() {
        while (sduBuffer.count(RX_Next_Reassembly) && sduBuffer.at(RX_Next_Reassembly).isComplete) {
            RX_Next_Reassembly = (RX_Next_Reassembly + 1) % snModulus;
        }
    }

    void setNextReassemblyToFirstInWindow() {
        uint32_t lowerBound = (RX_Next_Highest + snModulus - UM_Window_Size) % snModulus;
        RX_Next_Reassembly = lowerBound;

        updateNextReassembly();
    }

    void discardOutsideWindow();

    bool stopTimer(bool isTimerRunning );
    bool startTimer() ;
    /**
     * @brief Clears the reception buffer and deletes all stored packet pointers.
     */
    void clearBuffer() {
        for (auto& pair : sduBuffer) {
            if (pair.second.sduPointer) {
                delete pair.second.sduPointer;
                pair.second.sduPointer = nullptr;
            }
        }
        sduBuffer.clear();
    }


};

} /* namespace simu5g */

#endif /* STACK_RLC_UM_RLCUMRECEPTIONBUFFER_H_ */
