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

package simu5g.stack.rrc;

//
// RRC (Radio Resource Control) layer for LTE/NR networks.
//
// Contains functional submodules corresponding to RRC procedures:
//   - registration: registers the node with the Binder, sets up the interface
//   - drbTable: the configuration of the node's data radio bearers
//   - bearerManagement: sets up and tears down data radio bearers
//   - connectionControl: the node's control-plane entry point, which the control
//     plane of other nodes and the node's own data path talk to: ~ConnectionControlEnb
//     at a base station, ~ConnectionControlUe at a UE (the LTE leg's; its handover
//     decisions are the serving base station's, its execution the UE's)
//   - nrConnectionControl: the NR leg's ~ConnectionControlUe at a dual-stack UE
//
// The D2D-specific submodules (d2dModeController, d2dModeSelection) are added by
// the ~RrcD2D subtype.
//
module Rrc like IRrc
{
    parameters:
        @display("i=block/control");

        string interfaceTableModule;
        string routingTableModule;

        bool hasConnectionControl = default(false);
        bool hasNrConnectionControl = default(false);

        *.interfaceTableModule = default(absPath(this.interfaceTableModule));
        *.routingTableModule = default(absPath(this.routingTableModule));

    submodules:
        registration: <default("Registration")> like IRegistration {
            @display("p=100,80");
        }
        drbTable: <default("DrbTable")> like IDrbTable {
            @display("p=250,80");
        }
        bearerManagement: <default("BearerManagement")> like IBearerManagement {
            @display("p=400,80");
        }
        connectionControl: <default("ConnectionControlEnb")> like IConnectionControl if hasConnectionControl {
            @display("p=550,80");
        }
        nrConnectionControl: <default("ConnectionControlUe")> like IConnectionControl if hasNrConnectionControl {
            @display("p=700,80");
        }
}
