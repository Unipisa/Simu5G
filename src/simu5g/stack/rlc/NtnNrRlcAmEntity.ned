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

package simu5g.stack.rlc;

//
// RLC AM entity of an NR data radio bearer carried over a transparent NTN path.
// It is ~NrRlcAmEntity with the AM timers re-dimensioned for satellite round-trip
// delay; the TX/RX entity types, and therefore all behaviour, are unchanged.
// Selected by BearerManagement through the nrRlcAmEntityModuleType parameter,
// which ~NtnNrNicUe and ~NtnNrNic redirect here.
//
// Every terrestrial default is shorter than one satellite round trip -- 80ms
// t_PollRetransmit and 80ms t_Reassembly against 541ms for GEO -- so a poll can
// never be answered before it expires. Left unchanged, RETX_COUNT reaches
// maxRtxThreshold within a few poll periods and RlcAmTxEntityBase::
// declareRadioLinkFailure() releases the bearer at both ends.
//
// Values are derived from 3GPP, not chosen by hand:
//
//   RTD (TR 38.821 clause 7.2.2.2, transparent architecture)
//       GEO 541.46ms, LEO 25.77ms
//
//   t_Reassembly (TR 38.821 clause 7.2.2.1)
//       t-Reassembly = RTD * nrof_HARQ_retrans + scheduling_offset
//       With maxHarqRtx = 3, i.e. 4 HARQ transmissions:
//       GEO 541.46*4 = 2166 -> 2200ms;  LEO 25.77*4 = 103 -> 150ms
//       2200ms is beyond the base TS 38.331 T-Reassembly range (max ms200) and
//       corresponds to the Rel-17 t-ReassemblyExt-r17 IE, which was introduced
//       for exactly this reason. The LEO value fits the base range (ms150).
//
//   t_PollRetransmit
//       Must exceed one RTD or the poll expires before its STATUS can arrive.
//       Smallest legal TS 38.331 enumerated value above the RTD:
//       GEO ms500 < 541.46 < ms800 -> 800ms;  LEO -> 80ms
//       TR 38.821 clause 7.2.2.1 states that no *range* change is needed for
//       this timer, and none is: only the configured value has to change.
//
//   t_StatusProhibit
//       Must stay below t_PollRetransmit - RTD, so a poll arriving while the
//       prohibit timer runs can still be answered before the poll expires:
//       GEO 800-541 = 259 -> 250ms;  LEO 80-26 = 54 -> 50ms
//
//   maxRtxThreshold is left at its inherited default of 4, which is already the
//   upper of the two cases TR 38.821 clause 7.2.2.2 considers ("1 or 4 RLC
//   retransmissions are considered here").
//
// The resulting retransmission budget, t_PollRetransmit * (maxRtxThreshold+1),
// is 4.0s for GEO and 400ms for LEO, against the 3.0s / 150ms that TR 38.821
// clause 7.2.2.2 tabulates. Ours is the more conservative because rounding
// 541.46ms up to the legal ms800 overshoots; the report's figures were inputs to
// a supportable-bit-rate calculation rather than timer targets.
//
// ntnOrbitProfile is a stopgap, not the intended long-term design. Timer values
// should follow the serving path, which matters as soon as a UE can attach to
// either orbit. Two notes for whoever gets there:
//   - a mixed constellation is blocked by the absence of NTN handover and
//     dynamic association, not by these timers;
//   - BearerManagement creates these entities per (peer, DRB) and re-creates
//     them on teardown, so re-established bearers pick up fresh values at
//     construction. That is how RRC reconfiguration delivers new timer values in
//     a real network, and it is preferable to mutating a live entity.
// When the geometry-derived round-trip delay service arrives, replace the three
// expressions below with it. They are deliberately the only place these values
// are produced.
//
module NtnNrRlcAmEntity extends NrRlcAmEntity
{
    parameters:
        string ntnOrbitProfile @enum("GEO","LEO") = default("GEO");   // GEO is the safe worst case

        tx.t_PollRetransmit = default(this.ntnOrbitProfile == "GEO" ? 800ms : 80ms);
        rx.t_Reassembly = default(this.ntnOrbitProfile == "GEO" ? 2200ms : 150ms);
        rx.t_StatusProhibit = default(this.ntnOrbitProfile == "GEO" ? 250ms : 50ms);
}
