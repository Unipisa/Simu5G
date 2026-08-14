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

package simu5g.stack.mac;

//
// NR MAC for a gNodeB whose cell is served through a transparent NTN path. It is
// ~NrMacGnb plus uplink grant timing and an uplink reception measurement.
//
// ntnHarqRxOccupancy counts uplink transport blocks inside the receive evaluation
// window -- whether uplink data is arriving at all. It is NOT the stop-and-wait
// occupancy that caps throughput at N * TBS / RTT: a receive process is held only
// for harqFbEvaluationTimer (3GPP k1, a fixed decode budget independent of
// propagation delay). The occupancy that scales with the round trip is on the
// transmitter; see ~NtnNrMacUe.
//
module NtnNrMacGnb extends NrMacGnb
{
    parameters:
        @class("NtnNrMacGnb");

        @signal[ntnHarqRxOccupancy];
        @statistic[ntnHarqRxOccupancy](title="Uplink transport blocks in the HARQ evaluation window at the gNodeB"; unit=""; source="ntnHarqRxOccupancy"; record=mean,max);

        // Resource blocks are booked for the slot the transmission will be HEARD in
        // (not sent in), one offset per carrier -- this is what 3GPP calls K_offset
        // (TS 38.213), restoring DL-to-UL causality over a link with real propagation
        // delay. Booking by reception slot means the existing single-slot allocator is
        // already the future map: no ring buffer needed. Counted in the carrier's own
        // slots (numerology is a Binder-registered carrier property, so gNodeB and UE
        // agree on slot duration without signalling it).
        //
        // Negative derives from the cell's round-trip delay; an explicit assignment
        // wins untouched, in the granted carrier's slots.
        int ntnGrantOffsetSlots = default(-1);

        // TS 38.214 k2 analogue: UE preparation time after activation. Zero because
        // this UE model transmits within the same slot it receives a grant.
        double ntnGrantProcessingDelay @unit(s) = default(0s);

        // Cell-specific K_offset (SIB19) until the gNodeB has heard from a UE, then its
        // own delay -- the Rel-17 per-UE refinement by MAC CE after access. Read fresh
        // on every grant rather than cached: this is exact simulator geometry, not real
        // assistance data with an acquisition cost to amortise.
        bool ntnPerUeGrantTiming = default(true);

        // A receive process stays corrupted until its retransmission arrives -- a
        // round trip here, not the ~2 slots the inherited scheduler assumes. See
        // ~NtnSchedulerGnbUl.
        ulSchedulerType = default("NtnSchedulerGnbUl");

        @signal[ntnGrantActivationLead];
        @statistic[ntnGrantActivationLead](title="Slots between issuing a grant and its activation"; unit=""; source="ntnGrantActivationLead"; record=mean,min,max);
}
