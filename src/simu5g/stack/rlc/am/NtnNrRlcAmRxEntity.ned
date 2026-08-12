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
// ~NrRlcAmRxEntity for a bearer carried over a transparent NTN path: t_Reassembly and
// t_StatusProhibit are derived from the cell's satellite round-trip delay rather than configured.
// Bound by default inside the ~NtnNrRlcAmEntity compound.
//
simple NtnNrRlcAmRxEntity extends NrRlcAmRxEntity like simu5g.stack.rlc.IRlcAmRxEntity
{
    parameters:
        @class("NtnNrRlcAmRxEntity");

        string macModule;      // this bearer leg's MAC, for the serving cell id; set by the enclosing entity
        string binderModule = default("binder");   // holds the NTN geometry these timers are derived from

        // Negative: derive from the cell round-trip delay at bearer setup. An explicit assignment
        // is positive and wins, which is how a scenario overrides the derivation.
        t_Reassembly = default(-1s);
        t_StatusProhibit = default(-1s);
}
