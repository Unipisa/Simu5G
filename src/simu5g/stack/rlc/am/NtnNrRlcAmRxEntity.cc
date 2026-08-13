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

#include "simu5g/stack/rlc/am/NtnNrRlcAmRxEntity.h"

#include "simu5g/common/Ntn38331Timers.h"
#include "simu5g/common/NtnCommon.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(NtnNrRlcAmRxEntity);

void NtnNrRlcAmRxEntity::initMode()
{
    NrRlcAmRxEntity::initMode();

    bool deriveReassembly = ntnTimerIsDerived(tReassembly_);
    bool deriveStatusProhibit = ntnTimerIsDerived(tStatusProhibit_);
    if (!deriveReassembly && !deriveStatusProhibit)
        return;

    simtime_t roundTripDelay = ntnCellRoundTripDelay(this);
    if (roundTripDelay <= SIMTIME_ZERO)
        throw cRuntimeError("NtnNrRlcAmRxEntity::initMode - %s was left to derive its timers from the cell "
                "round-trip delay, but its cell has no NTN association. Either the bearer is not on a "
                "satellite path -- in which case it should not use this entity type -- or the association "
                "was never registered.", getFullPath().c_str());

    if (deriveReassembly) {
        // TR 38.821 clause 7.2.2.1: reassembly has to outlast the whole HARQ retransmission budget,
        // i.e. one round trip per transmission attempt.
        int transmissionAttempts = ntnHarqTransmissions(this);
        tReassembly_ = ntn38331Ceil(roundTripDelay * transmissionAttempts, Ntn38331Timer::Reassembly);
    }

    if (deriveStatusProhibit) {
        // TS 38.322: status prohibit must leave the peer's poll timer room to expire *after* a
        // STATUS report could have arrived, so it has to fit inside t_PollRetransmit minus one round
        // trip. The transmitting side's value is read rather than assumed, so that a scenario which
        // overrides t_PollRetransmit still gets a consistent pair; when that value is itself derived
        // this reproduces the same rounding the transmitter applied.
        simtime_t pollRetransmit = getParentModule()->getSubmodule("tx")->par("t_PollRetransmit").doubleValue();
        if (ntnTimerIsDerived(pollRetransmit))
            pollRetransmit = ntn38331Ceil(roundTripDelay, Ntn38331Timer::PollRetransmit);

        // Rounded down, and legitimately reaches zero on a short round trip, where the next legal
        // poll value sits only a few milliseconds above it. Zero simply means the receiver never
        // withholds a STATUS report, which costs control overhead but is otherwise safe.
        tStatusProhibit_ = ntn38331Floor(pollRetransmit - roundTripDelay, Ntn38331Timer::StatusProhibit);
    }

    EV_INFO << "NtnNrRlcAmRxEntity::initMode - " << getFullPath() << ": cell round-trip delay["
            << roundTripDelay.dbl() * 1000.0 << "ms] gives t_Reassembly[" << tReassembly_.dbl() * 1000.0
            << "ms] t_StatusProhibit[" << tStatusProhibit_.dbl() * 1000.0 << "ms]" << endl;
}

} // namespace simu5g
