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
}
