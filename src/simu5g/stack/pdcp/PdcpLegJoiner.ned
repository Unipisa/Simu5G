//
//                  Simu5G
//
// Authors: Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.stack.pdcp;

//
// RX-side leg merger of a multi-leg PDCP entity (see ~PdcpEntityBase): relays the
// PDU streams of all legs into the single RX entity, whose one reorder window
// restores SN order across legs (the join needs no logic of its own -- the SN
// space is common to all legs by construction).
//
simple PdcpLegJoiner like IPdcpLegJoiner
{
    parameters:
        @display("i=block/join");
    gates:
        input in[];                // Per-leg input (sized by the enclosing PdcpEntityBase's connections)
        output out;                // Merged stream to the RX entity
}
