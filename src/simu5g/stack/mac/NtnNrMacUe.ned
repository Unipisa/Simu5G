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
// NR MAC for a User Equipment served through a transparent NTN path. It is
// ~NrMacUe with the random-access and buffer-status-report counters
// re-dimensioned for satellite round-trip delay; no behaviour is changed.
//
// The inherited defaults are all far shorter than one round trip -- a 20-slot
// RA response window and a 320-slot BSR retransmission timer against 541ms for
// GEO -- so the UE re-fires a preamble roughly every 20ms while the response
// needs 541ms to arrive, and re-triggers BSRs long before any grant can return.
//
// TWO CAVEATS ABOUT WHAT THESE PARAMETERS MEAN HERE.
//
// First, Simu5G models no Scheduling Request at all: there is no PUCCH, no SR
// resource and no sr-ProhibitTimer, and a backlogged UE with no grant fires a
// RACH preamble instead. The RA response window therefore carries three 3GPP
// roles at once -- the RAR window start offset, the RAR window itself, and
// sr-ProhibitTimer -- so its total exceeds any legal TS 38.331
// ra-ResponseWindow value (that enumeration caps at sl80). That is a property
// of this model, not a spec-legal configuration, and it is why the offset and
// the window are declared separately below.
//
// Second, splitting them changes no behaviour: Simu5G has no RAR monitoring,
// and macHandleRac() accepts a response whenever it arrives with no window
// check. The split exists for traceability and for the migration described at
// the end of this comment.
//
// Values are derived from 3GPP, not chosen by hand:
//
//   RTD (TR 38.821 clause 7.2.2.2, transparent architecture)
//       GEO 541.46ms, LEO 25.77ms
//
//   ntnRaResponseWindowOffset (TR 38.821 clause 7.2.1.1.1.2)
//       "Introduce an offset for the start of the ra-ResponseWindow for NTN."
//       The offset equals the round-trip delay: GEO 542ms, LEO 26ms.
//
//   ntnRaResponseWindow (same clause)
//       "the RAR monitoring duration shall cover at least 2 * maximum
//       differential delay", which the report gives as 20.6ms for GEO and
//       6.36ms for LEO. Rounded up to the next legal TS 38.331
//       ra-ResponseWindow value: sl40 and sl8. Note this scales with the
//       spread across the cell, i.e. with beam size, NOT with the round-trip
//       delay.
//
//   ntnRetxBsrTimer
//       Smallest TS 38.331 retxBSR-Timer value above the RTD, but never below
//       the inherited default: GEO sf640; LEO keeps sf320, which already
//       clears the 25.77ms LEO round trip twelvefold.
//
//   ntnRacBackoffMax
//       Largest TS 38.321 backoff-indicator value not exceeding twice the RTD:
//       GEO 960ms, LEO 40ms. Rel-17 scales the backoff for NTN through
//       scalingFactorBI; this is the equivalent by configuration. Inert in a
//       single-UE scenario, since a collision needs two UEs sharing a preamble
//       in one gNodeB slot.
//
//   maxRacAttempts is left at its inherited 10, the legal preambleTransMax
//   value n10. Note it does not actually bound anything: the exhaustion branch
//   in LteMacUe::macHandleRac() resets the counter and zeroes the backoff, so
//   the UE retries immediately. That is a pre-existing gap, marked in the
//   source with "TODO flush all buffers here".
//
// The values are expressed in TIME and converted to slot counts against the
// actual slot duration in NtnNrMacUe::initialize(). The inherited parameters
// are slot counts, so a NED-only version would have to assume a slot duration;
// at numerology 0 slots and milliseconds coincide, which is exactly why such a
// version would look right in the smoke scenarios and silently halve every
// timeout at numerology 1.
//
// ntnOrbitProfile is a stopgap, as it is for the RLC timers -- see
// ~NtnNrRlcAmEntity. When a geometry-derived round-trip delay is available,
// ntnRaResponseWindowOffset is the one value here that becomes per-UE and
// time-varying, because it tracks that UE's own round trip;
// ntnRaResponseWindow tracks differential delay across the cell and stays a
// cell-level constant, as does ntnRetxBsrTimer. What blocks mixed LEO/GEO
// scenarios is the absence of handover and dynamic association, not these
// counters.
//
module NtnNrMacUe extends NrMacUe
{
    parameters:
        @class("NtnNrMacUe");

        // Both directions allocate through firstAvailable(), so HARQ is N-process
        // stop-and-wait and throughput is capped at N * TBS / RTT. Keeping the pipe full
        // needs one process per slot of round trip: about 26 for LEO-600 and 42 for
        // LEO-1200, but 541 for GEO. 32 is the Rel-17 ceiling
        // (nrofHARQ-ProcessesForPDSCH-v1700), so it covers LEO and cannot cover GEO --
        // which is why 3GPP also specifies feedback disabling, see harqFeedbackEnabled
        // below.
        //
        // This must match the value on the peer: LteHarqBufferRx is sized from the
        // receiver's own harqProcesses while the transmitter picks an ACID from its own,
        // so a mismatch indexes out of range. ~NtnNrNic sets the same value for the gNodeB.
        //
        // 32 exceeds the 4-bit HARQ process ID field of a real DCI; TR 38.821 clause 6.4.2
        // lists several ways round that (slot-number based IDs, virtual IDs, reuse within
        // the round-trip delay) without converging on one. Simu5G does not model DCI, so
        // the choice is free here -- but it is a real constraint in a deployment.
        harqProcesses = default(32);

        // Downlink feedback off, uplink feedback on. TR 38.821 clause 7.2.1.4 treats the
        // two as separate decisions, and measurement says they really are: over GEO,
        // disabling downlink feedback raises downlink delivery by about 37% (the pool stops
        // being stop-and-wait), while additionally disabling uplink feedback costs about
        // 80% of uplink delivery, because an uplink MAC PDU also carries the buffer status
        // report and losing one with no retransmission stalls the UE for a whole
        // retxBsrTimer before the gNodeB learns it has data.
        //
        // With uplink feedback still enabled the uplink remains stop-and-wait, which is why
        // harqProcesses above stays at 32 -- TR 38.821 clause 6.4.2 "Option 2: Greater than
        // 16 HARQ process IDs with UL HARQ feedback enabled". Downlink reliability now
        // rests on RLC ARQ, so an NTN bearer should use RLC AM; see the warning emitted
        // when it does not.
        harqFeedbackEnabledDl = default(false);
        harqFeedbackEnabledUl = default(true);

        string ntnOrbitProfile @enum("GEO","LEO") = default("GEO");   // GEO is the safe worst case

        double ntnRaResponseWindowOffset @unit(s) = default(this.ntnOrbitProfile == "GEO" ? 542ms : 26ms);
        double ntnRaResponseWindow @unit(s) = default(this.ntnOrbitProfile == "GEO" ? 40ms : 8ms);
        double ntnRetxBsrTimer @unit(s) = default(this.ntnOrbitProfile == "GEO" ? 640ms : 320ms);
        double ntnRacBackoffMin @unit(s) = default(0s);
        double ntnRacBackoffMax @unit(s) = default(this.ntnOrbitProfile == "GEO" ? 960ms : 40ms);
}
