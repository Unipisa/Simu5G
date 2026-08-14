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

#include "simu5g/common/NtnCommon.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/stack/mac/buffer/harq/LteHarqBufferTx.h"
#include "simu5g/stack/mac/buffer/harq/LteHarqProcessTx.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(NtnNrMacUe);

simsignal_t NtnNrMacUe::ntnHarqTxOccupancySignal_ = registerSignal("ntnHarqTxOccupancy");
simsignal_t NtnNrMacUe::ntnHarqTxStallSignal_ = registerSignal("ntnHarqTxStall");
simsignal_t NtnNrMacUe::ntnGrantHoldTimeSignal_ = registerSignal("ntnGrantHoldTime");
simsignal_t NtnNrMacUe::ntnPendingGrantsSignal_ = registerSignal("ntnPendingGrants");

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
    promoteDueGrants();

    NrMacUe::handleSelfMessage();

    // Before this slot's own PDU is inserted (macSduRequest() only requests it; the
    // grant is still installed until flushHarqBuffers() later this instant).
    emitNtnHarqTxState();
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

    unsigned int raRespWinStart = ntnDurationToSlots(responseWindowOffset.dbl() + par("ntnRaResponseWindow").doubleValue(), ttiPeriod_);
    unsigned int bsrRtxTimerStart = ntnDurationToSlots(retxBsrTimer.dbl(), ttiPeriod_);
    unsigned int minRacBackoff = ntnDurationToSlots(par("ntnRacBackoffMin").doubleValue(), ttiPeriod_);
    unsigned int maxRacBackoff = ntnDurationToSlots(racBackoffMax.dbl(), ttiPeriod_);

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

void NtnNrMacUe::macHandleGrant(cPacket *pktAux)
{
    // Read before the parent deletes the packet.
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto userInfo = pkt->getTag<UserControlInfo>();
    GHz carrierFrequency = userInfo->getCarrierFrequency();
    simtime_t activationTime = userInfo->getGrantActivationTime();

    NrMacUe::macHandleGrant(pktAux);

    // Zero = valid on receipt, the terrestrial default. Nothing to hold.
    if (activationTime == SIMTIME_ZERO)
        return;

    auto it = schedulingGrant_.find(carrierFrequency);
    if (it == schedulingGrant_.end() || it->second == nullptr)
        return;

    if (activationTime < NOW) {
        // Causality failure: the gNodeB booked blocks this UE could not reach in time.
        // No opt-out -- using it would transmit on another slot's blocks.
        throw cRuntimeError("NtnNrMacUe::macHandleGrant - UE %hu received a grant at t=%gs whose "
                "activation time was t=%gs, %gms in the past. The uplink grant offset does not cover "
                "this UE's round-trip delay, so the gNodeB is booking resource blocks for a slot the "
                "UE cannot transmit in. Raise ntnGrantOffsetSlots on the gNodeB.",
                 num(nodeId_), NOW.dbl(), activationTime.dbl(), (NOW - activationTime).dbl() * 1000.0);
    }

    if (activationTime == NOW)
        return; // usable now, no need to hold it

    auto& pending = pendingGrants_[carrierFrequency];

    if (!pending.insert({activationTime, it->second}).second)
        throw cRuntimeError("NtnNrMacUe::macHandleGrant - UE %hu already holds a grant activating at "
                "t=%gs. Two grants cannot be valid in the same slot on one carrier: the gNodeB booked "
                "one reception slot twice.", num(nodeId_), activationTime.dbl());

    it->second = nullptr; // parked here; flushHarqBuffers() only clears the active slot

    emit(ntnGrantHoldTimeSignal_, activationTime - NOW);
}

void NtnNrMacUe::promoteDueGrants()
{
    // Positive: refreshNtnCounters() already validated the NTN association this slot.
    simtime_t cellRoundTripDelay = ntnCellRoundTripDelay(binder_.get(), cellId_);

    unsigned int outstanding = 0;

    for (auto& [carrierFrequency, pending] : pendingGrants_) {
        // Everything before upper_bound(NOW) is due.
        auto notYetDue = pending.upper_bound(NOW);

        if (notYetDue != pending.begin()) {
            auto due = std::prev(notYetDue);

            // Exactly one grant may come due per tick: consecutive grants for a carrier
            // activate one carrier-slot apart, and this UE ticks at least that often.
            // More than one means a missed tick, not something to arbitrate between --
            // using the newest would transmit on blocks a discarded grant owned.
            unsigned int dueCount = std::distance(pending.begin(), notYetDue);
            if (dueCount > 1)
                throw cRuntimeError("NtnNrMacUe::promoteDueGrants - UE %hu has %u grants due at once on "
                        "carrier %gGHz, activating between t=%gs and t=%gs. Only one can be: the gNodeB "
                        "books one reception slot per slot of that carrier, using that carrier's exact "
                        "current geometry each time, and this UE ticks at least as often as the carrier "
                        "does. This UE most likely missed a tick.",
                        num(nodeId_), dueCount, carrierFrequency.get(),
                        pending.begin()->first.dbl(), due->first.dbl());

            schedulingGrant_[carrierFrequency] = due->second;
            pending.erase(pending.begin(), notYetDue);
        }

        // Runaway detector, derived not configured: at most D grants (the carrier's
        // offset, ~NtnNrMacGnb) are ever legitimately outstanding, doubled for margin
        // (start-up transients, k2 this UE cannot see directly).
        double slotDuration = binder_->getSlotDurationFromNumerologyIndex(
                binder_->getNumerologyIndexFromCarrierFreq(carrierFrequency));
        unsigned int limit = 2 * ntnDurationToSlots(cellRoundTripDelay.dbl(), slotDuration);

        if (pending.size() > limit)
            throw cRuntimeError("NtnNrMacUe::promoteDueGrants - UE %hu holds %u grants on carrier %gGHz "
                    "that have not become valid, above the %u legitimately possible for a %gms round trip "
                    "at a %gms slot. Grants are being issued and never consumed, which means their "
                    "activation times are not arriving -- check the uplink grant offset on the gNodeB.",
                    num(nodeId_), (unsigned int)pending.size(), carrierFrequency.get(), limit,
                    cellRoundTripDelay.dbl() * 1000.0, slotDuration * 1000.0);

        outstanding += pending.size();
    }

    emit(ntnPendingGrantsSignal_, outstanding);
}

void NtnNrMacUe::emitNtnHarqTxState()
{
    unsigned int occupied = 0;
    unsigned int total = 0;

    // Summed over all transmit buffers; fine on the NTN path (one carrier, one
    // destination -- would need splitting for D2D or carrier aggregation).
    for (const auto& [carrierFrequency, buffers] : harqTxBuffers_) {
        for (const auto& [destinationId, buffer] : buffers) {
            unsigned int processes = buffer->getNumProcesses();
            total += processes;
            for (unsigned int process = 0; process < processes; ++process) {
                if (!buffer->getProcess(process)->isEmpty())
                    ++occupied;
            }
        }
    }

    emit(ntnHarqTxOccupancySignal_, occupied);

    // A UE with no grant is waiting on the grant loop, not stalled on HARQ.
    bool holdsGrant = false;
    for (const auto& [carrierFrequency, grant] : schedulingGrant_) {
        if (grant != nullptr) {
            holdsGrant = true;
            break;
        }
    }

    if (holdsGrant && total > 0)
        emit(ntnHarqTxStallSignal_, occupied == total ? 1 : 0);
}

} //namespace
