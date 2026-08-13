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
  protected:
    // Uplink transport blocks currently inside the receive evaluation window at
    // this gNodeB. NOT a measure of stop-and-wait occupancy: a receive process is
    // held only from insertion until harqFbEvaluationTimer expires, which is a
    // fixed processing budget and does not grow with propagation delay. It says
    // whether uplink data is arriving at all, which over a satellite link is the
    // first thing to establish. The round-trip-scale occupancy is on the
    // transmitter, and is measured by ~NtnNrMacUe.
    static omnetpp::simsignal_t ntnHarqRxOccupancySignal_;

    // How far ahead of a grant's activation time it was issued, in slots, and how
    // often the target reception slot had to be forced forward.
    static omnetpp::simsignal_t ntnGrantActivationLeadSignal_;
    static omnetpp::simsignal_t ntnTargetSlotSkipsSignal_;

    // Worst-case round trip of this cell, and the resulting lookahead in slots.
    omnetpp::simtime_t ntnCellRoundTripDelay_ = SIMTIME_ZERO;
    long ntnGrantOffsetSlots_ = 0;

    // The slot the blocks booked in this slot are booked FOR. Must advance by at least
    // one every slot; see refreshNtnGrantTiming().
    int64_t ntnTargetSlot_ = 0;
    bool ntnTargetSlotValid_ = false;

    // UEs this gNodeB has actually heard from, and their latched round-trip delays.
    // Before a UE is in here only the cell-wide bound exists for it.
    std::set<MacNodeId> ntnHeardFrom_;
    std::map<MacNodeId, std::pair<omnetpp::simtime_t, omnetpp::simtime_t>> ntnUeRoundTripDelay_;

    void handleSelfMessage() override;

    // Stamps outgoing uplink grants with the time from which they become valid.
    void sendLowerPackets(omnetpp::cPacket *pkt) override;

    // Records the sender as heard from, which is what allows its own delay to be used
    // in place of the cell-wide bound.
    void macPduUnmake(omnetpp::cPacket *pkt) override;

    // Re-derives the lookahead and advances the target reception slot. Called at the
    // start of every slot, for the same reason ~NtnNrMacUe refreshes there.
    void refreshNtnGrantTiming();

    // The slot this gNodeB is currently in.
    int64_t ntnCurrentSlot() const;

    // Round-trip delay to use for a UE: its own once heard from, latched and refreshed
    // no more often than ntnUlSyncValidityDuration, and the cell bound before that.
    omnetpp::simtime_t ntnRoundTripDelayFor(MacNodeId ueId);

    // Walks the uplink HARQ receive buffers and emits their occupancy. Read-only:
    // it inspects process status and changes nothing.
    void emitNtnHarqRxOccupancy();
};

} //namespace

#endif
