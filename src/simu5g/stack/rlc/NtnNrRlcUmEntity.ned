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
// RLC UM entity of an NR data radio bearer carried over a transparent NTN path.
// It is ~NrRlcUmEntity with t_Reassembly re-dimensioned for satellite round-trip
// delay; the TX/RX entity types, and therefore all behaviour, are unchanged.
// Selected by BearerManagement through the nrRlcUmEntityModuleType parameter,
// which ~NtnNrNicUe and ~NtnNrNic redirect here.
//
// UM has no ARQ, so an expiry here discards an incomplete SDU rather than failing
// the bearer as the AM path can (see ~NtnNrRlcAmEntity). It nonetheless matters
// more in practice: Ip2Nic defaults every traffic class to UM, so this is the
// only RLC timer the default NTN scenarios actually run.
//
// t_Reassembly is derived exactly as in ~NtnNrRlcAmEntity -- TS 38.322 uses the
// one timer for both modes, and TR 38.821 clause 7.2.2.1 says so explicitly
// ("used in RLC AM as well as in RLC UM"):
//
//   t-Reassembly = RTD * nrof_HARQ_retrans + scheduling_offset
//   GEO 541.46*4 = 2166 -> 2200ms;  LEO 25.77*4 = 103 -> 150ms
//
// See ~NtnNrRlcAmEntity for the full derivation, the spec citations, and the note
// on why ntnOrbitProfile is a stopgap for mixed constellations.
//
module NtnNrRlcUmEntity extends NrRlcUmEntity
{
    parameters:
        string ntnOrbitProfile @enum("GEO","LEO") = default("GEO");   // GEO is the safe worst case

        rx.t_Reassembly = default(this.ntnOrbitProfile == "GEO" ? 2200ms : 150ms);
}
