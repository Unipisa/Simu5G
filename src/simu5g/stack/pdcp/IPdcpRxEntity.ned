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
// Module interface for the receiving side of a PDCP entity. Concrete types
// (LtePdcpRxEntity, NrPdcpRxEntity) implement it so the PdcpEntityBase compound
// can instantiate them via a typename parameter. The RX entity is leg-agnostic;
// on a multi-leg bearer the compound's joiner merges all legs into its single
// input, and its reorder window restores SN order (see ~PdcpEntityBase).
//
moduleinterface IPdcpRxEntity
{
    parameters:
        @display("i=block/layer");
    gates:
        input in;    // PDCP PDUs from the RLC leg(s) (merged by the compound)
        output out;  // Reassembled SDUs to the PdcpMux
}
