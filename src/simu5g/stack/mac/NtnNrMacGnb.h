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

#ifndef _NTNNRMACGNB_H_
#define _NTNNRMACGNB_H_

#include <map>
#include <set>

#include "simu5g/stack/mac/NrMacGnb.h"

namespace simu5g {

//
// NR MAC for a gNodeB serving its cell through a transparent NTN path; see
// NtnNrMacGnb.ned.
//
class NtnNrMacGnb : public NrMacGnb
{
  public:
    // GRANT TIMING IS PER CARRIER. A carrier's numerology fixes its slot duration, and
    // every quantity below is counted in the slots of the carrier the grant belongs to --
    // not in this module's own tick period, which is the shortest slot across all its
    // carriers and is therefore only the right unit for a single-carrier cell.
    //
    // All three are pure functions of the current time and the carrier, holding no
    // per-slot state. That is what makes them safe to call from ~NtnSchedulerGnbUl, which
    // runs during scheduling, and from sendLowerPackets(), which runs after it: both see
    // the same answer regardless of the order they ask in.
    bool ntnGrantTimingReady() const { return ntnCellRoundTripDelay_ > SIMTIME_ZERO; }

    // Slot duration of the given carrier, from the numerology the Binder holds for it.
    double ntnSlotDurationFor(inet::GHz carrierFrequency);

    // How far ahead of the current slot this cell books, in that carrier's slots.
    long ntnGrantOffsetSlotsFor(inet::GHz carrierFrequency);

    // The slot, on that carrier's grid, that resource blocks booked now are booked FOR.
    int64_t ntnTargetSlotFor(inet::GHz carrierFrequency);

  protected:
    // Uplink transport blocks currently inside the receive evaluation window at
    // this gNodeB. NOT a measure of stop-and-wait occupancy: a receive process is
    // held only from insertion until harqFbEvaluationTimer expires, which is a
    // fixed processing budget and does not grow with propagation delay. It says
    // whether uplink data is arriving at all, which over a satellite link is the
    // first thing to establish. The round-trip-scale occupancy is on the
    // transmitter, and is measured by ~NtnNrMacUe.
    static omnetpp::simsignal_t ntnHarqRxOccupancySignal_;

    // How far ahead of a grant's activation time it was issued, in slots.
    static omnetpp::simsignal_t ntnGrantActivationLeadSignal_;

    // Worst-case round trip of this cell. Constant for a circular orbit, and cached on
    // the association once derived, so the offsets below are constant too.
    omnetpp::simtime_t ntnCellRoundTripDelay_ = SIMTIME_ZERO;

    // Per carrier, memoised on first use: its slot duration and the resulting lookahead.
    // Both are checked for stability, since a change would leave grants already in flight
    // activating against a different slot grid.
    std::map<inet::GHz, double> ntnCarrierSlotDuration_;
    std::map<inet::GHz, long> ntnGrantOffsetSlots_;

    // UEs this gNodeB has actually heard from. Before a UE is in here only the cell-wide
    // bound exists for it.
    std::set<MacNodeId> ntnHeardFrom_;

    void handleSelfMessage() override;

    // Stamps outgoing uplink grants with the time from which they become valid.
    void sendLowerPackets(omnetpp::cPacket *pkt) override;

    // Records the sender as heard from, which is what allows its own delay to be used
    // in place of the cell-wide bound.
    void macPduUnmake(omnetpp::cPacket *pkt) override;

    // Re-reads the cell round-trip delay every slot, for the same reason ~NtnNrMacUe
    // refreshes there: the geometry it comes from does not exist during any init stage.
    void refreshNtnGrantTiming();

    // Round-trip delay to use for a UE: its own once heard from, read fresh every call,
    // and the cell bound before that.
    omnetpp::simtime_t ntnRoundTripDelayFor(MacNodeId ueId);

    // Walks the uplink HARQ receive buffers and emits their occupancy. Read-only:
    // it inspects process status and changes nothing.
    void emitNtnHarqRxOccupancy();
};

} //namespace

#endif
