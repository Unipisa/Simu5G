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

#include <cmath>

#include "simu5g/common/NtnCommon.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/stack/mac/packet/LteSchedulingGrant.h"
#include "simu5g/stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "simu5g/stack/mac/buffer/harq/LteHarqProcessRx.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(NtnNrMacGnb);

simsignal_t NtnNrMacGnb::ntnHarqRxOccupancySignal_ = registerSignal("ntnHarqRxOccupancy");
simsignal_t NtnNrMacGnb::ntnGrantActivationLeadSignal_ = registerSignal("ntnGrantActivationLead");
simsignal_t NtnNrMacGnb::ntnTargetSlotSkipsSignal_ = registerSignal("ntnTargetSlotSkips");

int64_t NtnNrMacGnb::ntnCurrentSlot() const
{
    // Every MAC schedules its first tick at ttiPeriod_ and every later one a period
    // apart, so the grid is a whole number of periods from zero.
    return static_cast<int64_t>(std::llround(NOW.dbl() / ttiPeriod_));
}

void NtnNrMacGnb::handleSelfMessage()
{
    // Before the parent, so the blocks it is about to book already belong to a known
    // reception slot. Refreshed per slot rather than in initialize() for the reason
    // given in ~NtnNrMacUe: no radio holds a valid position until
    // inet::INITSTAGE_SINGLE_MOBILITY, which runs after every Simu5G init stage.
    refreshNtnGrantTiming();

    NrMacGnb::handleSelfMessage();

    // After the slot rather than before it. The parent extracts correct PDUs first and
    // purges corrupted ones last, so this reads the slot's minimum occupancy rather than
    // its peak -- consistent from slot to slot, which is what matters for a trend.
    emitNtnHarqRxOccupancy();
}

void NtnNrMacGnb::refreshNtnGrantTiming()
{
    ntnCellRoundTripDelay_ = ntnCellRoundTripDelay(binder_.get(), getMacCellId());

    if (ntnCellRoundTripDelay_ <= SIMTIME_ZERO)
        throw cRuntimeError("NtnNrMacGnb::refreshNtnGrantTiming - cell %hu has no NTN association. Either "
                "this cell is not on a satellite path -- in which case it should not use this MAC type -- "
                "or the association was never registered.", num(getMacCellId()));

    long offset = par("ntnGrantOffsetSlots").intValue();
    if (offset < 0) {
        // The blocks booked in this slot are for the slot in which the granted
        // transmission will be heard. Bounding that by the cell's worst-case round trip
        // is what keeps the offset a single cell-wide value, which it has to be: a UE
        // that has not been heard from yet must still be given a grant, and the
        // reception slot has to advance by exactly one per slot (see below).
        offset = ntnDurationToSlots(ntnCellRoundTripDelay_.dbl(), ttiPeriod_)
               + ntnDurationToSlots(par("ntnGrantProcessingDelay").doubleValue(), ttiPeriod_);
    }

    int64_t wanted = ntnCurrentSlot() + offset;

    if (!ntnTargetSlotValid_) {
        ntnTargetSlot_ = wanted;
        ntnTargetSlotValid_ = true;
    }
    else {
        // The reception slot must advance by at least one every slot. Two slots
        // targeting one reception slot would book the same resource blocks twice and
        // nothing downstream would notice, so a shrinking offset costs a slot of
        // latency instead; a growing one leaves a slot unbooked.
        int64_t next = std::max(ntnTargetSlot_ + 1, wanted);
        if (next != wanted) {
            EV_INFO << "NtnNrMacGnb::refreshNtnGrantTiming - cell " << getMacCellId()
                    << " held its target reception slot at " << next << " rather than " << wanted
                    << "; the derived grant offset shrank" << endl;
            emit(ntnTargetSlotSkipsSignal_, 1);
        }
        ntnTargetSlot_ = next;
    }

    if (offset == ntnGrantOffsetSlots_)
        return;

    // On change only, at INFO: an offset silently left at zero is indistinguishable from
    // a working one, and the resulting loss looks like a channel problem.
    EV_INFO << "NtnNrMacGnb::refreshNtnGrantTiming - cell " << getMacCellId()
            << ", round-trip delay[" << ntnCellRoundTripDelay_.dbl() * 1000.0 << "ms], slot "
            << ttiPeriod_ * 1000.0 << "ms: uplink grant offset[" << offset << " slots], "
            << "booking reception slot " << ntnTargetSlot_ << endl;

    ntnGrantOffsetSlots_ = offset;
}

