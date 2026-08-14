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
    // Transmit HARQ occupancy: scales with the round trip, unlike the gNodeB's
    // receive-side occupancy (~NtnNrMacGnb). Sampled before this slot's own PDU is
    // inserted, i.e. exactly when firstAvailable() would fail.
    static omnetpp::simsignal_t ntnHarqTxOccupancySignal_;
    static omnetpp::simsignal_t ntnHarqTxStallSignal_;

    static omnetpp::simsignal_t ntnGrantHoldTimeSignal_;
    static omnetpp::simsignal_t ntnPendingGrantsSignal_;

    // Grants received but not yet valid, per carrier, ordered by activation time. The
    // gNodeB issues one grant per slot and holds each for most of a round trip, so many
    // are outstanding; the inherited schedulingGrant_ keeps only the last.
    std::map<inet::GHz, PendingGrantsByActivation> pendingGrants_;

    void handleSelfMessage() override;
    void macHandleRac(omnetpp::cPacket *pkt) override;

    // Holds a grant until its activation time instead of using it on receipt.
    void macHandleGrant(omnetpp::cPacket *pkt) override;

    // Installs any held grant whose activation time has arrived, before the inherited
    // implementation looks for one.
    void promoteDueGrants();

    // Re-derives the inherited slot counts from the cell's current round-trip delay. Called at
    // the start of every TTI and on random-access response, i.e. before any of the points that
    // latch one of them, so a procedure always starts from a value valid at that moment.
    void refreshNtnCounters();

    void emitNtnHarqTxState();
};

} //namespace

#endif
