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
// Base type of the RLC UM entity of one data radio bearer: packages the
// transmitting and the receiving side (TS 36.322 / 38.322 model an entity per
// bearer). Created dynamically by BearerManagement, one per (peer, DRB) pair;
// a bearer with traffic in one direction only simply leaves the other side idle.
//
// This base type binds no TX/RX entity types; the concrete subclasses do
// (~LteRlcUmEntity, ~NrRlcUmEntity), so it is not instantiable on its own.
//
module RlcUmEntityBase
{
    parameters:
        @dynamic(true);
        @display("i=block/wheelbarrow");
        string macModule = default("^.mac");        // This bearer leg's MAC; set per leg by BearerManagement
        string rlcMuxModule = default("^.rlcMux");   // This bearer leg's RLC mux
        *.macModule = default(absPath(this.macModule));
        *.rlcMuxModule = default(absPath(this.rlcMuxModule));
    gates:
        input upperIn;    // SDUs from PDCP
        output upperOut;  // Reassembled SDUs to PDCP
        input lowerIn;    // PDUs from RlcMux
        output lowerOut;  // PDUs and new-data notifications to RlcMux/MAC
        input macIn;      // MAC SDU requests via RlcMux
    submodules:
        tx: <> like IRlcTxEntity {
            @display("p=50,50");
        }
        rx: <> like IRlcRxEntity {
            @display("p=150,50");
        }
    connections:
        upperIn --> { @display("m=n"); } --> tx.in;
        macIn --> { @display("m=s"); } --> tx.macIn;
        tx.out --> { @display("m=s"); } --> lowerOut;
        lowerIn --> { @display("m=s"); } --> rx.in;
        rx.out --> { @display("m=n"); } --> upperOut;
}
