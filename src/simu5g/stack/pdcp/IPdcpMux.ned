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
// Module interface for the upper-layer PDCP packet dispatcher of a cellular NIC
// (see ~PdcpMux for the default implementation). Implementations dispatch SDUs
// coming from Ip2Nic/SDAP to the PDCP TX entity serving their DRB, and pass the
// output of the RX entities back up. The per-bearer gates are created and wired
// at runtime by ~BearerManagement, which also registers the TX routing table;
// implementations must therefore subclass the PdcpMux C++ class.
//
moduleinterface IPdcpMux
{
    parameters:
        @display("i=block/dispatch");
    gates:
        input upperLayerIn;
        output upperLayerOut;
        output toTxEntity[];       // Dynamic: dispatches to TX entity in gates
        input fromRxEntity[];      // Dynamic: receives from RX entity out gates
}
