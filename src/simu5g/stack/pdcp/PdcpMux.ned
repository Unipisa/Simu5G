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
// Upper-layer PDCP packet dispatcher.
//
// Receives SDUs from the module above -- ~Ip2Nic, or ~NrSdap where SDAP is present --
// looks up the toTxEntity gate of the PDCP TX entity serving that DRB, and dispatches
// the packet to it. Purely a dispatcher: TX entities are established by Ip2Nic/SDAP,
// and wired and registered in this module's routing table by ~BearerManagement.
// Collects processed SDUs from the RX entities and sends them back up.
//
simple PdcpMux like IPdcpMux
{
    parameters:
        @class("PdcpMux");
        @display("i=block/dispatch");
    gates:
        input upperLayerIn;
        output upperLayerOut;
        output toTxEntity[];       // Dynamic: dispatches to TX entity in gates
        input fromRxEntity[];      // Dynamic: receives from RX entity out gates
}
