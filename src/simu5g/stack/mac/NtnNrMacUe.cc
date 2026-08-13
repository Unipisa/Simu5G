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
simsignal_t NtnNrMacUe::ntnLateGrantsSignal_ = registerSignal("ntnLateGrants");
simsignal_t NtnNrMacUe::ntnGrantsSkippedSignal_ = registerSignal("ntnGrantsSkipped");

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

    // Before the parent, so a grant that becomes valid in this slot is already
    // installed when the parent looks for one.
    promoteDueGrants();

    NrMacUe::handleSelfMessage();

    // Sampled here, which is BEFORE this slot's own transport block is inserted:
    // macSduRequest() only asks RLC for the SDUs, and the PDU is built and inserted
    // by handleUpperMessage() in a later event of the same instant. That is the point
    // worth sampling rather than an accident of placement -- occupancy equal to the
    // pool size here is exactly the condition under which firstAvailable() will return
    // no unit and macPduMake() will drop the PDU.
    //
    // The grant is still installed: flushHarqBuffers(), which clears it, runs later in
    // the same instant on the priority-1 self message.
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
    // Read before the parent runs: it deletes the packet, and the tag with it.
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto userInfo = pkt->getTag<UserControlInfo>();
    GHz carrierFrequency = userInfo->getCarrierFrequency();
    simtime_t activationTime = userInfo->getNtnGrantActivationTime();

    // The parent installs the grant and clears racRequested_. Clearing that on receipt
    // is correct and must not be deferred: the grant is the answer to the preamble
    // whatever slot it turns out to be valid for.
    NrMacUe::macHandleGrant(pktAux);

    // Zero means "valid on receipt", which is the terrestrial semantics and what every
    // gNodeB sends until the uplink grant offset is switched on. Nothing to hold.
    if (activationTime == SIMTIME_ZERO)
        return;

    auto it = schedulingGrant_.find(carrierFrequency);
    if (it == schedulingGrant_.end() || it->second == nullptr)
        return;

    if (activationTime < NOW) {
        // The grant is for a slot that has already gone. This is the causality failure
        // the activation time exists to prevent: the gNodeB booked resource blocks the
        // UE could not reach in time, so the offset does not cover this UE's delay.
        if (!par("ntnAllowLateGrants").boolValue())
            throw cRuntimeError("NtnNrMacUe::macHandleGrant - UE %hu received a grant at t=%gs whose "
                    "activation time was t=%gs, %gms in the past. The uplink grant offset does not cover "
                    "this UE's round-trip delay, so the gNodeB is booking resource blocks for a slot the "
                    "UE cannot transmit in. Raise ntnGrantOffsetSlots on the gNodeB, or set "
                    "ntnAllowLateGrants=true to use such grants in the next slot instead.",
                    num(nodeId_), NOW.dbl(), activationTime.dbl(), (NOW - activationTime).dbl() * 1000.0);

        EV_INFO << "NtnNrMacUe::macHandleGrant - UE " << nodeId_ << " received a grant "
                << (NOW - activationTime).dbl() * 1000.0 << "ms after its activation time; using it now"
                << endl;
        emit(ntnLateGrantsSignal_, 1);
        return;
    }

    if (activationTime == NOW)
        return; // usable in this very slot, no need to hold it

    auto& pending = pendingGrants_[carrierFrequency];

    if (!pending.insert({activationTime, it->second}).second)
        throw cRuntimeError("NtnNrMacUe::macHandleGrant - UE %hu already holds a grant activating at "
                "t=%gs. Two grants cannot be valid in the same slot on one carrier: the gNodeB booked "
                "one reception slot twice.", num(nodeId_), activationTime.dbl());

    // Held out of the active slot until its time. flushHarqBuffers() clears only the
    // active grant, so what is parked here survives the slot.
    it->second = nullptr;

    emit(ntnGrantHoldTimeSignal_, activationTime - NOW);
}

void NtnNrMacUe::promoteDueGrants()
{
    unsigned int outstanding = 0;

    for (auto& [carrierFrequency, pending] : pendingGrants_) {
        // upper_bound(NOW) is the first grant that is still in the future, so everything
        // before it is due.
        auto notYetDue = pending.upper_bound(NOW);

        if (notYetDue != pending.begin()) {
            auto due = std::prev(notYetDue);

            // Only the newest due grant is usable. An older one names a slot that has
            // already passed, which can only happen if this UE missed a slot; it is
            // counted rather than silently dropped.
            unsigned int skipped = std::distance(pending.begin(), due);
            if (skipped > 0) {
                EV_INFO << "NtnNrMacUe::promoteDueGrants - UE " << nodeId_ << " discarded " << skipped
                        << " grant(s) whose slot passed unused" << endl;
                emit(ntnGrantsSkippedSignal_, skipped);
            }

            schedulingGrant_[carrierFrequency] = due->second;
            pending.erase(pending.begin(), notYetDue);
        }

        outstanding += pending.size();
    }

    // A runaway detector, not a design limit: at a GEO round trip with a grant every
    // slot, of the order of a thousand are legitimately outstanding.
    unsigned int limit = par("ntnMaxPendingGrants").intValue();
    if (outstanding > limit)
        throw cRuntimeError("NtnNrMacUe::promoteDueGrants - UE %hu holds %u grants that have not become "
                "valid, above ntnMaxPendingGrants (%u). Grants are being issued and never consumed, which "
                "means their activation times are not arriving -- check the uplink grant offset on the "
                "gNodeB.", num(nodeId_), outstanding, limit);

    emit(ntnPendingGrantsSignal_, outstanding);
}

void NtnNrMacUe::emitNtnHarqTxState()
{
    unsigned int occupied = 0;
    unsigned int total = 0;

    // Every transmit buffer, summed. harqTxBuffers_ is keyed by destination, so with
    // D2D enabled it would also hold peer-UE buffers, and with carrier aggregation the
    // sum spans carriers whose slots do not coincide. Neither happens on the NTN path
    // -- one carrier, one destination, the serving gNodeB -- and both would need this
    // split per destination and per carrier to stay meaningful.
    for (const auto& [carrierFrequency, buffers] : harqTxBuffers_) {
        for (const auto& [destinationId, buffer] : buffers) {
            unsigned int processes = buffer->getNumProcesses();
            total += processes;
            for (unsigned int process = 0; process < processes; ++process) {
                // A process counts as occupied whenever it is not empty, which is the
                // condition under which firstAvailable() will not hand it out.
                if (!buffer->getProcess(process)->isEmpty())
                    ++occupied;
            }
        }
    }

    emit(ntnHarqTxOccupancySignal_, occupied);

    // Only meaningful on a slot where the UE actually had something to transmit with.
    // A UE with no grant is not stalled on HARQ, it is waiting on the grant loop, and
    // conflating the two is exactly the attribution this statistic exists to make.
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
