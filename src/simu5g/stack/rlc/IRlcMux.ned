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
// Module interface for the lower-layer RLC packet dispatcher of a cellular NIC
// (see ~RlcMux for the default implementation). Implementations dispatch packets
// coming from the MAC layer to the RLC RX entity serving their DRB (or, for MAC
// SDU requests, to its TX entity), and pass the output of the TX entities down to
// MAC. The per-bearer gates are created and wired at runtime by ~BearerManagement,
// which also registers the RX routing table; implementations must therefore
// subclass the RlcMux C++ class.
//
moduleinterface IRlcMux
{
    parameters:
        string bearerManagementModule;
        @display("i=block/dispatch");
    gates:
        input macIn;
        output macOut;
        output toRxEntity[];          // Dynamic: dispatches to RX entity in gates
        input fromTxEntity[];         // Dynamic: receives from TX entity out gates
        output macToTxEntity[];       // Dynamic: dispatches MAC SDU requests to TX entity macIn gates
}
