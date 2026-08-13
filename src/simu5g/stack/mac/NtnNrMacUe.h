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

#ifndef _NTNNRMACUE_H_
#define _NTNNRMACUE_H_

#include <map>

#include "simu5g/stack/mac/NrMacUe.h"
#include "simu5g/stack/mac/packet/LteSchedulingGrant.h"

namespace simu5g {

// Grants held for one carrier, not yet valid, ordered by activation time.
typedef std::map<omnetpp::simtime_t, inet::IntrusivePtr<const LteSchedulingGrant>> PendingGrantsByActivation;

//
// NR MAC for a UE served through a transparent NTN path. It re-dimensions the
// random-access and buffer-status-report counters for satellite round-trip
// delay and changes nothing else; see NtnNrMacUe.ned for the derivation of the
// values from TR 38.821 and TS 38.331.
//
// The counters themselves are inherited slot counts. This class only converts
// the NED parameters, which are expressed in time, into slot counts against the
// slot duration this UE actually runs at, and overwrites the inherited reload
// values. Nothing in the random-access path is overridden: checkRAC() and
// macHandleRac() only ever read these members, so replacing them after
// initialisation is sufficient and avoids duplicating an unfactored 75-line
// function.
//
class NtnNrMacUe : public NrMacUe
{
  protected:
    // Transmit HARQ processes at this UE holding a transport block. With uplink feedback
    // enabled a process stays occupied until its feedback returns, i.e. for a whole round
    // trip, so this is the occupancy that decides whether the uplink is process-limited.
    // Sampled before this slot's own PDU is inserted; see emitNtnHarqTxState().
    static omnetpp::simsignal_t ntnHarqTxOccupancySignal_;

    // Emitted once per slot in which the UE holds a grant: 1 if every transmit process
    // was busy, 0 otherwise. Sampled only on granted slots, so its mean reads as the
    // fraction of usable slots lost to a full pool rather than of wall-clock time.
    static omnetpp::simsignal_t ntnHarqTxStallSignal_;

    // Time a grant spent held before becoming usable, and how many are outstanding.
    static omnetpp::simsignal_t ntnGrantHoldTimeSignal_;
    static omnetpp::simsignal_t ntnPendingGrantsSignal_;

    // Grants that arrived already past their activation time, and grants dropped
    // unused because a later one became due in the same slot. Both should stay at
    // zero; see promoteDueGrants() and macHandleGrant().
    static omnetpp::simsignal_t ntnLateGrantsSignal_;
    static omnetpp::simsignal_t ntnGrantsSkippedSignal_;

    // Grants received but not yet valid, per carrier, ordered by activation time.
    // A container is needed rather than the single slot schedulingGrant_ offers: the
    // gNodeB issues one grant per slot while the UE holds each for the better part of
    // a round trip, so many are outstanding at once and the inherited single slot
    // would keep only the last.
    std::map<inet::GHz, PendingGrantsByActivation> pendingGrants_;

    void handleSelfMessage() override;
    void macHandleRac(omnetpp::cPacket *pkt) override;

    // Holds a grant that is not yet valid, instead of letting the inherited
    // implementation make it usable on receipt.
    void macHandleGrant(omnetpp::cPacket *pkt) override;

    // Installs any held grant whose activation time has arrived. Called at the start
    // of every slot, before the inherited implementation looks for one.
    void promoteDueGrants();

    // Re-derives the inherited slot counts from the cell's current round-trip delay. Called at
    // the start of every TTI and on random-access response, i.e. before any of the points that
    // latch one of them, so a procedure always starts from a value valid at that moment.
    void refreshNtnCounters();

    // Walks the uplink HARQ transmit buffers and emits their occupancy, and whether a
    // grant went unused because the pool was full. Read-only.
    void emitNtnHarqTxState();
};

} //namespace

#endif
