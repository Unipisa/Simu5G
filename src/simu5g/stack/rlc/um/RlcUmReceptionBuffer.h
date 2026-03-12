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
     * Logic: (RX_Next_Highest – UM_Window_Size) <= SN < RX_Next_Highest
     */
    bool isWithinWindow(uint32_t sn) const {
        uint32_t lowerBound = (RX_Next_Highest + snModulus - UM_Window_Size) % snModulus;

        if (lowerBound < RX_Next_Highest) {
            return (sn >= lowerBound && sn < RX_Next_Highest);
        } else {
            // Window wraps around the modulus
            return (sn >= lowerBound || sn < RX_Next_Highest);
        }
    }

    /**
     * @brief Processes an incoming UMD PDU segment.
     */
    void handleSegment(uint32_t sn, uint32_t totalLen, uint32_t start, uint32_t end, inet::Packet* ptr);

    void onTimerExpiry() {
        std::cout << "[RLC UM RX] t-Reassembly expired." << std::endl;
        isTimerRunning = false;

        // Update RX_Next_Reassembly to first SN >= RX_Timer_Trigger not reassembled
        RX_Next_Reassembly = RX_Timer_Trigger;
        updateNextReassembly();

        // Discard all segments with SN < updated RX_Next_Reassembly
        for (auto it = sduBuffer.begin(); it != sduBuffer.end(); ) {
            if (isSnLessThan(it->first, RX_Next_Reassembly)) {
                it = sduBuffer.erase(it);
            } else {
                ++it;
            }
        }

        manageTimer();
    }


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

    void discardOutsideWindow() {
        for (auto it = sduBuffer.begin(); it != sduBuffer.end(); ) {
            if (!isWithinWindow(it->first)) {
                it = sduBuffer.erase(it);
            } else {
                ++it;
            }
        }
    }

    void manageTimer() {
        bool shouldStop = false;
        if (isTimerRunning) {
            // Check stopping conditions from 5.2.2.2.3
            bool cond1 = !isSnLessThan(RX_Next_Reassembly, RX_Timer_Trigger); // RX_Timer_Trigger <= RX_Next_Reassembly
            bool cond2 = (!isWithinWindow(RX_Timer_Trigger) && RX_Timer_Trigger != RX_Next_Highest);

            bool cond3 = false;
            uint32_t nextAfterReas = (RX_Next_Reassembly + 1) % snModulus;
            if (RX_Next_Highest == nextAfterReas) {
                auto it = sduBuffer.find(RX_Next_Reassembly);
                if (it != sduBuffer.end() && !it->second.hasMissingByteSegmentBeforeLast()) {
                    cond3 = true;
                }
            }

            if (cond1 || cond2 || cond3) {
                isTimerRunning = false;
                shouldStop = true;
            }
        }

        if (!isTimerRunning) {
            // Check starting conditions
            bool startCond1 = isSnLessThan(RX_Next_Reassembly + 1, RX_Next_Highest);
            bool startCond2 = false;
            if (RX_Next_Highest == (RX_Next_Reassembly + 1) % snModulus) {
                auto it = sduBuffer.find(RX_Next_Reassembly);
                if (it != sduBuffer.end() && it->second.hasMissingByteSegmentBeforeLast()) {
                    startCond2 = true;
                }
            }

            if (startCond1 || startCond2) {
                isTimerRunning = true;
                RX_Timer_Trigger = RX_Next_Highest;
                // In actual simulation, trigger the physical timer here
            }
        }
    }

    void deliverToUpperLayer(uint32_t sn) {
        std::cout << "[RLC UM RX] SDU SN=" << sn << " Reassembled and Delivered." << std::endl;
        // In simulation, logic to pop from buffer and send up
    }

};

} /* namespace simu5g */

#endif /* STACK_RLC_UM_RLCUMRECEPTIONBUFFER_H_ */
