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
// ~NrMacGnb with an uplink reception measurement; no behaviour is changed.
//
// WHAT IT MEASURES. Over a satellite link a cell that delivers a fraction of its
// offered load gives no way to tell whether uplink data is failing to arrive or
// arriving and failing to be scheduled, and the delivery counters look the same
// either way. ntnHarqRxOccupancy answers the first half: it counts the uplink
// transport blocks currently inside the receive evaluation window.
//
// READ IT FOR WHAT IT IS. This is NOT the stop-and-wait occupancy that caps
// uplink throughput at N * TBS / RTT. A receive process is held only from
// insertion until harqFbEvaluationTimer expires -- a fixed decode budget, the
// 3GPP k1, which deliberately does not grow with propagation delay -- so this
// value stays small at any orbit and cannot be compared against harqProcesses to
// decide whether the pool binds. The occupancy that does scale with the round
// trip is on the transmitter, waiting for feedback, and ~NtnNrMacUe measures it
// there. Measured over the GEO smoke scenario the two differ by design: uplink
// receive occupancy peaks at 3 while the UE's transmit occupancy peaks at 13.
//
// It is sampled once per slot, immediately after the slot's own reception and
// scheduling have run -- that is, just after correct PDUs have been extracted and
// corrupted ones purged, so it reads the slot's minimum rather than its peak.
//
// The walk is done here rather than inside LteHarqBufferRx because the buffers
// are shared terrestrial code and an NTN measurement has no business changing
// them.
//
module NtnNrMacGnb extends NrMacGnb
{
    parameters:
        @class("NtnNrMacGnb");

        // Summed over every carrier and every served UE. With more than one UE the
        // sum alone does not say which UE is contributing; use the per-UE uplink
        // statistics for that.
        @signal[ntnHarqRxOccupancy];
        @statistic[ntnHarqRxOccupancy](title="Uplink transport blocks in the HARQ evaluation window at the gNodeB"; unit=""; source="ntnHarqRxOccupancy"; record=mean,max);

        // UPLINK GRANT TIMING.
        //
        // Resource blocks booked in one slot are used by the UE a round trip later, so
        // without an offset the gNodeB books a slot the granted transmission cannot
        // possibly occupy. The grant therefore carries the time from which it is valid,
        // and the UE holds it until then; see ~NtnNrMacUe.
        //
        // The blocks are booked for the slot the transmission will be HEARD in, not the
        // slot the UE transmits in. That choice is what keeps this cheap: with a
        // cell-wide offset each reception slot is booked by exactly one scheduling
        // round, so the existing single-slot allocator already is that slot's map and no
        // future-slot structure is needed. It is also the physically right frame, since
        // with no timing advance two transmissions collide according to when they
        // arrive, not when they were sent.
        //
        // Negative means "derive from this cell's round-trip delay"; an explicit
        // assignment is positive and wins untouched.
        int ntnGrantOffsetSlots = default(-1);

        // The k2 analogue: time the UE is allowed for preparing a transmission after the
        // grant becomes valid. Zero because Simu5G's UE consumes a grant and transmits
        // within the same slot, so nothing here needs the budget. A non-zero value
        // should come from the TS 38.214 k2 tables rather than being chosen.
        double ntnGrantProcessingDelay @unit(s) = default(0s);

        // Once the gNodeB has heard from a UE it may time that UE's grants from the UE's
        // own delay rather than the cell-wide worst case, which is the analogue of the
        // UE-specific K_offset that Rel-17 signals by MAC CE after access. Before that
        // only the broadcast bound exists, and using a per-UE value there would model a
        // network that knew a UE's range before it connected. Set false to use the cell
        // bound throughout.
        bool ntnPerUeGrantTiming = default(true);

        // How long a per-UE delay stays usable before it is read again, after the
        // specification's ntn-UlSyncValidityDuration. Latching rather than reading every
        // slot is what a real UE does; a geostationary cell never needs the refresh, and
        // a low-orbit one drifts slowly enough that one second is ample.
        double ntnUlSyncValidityDuration @unit(s) = default(1s);

        // How far a UE's delay may move between two reads before the run aborts. Grants
        // issued from the previous value are still in flight, and if the geometry has
        // moved further than this they would activate in the wrong slot.
        int ntnGrantTimingMarginSlots = default(2);

        @signal[ntnGrantActivationLead];
        @statistic[ntnGrantActivationLead](title="Slots between issuing a grant and its activation"; unit=""; source="ntnGrantActivationLead"; record=mean,min,max);
        @signal[ntnTargetSlotSkips];
        @statistic[ntnTargetSlotSkips](title="Slots where the booked reception slot had to be forced forward"; unit=""; source="ntnTargetSlotSkips"; record=sum);
}
