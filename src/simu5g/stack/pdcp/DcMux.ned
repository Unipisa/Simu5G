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
// Dual Connectivity mux: routes split-bearer PDCP PDUs between the local PDCP
// entities and the ~DcX2Forwarder, which tunnels them to the peer eNB/gNB over X2.
//
simple DcMux
{
    parameters:
        @display("i=block/dispatch");
        string bearerManagementModule = default("^.rrc.bearerManagement");
    gates:
        input dcManagerIn @loose;
        output dcManagerOut @loose;
        input fromEntity[];       // From a secondary's PDCP relay (UL) or a master leg's remote side (DL out)
        output toRelayEntity[];   // To a secondary's PDCP relay (DL, PdcpRelayEntity)
        output toRxEntity[];      // To a master PDCP entity's remote (X2) leg (UL in)
}
