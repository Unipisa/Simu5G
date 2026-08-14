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

    // Numerology is per-carrier (Binder), so gNodeB and UE agree on slot duration
    // without signalling it.
    double slotDuration = binder_->getSlotDurationFromNumerologyIndex(
            binder_->getNumerologyIndexFromCarrierFreq(carrierFrequency));

    if (slotDuration <= 0)
        throw cRuntimeError("NtnNrMacGnb::ntnSlotDurationFor - carrier %gGHz has no registered numerology, "
                "so the slot its grants are timed against is unknown.", carrierFrequency.get());

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
        offset = ntnDurationToSlots(ntnCellRoundTripDelay_.dbl(), slotDuration)
               + ntnDurationToSlots(par("ntnGrantProcessingDelay").doubleValue(), slotDuration);
    }

    auto cached = ntnGrantOffsetSlots_.find(carrierFrequency);
    if (cached == ntnGrantOffsetSlots_.end()) {
        EV_INFO << "NtnNrMacGnb::ntnGrantOffsetSlotsFor - cell " << getMacCellId() << " carrier "
                << carrierFrequency << ", round-trip delay[" << ntnCellRoundTripDelay_.dbl() * 1000.0
                << "ms], slot " << slotDuration * 1000.0 << "ms: uplink grant offset[" << offset
                << " slots]" << endl;
        ntnGrantOffsetSlots_[carrierFrequency] = offset;
        return offset;
    }

    // Cannot happen for a circular orbit (cell RTD is derived once, cached on the
    // association); a change means the association was rebuilt underneath us.
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

    // Floor, not round: a carrier below the cell's highest numerology ticks less often
    // than this module, so NOW is usually mid-slot for it.
    int64_t currentSlot = static_cast<int64_t>(std::floor(NOW.dbl() / slotDuration + 1e-9));

    return currentSlot + ntnGrantOffsetSlotsFor(carrierFrequency);
}

void NtnNrMacGnb::handleSelfMessage()
{
    refreshNtnGrantTiming();

    NrMacGnb::handleSelfMessage();

    emitNtnHarqRxOccupancy();
}

void NtnNrMacGnb::refreshNtnGrantTiming()
{
    ntnCellRoundTripDelay_ = ntnCellRoundTripDelay(binder_.get(), getMacCellId());

    if (ntnCellRoundTripDelay_ <= SIMTIME_ZERO)
        throw cRuntimeError("NtnNrMacGnb::refreshNtnGrantTiming - cell %hu has no NTN association. Either "
                "this cell is not on a satellite path -- in which case it should not use this MAC type -- "
                "or the association was never registered.", num(getMacCellId()));
}

simtime_t NtnNrMacGnb::ntnRoundTripDelayFor(MacNodeId ueId)
{
    // TS 38.331: cell-specific K_offset (SIB19) until MAC CE refines it per UE after
    // access -- using a UE's own delay earlier would assume its range before connecting.
    if (!par("ntnPerUeGrantTiming").boolValue() || ntnHeardFrom_.find(ueId) == ntnHeardFrom_.end())
        return ntnCellRoundTripDelay_;

    simtime_t measured = ntnRoundTripDelay(binder_.get(), getMacCellId(), ueId);

    return measured > SIMTIME_ZERO ? measured : ntnCellRoundTripDelay_;
}

void NtnNrMacGnb::macPduUnmake(cPacket *pktAux)
{
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    ntnHeardFrom_.insert(pkt->getTag<UserControlInfo>()->getSourceId());

    NrMacGnb::macPduUnmake(pktAux);
}

void NtnNrMacGnb::sendLowerPackets(cPacket *pktAux)
{
    // Every grant leaves through here (LteMacEnbD2D::sendGrants(), the live override).
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto userInfo = pkt->getTagForUpdate<UserControlInfo>();

    // sendGrants() sets direction on the grant chunk, not the tag (which stays DL); the
    // chunk is also what excludes D2D grants from the same method.
    bool isUplinkGrant = false;
    if (userInfo->getFrameType() == GRANTPKT)
        isUplinkGrant = pkt->peekAtFront<LteSchedulingGrant>()->getDirection() == UL;

    if (isUplinkGrant && ntnGrantTimingReady()) {
        MacNodeId ueId = userInfo->getDestId();

        GHz carrierFrequency = userInfo->getCarrierFrequency();
        double slotDuration = ntnSlotDurationFor(carrierFrequency);
        int64_t targetSlot = ntnTargetSlotFor(carrierFrequency);

        // Round up: a fraction-of-a-slot-early transmission is the side to err on.
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
                if (!buffer->getProcess(process)->isEmpty())
                    ++occupied;
            }
        }
    }

    emit(ntnHarqRxOccupancySignal_, occupied);
}

} //namespace
