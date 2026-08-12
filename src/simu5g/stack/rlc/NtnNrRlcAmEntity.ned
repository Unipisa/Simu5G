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
// Values are derived from 3GPP and from this cell's own geometry, not chosen by
// hand. The round-trip delay RTD comes from Binder::getNtnCellRoundTripDelay(),
// which bounds it by the lowest elevation the cell will use -- reproducing the
// 541.46ms (GEO) and 25.77ms (LEO-600) of TR 38.821 clause 7.2.2.2 for those
// orbits, and giving a real number for any other. Each timer is then rounded to a
// value TS 38.331 can actually signal, which is where its margin comes from:
//
//   t_Reassembly (TR 38.821 clause 7.2.2.1), in ~NtnNrRlcAmRxEntity
//       t-Reassembly = RTD * nrof_HARQ_transmissions, rounded up.
//       With maxHarqRtx = 3, GEO gives 541.46*4 = 2166 -> 2200ms. That is beyond
//       the base TS 38.331 T-Reassembly range (max ms200) and corresponds to the
//       Rel-17 t-ReassemblyExt-r17 IE, introduced for exactly this reason -- and
//       2200ms is that IE's ceiling, which the GEO case lands just inside.
//
//   t_PollRetransmit, in ~NtnNrRlcAmTxEntity
//       Must exceed one RTD or the poll expires before its STATUS can arrive, so
//       it is the smallest legal value above the RTD: for GEO the enumeration
//       jumps ms500 -> ms800, giving 800ms. TR 38.821 clause 7.2.2.1 states that
//       no *range* change is needed for this timer, and none is: only the
//       configured value has to change. ~NtnNrRlcAmTxEntity also asserts the
//       relation, because getting it wrong degrades silently.
//
//   t_StatusProhibit, in ~NtnNrRlcAmRxEntity
//       Must stay below t_PollRetransmit - RTD, so a poll arriving while the
//       prohibit timer runs can still be answered before the poll expires:
//       GEO 800-541 = 259 -> 250ms. On a short round trip this legitimately
//       rounds down to zero, which simply means STATUS reports are never
//       withheld.
//
//   maxRtxThreshold is left at its inherited default of 4, which is already the
//   upper of the two cases TR 38.821 clause 7.2.2.2 considers ("1 or 4 RLC
//   retransmissions are considered here").
//
// Derivation happens in C++ rather than in the expressions below because it needs
// the geometry, and no radio holds a valid position until
// inet::INITSTAGE_SINGLE_MOBILITY -- after every Simu5G stage. BearerManagement
// creates these entities per (peer, DRB) at simulation time and re-creates them on
// teardown, so the geometry is available when they initialise and a re-established
// bearer picks up fresh values at construction. That is how RRC reconfiguration
// delivers new timer values in a real network, and it is preferable to mutating a
// live entity.
//
module NtnNrRlcAmEntity extends NrRlcAmEntity
{
    parameters:
        // The NTN profiles of the NR entity types. They derive the three timers above from the
        // cell round-trip delay; everything else -- segmentation, polling, ARQ, reassembly -- is
        // the inherited NR behaviour.
        tx.typename = default("simu5g.stack.rlc.am.NtnNrRlcAmTxEntity");
        rx.typename = default("simu5g.stack.rlc.am.NtnNrRlcAmRxEntity");
}
