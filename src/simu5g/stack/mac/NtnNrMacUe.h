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

#include "simu5g/stack/mac/NrMacUe.h"

namespace simu5g {

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

    void handleSelfMessage() override;
    void macHandleRac(omnetpp::cPacket *pkt) override;

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
