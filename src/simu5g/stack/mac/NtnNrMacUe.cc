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

#include "simu5g/stack/mac/NtnNrMacUe.h"

#include <cmath>

#include <inet/common/ModuleAccess.h>

#include "simu5g/common/Ntn38331Timers.h"
#include "simu5g/common/NtnCommon.h"
#include "simu5g/common/binder/Binder.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(NtnNrMacUe);

namespace {

// Rounds a duration up to a whole number of slots. The inherited counters are
// slot counts, while the NTN parameters are expressed in time so that they stay
// correct under any numerology; rounding up keeps a timer from ever coming out
// shorter than the delay it is meant to cover.
unsigned int toSlots(double seconds, double slotDuration)
{
    // The epsilon keeps a duration that is already a whole number of slots from being
    // pushed to the next one by representation error: 0.542 + 0.040 does not divide
    // exactly by 0.001. At a 1ms slot it is worth a nanosecond, far below anything these
    // counters can express.
    return static_cast<unsigned int>(std::ceil(seconds / slotDuration - 1e-6));
}

} // namespace

void NtnNrMacUe::handleSelfMessage()
{
    // Refreshed here rather than in initialize(). The counters are derived from the cell's
    // round-trip delay, which comes from geometry, and no radio holds a valid position until
    // inet::INITSTAGE_SINGLE_MOBILITY -- after every Simu5G init stage, including the
    // INITSTAGE_SIMU5G_TTI_SETUP where ttiPeriod_ becomes known.
    //
    // Doing it on the TTI tick rather than at the latch points themselves is what makes the
    // coverage structural: this runs before checkRAC() latches raRespTimer_, and before all
    // three sites that latch bsrRtxTimer_, in the same slot.
    refreshNtnCounters();

    NrMacUe::handleSelfMessage();
}

void NtnNrMacUe::macHandleRac(cPacket *pkt)
{
    // The one latch point outside the TTI path: a failed attempt draws its backoff from
    // minRacBackoff_/maxRacBackoff_ here, on receipt of the response.
    refreshNtnCounters();

    NrMacUe::macHandleRac(pkt);
}

void NtnNrMacUe::refreshNtnCounters()
{
    // binder_ is the reference LteMacBase already resolves from its own binderModule parameter.
    simtime_t roundTripDelay = ntnCellRoundTripDelay(binder_.get(), cellId_);

    if (roundTripDelay <= SIMTIME_ZERO)
        throw cRuntimeError("NtnNrMacUe::refreshNtnCounters - UE %hu is served by cell %hu, which has no NTN "
                "association. Either this UE is not on a satellite path -- in which case it should not use "
                "this MAC type -- or the association was never registered.", num(nodeId_), num(cellId_));

    // 3GPP splits the wait into an offset that delays the start of the RAR window and the window
    // itself (TR 38.821 clause 7.2.1.1.1.2). Simu5G has a single counter, so they are summed
    // here -- the only place the two are combined.
    simtime_t responseWindowOffset = par("ntnRaResponseWindowOffset").doubleValue();
    if (responseWindowOffset < SIMTIME_ZERO)
        responseWindowOffset = roundTripDelay;

    simtime_t retxBsrTimer = par("ntnRetxBsrTimer").doubleValue();
    if (retxBsrTimer < SIMTIME_ZERO)
        retxBsrTimer = ntn38331Ceil(roundTripDelay, Ntn38331Timer::RetxBsrTimer);

    simtime_t racBackoffMax = par("ntnRacBackoffMax").doubleValue();
    if (racBackoffMax < SIMTIME_ZERO)
        racBackoffMax = ntn38331Floor(roundTripDelay * 2, Ntn38331Timer::BackoffIndicator);

    unsigned int raRespWinStart = toSlots(responseWindowOffset.dbl() + par("ntnRaResponseWindow").doubleValue(), ttiPeriod_);
    unsigned int bsrRtxTimerStart = toSlots(retxBsrTimer.dbl(), ttiPeriod_);
    unsigned int minRacBackoff = toSlots(par("ntnRacBackoffMin").doubleValue(), ttiPeriod_);
    unsigned int maxRacBackoff = toSlots(racBackoffMax.dbl(), ttiPeriod_);

    bool changed = raRespWinStart != raRespWinStart_ || bsrRtxTimerStart != bsrRtxTimerStart_
        || minRacBackoff != minRacBackoff_ || maxRacBackoff != maxRacBackoff_;

    raRespWinStart_ = raRespWinStart;
    bsrRtxTimerStart_ = bsrRtxTimerStart;
    minRacBackoff_ = minRacBackoff;
    maxRacBackoff_ = maxRacBackoff;

    if (!changed)
        return;

    // Reported on change, at INFO because a counter silently left at its terrestrial value is
    // indistinguishable from a working one, and the resulting preamble storm looks like a channel
    // problem rather than a configuration one.
    EV_INFO << "NtnNrMacUe::refreshNtnCounters - UE " << nodeId_ << " in cell " << cellId_
            << ", round-trip delay[" << roundTripDelay.dbl() * 1000.0 << "ms], slot "
            << ttiPeriod_ * 1000.0 << "ms: raResponseWindow[" << raRespWinStart_ << " slots = "
            << responseWindowOffset.dbl() * 1000.0 << "ms offset + "
            << par("ntnRaResponseWindow").doubleValue() * 1000.0 << "ms window]"
            << ", retxBsrTimer[" << bsrRtxTimerStart_ << " slots = " << retxBsrTimer.dbl() * 1000.0 << "ms]"
            << ", racBackoff[" << minRacBackoff_ << ".." << maxRacBackoff_ << " slots]" << endl;
}

} //namespace
