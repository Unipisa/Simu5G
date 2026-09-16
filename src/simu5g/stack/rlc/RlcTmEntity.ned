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

package simu5g.stack.rlc;

//
// RLC TM entity of one data radio bearer: packages the transmitting and the
// receiving side (TS 36.322 / 38.322 model an entity per bearer). Created
// dynamically by BearerManagement, one per (peer, DRB) pair; a bearer with
// traffic in one direction only simply leaves the other side idle.
//
module RlcTmEntity
{
    parameters:
        @dynamic(true);
        @display("i=block/wheelbarrow");
    gates:
        input upperIn;    // SDUs from PDCP
        output upperOut;  // Reassembled SDUs to PDCP
        input lowerIn;    // PDUs from RlcMux
        output lowerOut;  // PDUs and new-data notifications to RlcMux/MAC
        input macIn;      // MAC SDU requests via RlcMux
    submodules:
        tx: <default("simu5g.stack.rlc.tm.RlcTmTxEntity")> like IRlcTxEntity {
            @display("p=50,50");
        }
        rx: <default("simu5g.stack.rlc.tm.RlcTmRxEntity")> like IRlcRxEntity {
            @display("p=150,50");
        }
    connections:
        upperIn --> { @display("m=n"); } --> tx.in;
        macIn --> { @display("m=s"); } --> tx.macIn;
        tx.out --> { @display("m=s"); } --> lowerOut;
        lowerIn --> { @display("m=s"); } --> rx.in;
        rx.out --> { @display("m=n"); } --> upperOut;
}
