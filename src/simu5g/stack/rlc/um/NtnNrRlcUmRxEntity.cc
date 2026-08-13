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

#include "simu5g/stack/rlc/um/NtnNrRlcUmRxEntity.h"

#include "simu5g/common/Ntn38331Timers.h"
#include "simu5g/common/NtnCommon.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(NtnNrRlcUmRxEntity);

void NtnNrRlcUmRxEntity::initMode(LteMacBase *mac)
{
    NrRlcUmRxEntity::initMode(mac);

    if (!ntnTimerIsDerived(t_Reassembly))
        return;

    simtime_t roundTripDelay = ntnCellRoundTripDelay(this);
    if (roundTripDelay <= SIMTIME_ZERO)
        throw cRuntimeError("NtnNrRlcUmRxEntity::initMode - %s was left to derive t_Reassembly from the cell "
                "round-trip delay, but its cell has no NTN association. Either the bearer is not on a "
                "satellite path -- in which case it should not use this entity type -- or the association "
                "was never registered.", getFullPath().c_str());

    // TR 38.821 clause 7.2.2.1 gives the same reassembly formula for both RLC modes: the timer has
    // to outlast the HARQ retransmission budget, i.e. one round trip per transmission attempt.
    int transmissionAttempts = ntnHarqTransmissions(this);
    t_Reassembly = ntn38331Ceil(roundTripDelay * transmissionAttempts, Ntn38331Timer::Reassembly);

    EV_INFO << "NtnNrRlcUmRxEntity::initMode - " << getFullPath() << ": cell round-trip delay["
            << roundTripDelay.dbl() * 1000.0 << "ms] over " << transmissionAttempts
            << " transmissions gives t_Reassembly[" << t_Reassembly.dbl() * 1000.0 << "ms]" << endl;
}

} // namespace simu5g
