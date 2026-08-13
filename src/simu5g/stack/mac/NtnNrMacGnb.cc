//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/mac/NtnNrMacGnb.h"

#include "simu5g/stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "simu5g/stack/mac/buffer/harq/LteHarqProcessRx.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(NtnNrMacGnb);

simsignal_t NtnNrMacGnb::ntnHarqRxOccupancySignal_ = registerSignal("ntnHarqRxOccupancy");

void NtnNrMacGnb::handleSelfMessage()
{
    NrMacGnb::handleSelfMessage();

    // After the slot rather than before it. The parent extracts correct PDUs first and
    // purges corrupted ones last, so this reads the slot's minimum occupancy rather than
    // its peak -- consistent from slot to slot, which is what matters for a trend.
    emitNtnHarqRxOccupancy();
}

void NtnNrMacGnb::emitNtnHarqRxOccupancy()
{
    unsigned int occupied = 0;

    for (auto& [carrierFrequency, buffers] : harqRxBuffers_) {
        for (auto& [nodeId, buffer] : buffers) {
            unsigned int processes = buffer->getProcesses();
            for (unsigned int process = 0; process < processes; ++process) {
                // isEmpty() is false as soon as any codeword of the process holds a
                // transport block, which is the condition that makes the process
                // unavailable to the scheduler.
                if (!buffer->getProcess(process)->isEmpty())
                    ++occupied;
            }
        }
    }

    emit(ntnHarqRxOccupancySignal_, occupied);
}

} //namespace
