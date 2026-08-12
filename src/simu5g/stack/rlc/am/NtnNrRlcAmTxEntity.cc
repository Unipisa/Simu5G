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

#include "simu5g/stack/rlc/am/NtnNrRlcAmTxEntity.h"

#include "simu5g/common/Ntn38331Timers.h"
#include "simu5g/stack/rlc/NtnRlcTimers.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(NtnNrRlcAmTxEntity);

void NtnNrRlcAmTxEntity::initialize(int stage)
{
    NrRlcAmTxEntity::initialize(stage);

    if (stage != inet::INITSTAGE_LOCAL)
        return;

    simtime_t roundTripDelay = ntnCellRoundTripDelay(this);

    if (ntnTimerIsDerived(tPollRetransmit_)) {
        if (roundTripDelay <= SIMTIME_ZERO)
            throw cRuntimeError("NtnNrRlcAmTxEntity::initialize - %s was left to derive t_PollRetransmit "
                    "from the cell round-trip delay, but its cell has no NTN association. Either the bearer "
                    "is not on a satellite path -- in which case it should not use this entity type -- or "
                    "the association was never registered.", getFullPath().c_str());

        // TS 38.322: the poll retransmit timer bounds how long the transmitter waits for a STATUS
        // report before polling again, so it has to outlast one full round trip. Rounding up to the
        // next value TS 38.331 can actually signal is what supplies the margin; for a GEO round trip
        // of 541ms the enumeration jumps from 500ms to 800ms, which is where the value this branch
        // previously hard-coded came from.
        tPollRetransmit_ = ntn38331Ceil(roundTripDelay, Ntn38331Timer::PollRetransmit);

        EV_INFO << "NtnNrRlcAmTxEntity::initialize - " << getFullPath() << ": cell round-trip delay["
                << roundTripDelay.dbl() * 1000.0 << "ms] gives t_PollRetransmit["
                << tPollRetransmit_.dbl() * 1000.0 << "ms]" << endl;
    }

    if (roundTripDelay <= SIMTIME_ZERO || !par("ntnCheckTimersCoverRoundTripDelay").boolValue())
        return;

    if (tPollRetransmit_ <= roundTripDelay)
        throw cRuntimeError("NtnNrRlcAmTxEntity::initialize - %s has t_PollRetransmit[%gms], which does not "
                "exceed the cell round-trip delay[%gms]. Every poll would expire before its STATUS report "
                "could physically arrive, so RETX_COUNT reaches maxRtxThreshold within a few poll periods "
                "and the bearer is released at both ends -- on a link with no errors at all. Raise "
                "t_PollRetransmit above %gms, or leave it at its default to have it derived from the "
                "geometry. Set ntnCheckTimersCoverRoundTripDelay=false if the violation is deliberate.",
                getFullPath().c_str(), tPollRetransmit_.dbl() * 1000.0, roundTripDelay.dbl() * 1000.0,
                roundTripDelay.dbl() * 1000.0);
}

} // namespace simu5g
