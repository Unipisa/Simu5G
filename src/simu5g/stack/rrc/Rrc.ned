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
//     plane of other nodes talks to (base stations only for now)
//   - handoverController: manages handover decisions and procedures (UE only)
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
        bool hasHandoverController = default(false);
        bool hasNrHandoverController = default(false);

        *.interfaceTableModule = default(absPath(this.interfaceTableModule));
        *.routingTableModule = default(absPath(this.routingTableModule));

        // the node's control-plane entry point, for the submodules that report to it: a
        // base station's connectionControl, a UE's handoverController (its LTE leg's)
        bearerManagement.connectionControlModule = default(hasConnectionControl ? "^.connectionControl" : "^.handoverController");
        registration.connectionControlModule = default(hasConnectionControl ? "^.connectionControl" : "^.handoverController");

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
        handoverController: <default("HandoverController")> like IHandoverController if hasHandoverController {
            @display("p=550,80");
        }
        nrHandoverController: <default("HandoverController")> like IHandoverController if hasNrHandoverController {
            @display("p=700,80");
        }
}
