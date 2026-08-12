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

package simu5g.stack.rlc.am;

//
// ~NrRlcAmTxEntity for a bearer carried over a transparent NTN path: t_PollRetransmit is derived
// from the cell's satellite round-trip delay rather than configured. Bound by default inside the
// ~NtnNrRlcAmEntity compound.
//
simple NtnNrRlcAmTxEntity extends NrRlcAmTxEntity like simu5g.stack.rlc.IRlcAmTxEntity
{
    parameters:
        @class("NtnNrRlcAmTxEntity");

        string macModule;      // this bearer leg's MAC, for the serving cell id; set by the enclosing entity
        string binderModule = default("binder");   // holds the NTN geometry this timer is derived from

        // Negative: derive from the cell round-trip delay at bearer setup. An explicit assignment
        // is positive and wins, which is how a scenario overrides the derivation -- see
        // GeoSatAmTerrestrialTimers in simulations/nr/ntn_smoke.
        t_PollRetransmit = default(-1s);

        // A poll must outlast a round trip, or RETX_COUNT climbs on an error-free link and the
        // bearer is released. Checked at both ends of every AM bearer, because RLC is a peer
        // protocol. Turn off only to study the failure deliberately.
        bool ntnCheckTimersCoverRoundTripDelay = default(true);
}