simtime_t NtnNrMacGnb::ntnRoundTripDelayFor(MacNodeId ueId)
{
    // A UE the gNodeB has never heard from gets the cell-wide bound. Using its own delay
    // earlier would model a network that knew a UE's range before it connected, which is
    // precisely the assumption the specifications refuse to make: Rel-17 signals the
    // cell-specific K_offset in SIB19 and refines it per UE by MAC CE only after access.
    if (!par("ntnPerUeGrantTiming").boolValue() || ntnHeardFrom_.find(ueId) == ntnHeardFrom_.end())
        return ntnCellRoundTripDelay_;

    simtime_t validity = par("ntnUlSyncValidityDuration").doubleValue();
    auto cached = ntnUeRoundTripDelay_.find(ueId);

    if (cached != ntnUeRoundTripDelay_.end() && NOW - cached->second.second < validity)
        return cached->second.first;

    // Latched rather than read every slot. A real UE fixes these durations when the
    // procedure starts, and ntn-UlSyncValidityDuration is the specification's own bound
    // on how long assistance data stays usable.
    simtime_t measured = ntnRoundTripDelay(binder_.get(), getMacCellId(), ueId);

    if (measured <= SIMTIME_ZERO)
        return ntnCellRoundTripDelay_;

    if (cached != ntnUeRoundTripDelay_.end()) {
        long margin = par("ntnGrantTimingMarginSlots").intValue();
        double moved = std::fabs((measured - cached->second.first).dbl());
        if (moved > margin * ttiPeriod_)
            throw cRuntimeError("NtnNrMacGnb::ntnRoundTripDelayFor - the round-trip delay of UE %hu moved "
                    "by %gms (%g slots) over the %gms since it was last read, more than the %ld slots "
                    "ntnGrantTimingMarginSlots allows. Grants issued from the old value are still in "
                    "flight and would activate in the wrong slot. Shorten ntnUlSyncValidityDuration, or "
                    "raise ntnGrantTimingMarginSlots if the geometry really does move this fast.",
                    num(ueId), moved * 1000.0, moved / ttiPeriod_,
                    (NOW - cached->second.second).dbl() * 1000.0, margin);
    }

    ntnUeRoundTripDelay_[ueId] = {measured, NOW};

    EV_INFO << "NtnNrMacGnb::ntnRoundTripDelayFor - cell " << getMacCellId() << " read UE " << ueId
            << " round-trip delay[" << measured.dbl() * 1000.0 << "ms] against cell bound["
            << ntnCellRoundTripDelay_.dbl() * 1000.0 << "ms]" << endl;

    return measured;
}

void NtnNrMacGnb::macPduUnmake(cPacket *pktAux)
{
    // The first point at which this gNodeB has provably heard the UE, which is what
    // makes its own delay usable in place of the cell bound.
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    ntnHeardFrom_.insert(pkt->getTag<UserControlInfo>()->getSourceId());

    NrMacGnb::macPduUnmake(pktAux);
}

void NtnNrMacGnb::sendLowerPackets(cPacket *pktAux)
{
    // Every grant leaves through here, from both LteMacEnb::sendGrants() and the
    // LteMacEnbD2D override that NrMacGnb actually inherits, so the activation time is
    // applied in one place. Everything else this MAC sends -- downlink PDUs, HARQ
    // feedback, D2D mode switches -- passes through untouched.
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto userInfo = pkt->getTagForUpdate<UserControlInfo>();

    // The direction comes from the grant itself, not from the control info: sendGrants()
    // sets it on the chunk and leaves the tag at its DL default. Reading the chunk is
    // also what correctly excludes the D2D grants the same method emits.
    bool isUplinkGrant = false;
    if (userInfo->getFrameType() == GRANTPKT)
        isUplinkGrant = pkt->peekAtFront<LteSchedulingGrant>()->getDirection() == UL;

    if (isUplinkGrant && ntnTargetSlotValid_) {
        MacNodeId ueId = userInfo->getDestId();

        // The UE must transmit this many slots before the reception slot for its signal
        // to arrive in it. Rounding up puts the transmission a fraction of a slot early
        // rather than late, which is the side to err on when the arrival decides which
        // slot's blocks are used.
        long uplinkSlots = ntnDurationToSlots(ntnRoundTripDelayFor(ueId).dbl() / 2.0, ttiPeriod_);
        simtime_t activation = (ntnTargetSlot_ - uplinkSlots) * ttiPeriod_;

        userInfo->setGrantActivationTime(activation);
        userInfo->setGrantIssueTime(NOW);

        emit(ntnGrantActivationLeadSignal_, (activation - NOW).dbl() / ttiPeriod_);

        EV_DEBUG << "NtnNrMacGnb::sendLowerPackets - grant to UE " << ueId << " books reception slot "
                 << ntnTargetSlot_ << ", valid from t=" << activation << endl;
    }

    NrMacGnb::sendLowerPackets(pktAux);
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
