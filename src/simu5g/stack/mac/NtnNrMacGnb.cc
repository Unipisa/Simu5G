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

double NtnNrMacGnb::ntnSlotDurationFor(GHz carrierFrequency)
{
    auto cached = ntnCarrierSlotDuration_.find(carrierFrequency);
    if (cached != ntnCarrierSlotDuration_.end())
        return cached->second;

    // Numerology is a property of the carrier, registered once in the Binder, so the
    // gNodeB and every UE on that carrier necessarily agree on its slot duration. That
    // agreement is what lets an activation time computed here be interpreted correctly
    // at the UE.
    double slotDuration = binder_->getSlotDurationFromNumerologyIndex(
            binder_->getNumerologyIndexFromCarrierFreq(carrierFrequency));

    if (slotDuration <= 0)
        throw cRuntimeError("NtnNrMacGnb::ntnSlotDurationFor - carrier %gGHz has no registered numerology, "
                "so the slot its grants are timed against is unknown.", carrierFrequency.get());

    // This module ticks at its shortest carrier's slot, so no carrier may be shorter.
    if (slotDuration < ttiPeriod_ - 1e-12)
        throw cRuntimeError("NtnNrMacGnb::ntnSlotDurationFor - carrier %gGHz has a slot of %gms, shorter "
                "than this MAC's own tick of %gms. The tick is taken from the cell's highest numerology, "
                "so this means the carrier is not one of the cell's.",
                carrierFrequency.get(), slotDuration * 1000.0, ttiPeriod_ * 1000.0);

    ntnCarrierSlotDuration_[carrierFrequency] = slotDuration;
    return slotDuration;
}

long NtnNrMacGnb::ntnGrantOffsetSlotsFor(GHz carrierFrequency)
{
    double slotDuration = ntnSlotDurationFor(carrierFrequency);

    long offset = par("ntnGrantOffsetSlots").intValue();
    if (offset < 0) {
        // The blocks booked now are for the slot in which the granted transmission will be
        // heard. Bounding that by the cell's worst-case round trip is what keeps the offset
        // one value for the whole carrier, which it has to be: a UE that has not been heard
        // from yet must still be given a grant, and each of the carrier's slots has to book
        // a distinct reception slot.
        offset = ntnDurationToSlots(ntnCellRoundTripDelay_.dbl(), slotDuration)
               + ntnDurationToSlots(par("ntnGrantProcessingDelay").doubleValue(), slotDuration);
    }

    auto cached = ntnGrantOffsetSlots_.find(carrierFrequency);
    if (cached == ntnGrantOffsetSlots_.end()) {
        // On first use only, at INFO: an offset silently left at zero is indistinguishable
        // from a working one, and the resulting loss looks like a channel problem.
        EV_INFO << "NtnNrMacGnb::ntnGrantOffsetSlotsFor - cell " << getMacCellId() << " carrier "
                << carrierFrequency << ", round-trip delay[" << ntnCellRoundTripDelay_.dbl() * 1000.0
                << "ms], slot " << slotDuration * 1000.0 << "ms: uplink grant offset[" << offset
                << " slots]" << endl;
        ntnGrantOffsetSlots_[carrierFrequency] = offset;
        return offset;
    }

    // The offset is what maps a scheduling slot onto the reception slot it books, so
    // changing it mid-run would point two different slots at one reception slot, or leave
    // grants already in flight activating against a grid that has moved. It cannot happen
    // for a circular orbit -- the cell round-trip delay is derived once and cached on the
    // association -- so a change means the association was rebuilt underneath us.
    if (offset != cached->second)
        throw cRuntimeError("NtnNrMacGnb::ntnGrantOffsetSlotsFor - the uplink grant offset of cell %hu on "
                "carrier %gGHz changed from %ld to %ld slots mid-run. Grants issued from the old value are "
                "still in flight and would activate against a different reception slot.",
                num(getMacCellId()), carrierFrequency.get(), cached->second, offset);

    return offset;
}

int64_t NtnNrMacGnb::ntnTargetSlotFor(GHz carrierFrequency)
{
    double slotDuration = ntnSlotDurationFor(carrierFrequency);

    // Which of this carrier's slots the cell is currently in. Floor rather than round:
    // a carrier whose numerology is lower than the cell's ticks less often than this
    // module does, so NOW is generally partway through one of its slots rather than on
    // its boundary. The epsilon absorbs the representation error of dividing a simulation
    // time that is an exact multiple of one slot length by a different one.
    int64_t currentSlot = static_cast<int64_t>(std::floor(NOW.dbl() / slotDuration + 1e-9));

    // No state, so it advances exactly when the carrier's own slot advances, and every
    // caller in a slot gets the same answer whatever order they ask in.
    return currentSlot + ntnGrantOffsetSlotsFor(carrierFrequency);
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

    // Nothing else to do here: the offset and the reception slot are derived per carrier,
    // on demand, from this value and the time -- see ntnTargetSlotFor().
}

simtime_t NtnNrMacGnb::ntnRoundTripDelayFor(MacNodeId ueId)
{
    // A UE the gNodeB has never heard from gets the cell-wide bound. Using its own delay
    // earlier would model a network that knew a UE's range before it connected, which is
    // precisely the assumption the specifications refuse to make: Rel-17 signals the
    // cell-specific K_offset in SIB19 and refines it per UE by MAC CE only after access.
    if (!par("ntnPerUeGrantTiming").boolValue() || ntnHeardFrom_.find(ueId) == ntnHeardFrom_.end())
        return ntnCellRoundTripDelay_;

    // Read fresh on every call rather than latched. A real UE would fix this at the start
    // of a procedure and rely on assistance data staying valid for a while, but this is
    // exact geometry available on demand: nothing is gained by caching it, and a cached
    // value could only be less accurate than reading it again.
    simtime_t measured = ntnRoundTripDelay(binder_.get(), getMacCellId(), ueId);

    return measured > SIMTIME_ZERO ? measured : ntnCellRoundTripDelay_;
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

    if (isUplinkGrant && ntnGrantTimingReady()) {
        MacNodeId ueId = userInfo->getDestId();

        // Everything here is counted in the slots of the carrier this grant is for, not in
        // this module's own tick period. The two coincide only when the cell has a single
        // carrier, or when all its carriers share the highest numerology.
        GHz carrierFrequency = userInfo->getCarrierFrequency();
        double slotDuration = ntnSlotDurationFor(carrierFrequency);
        int64_t targetSlot = ntnTargetSlotFor(carrierFrequency);

        // The UE must transmit this many slots before the reception slot for its signal
        // to arrive in it. Rounding up puts the transmission a fraction of a slot early
        // rather than late, which is the side to err on when the arrival decides which
        // slot's blocks are used.
        long uplinkSlots = ntnDurationToSlots(ntnRoundTripDelayFor(ueId).dbl() / 2.0, slotDuration);
        simtime_t activation = (targetSlot - uplinkSlots) * slotDuration;

        userInfo->setGrantActivationTime(activation);
        userInfo->setGrantIssueTime(NOW);

        emit(ntnGrantActivationLeadSignal_, (activation - NOW).dbl() / slotDuration);

        EV_DEBUG << "NtnNrMacGnb::sendLowerPackets - grant to UE " << ueId << " on carrier "
                 << carrierFrequency << " books reception slot " << targetSlot << ", valid from t="
                 << activation << endl;
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
