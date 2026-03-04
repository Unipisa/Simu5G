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
//   - bearerManagement: sets up and tears down data radio bearers
//   - d2dModeSelection: periodically selects IM/DM mode for D2D flows (eNB/gNB only)
//   - handoverController: manages handover decisions and procedures (UE only)
//
module Rrc like IRrc
{
    parameters:
        @display("i=block/control");

        string interfaceTableModule;
        string routingTableModule;

        bool hasD2DModeController = default(false);
        bool hasD2DModeSelection = default(false);
        bool hasHandoverController = default(false);
        bool hasNrHandoverController = default(false);

        *.interfaceTableModule = default(absPath(this.interfaceTableModule));
        *.routingTableModule = default(absPath(this.routingTableModule));

    submodules:
        registration: Registration {
            @display("p=100,80");
        }
        bearerManagement: BearerManagement {
            @display("p=250,80");
        }
        d2dModeController: D2DModeController if hasD2DModeController {
            @display("p=400,80");
        }
        d2dModeSelection: <default("D2dModeSelectionBase")> like ID2DModeSelection if hasD2DModeSelection {
            @display("p=400,160");
        }
        handoverController: HandoverController if hasHandoverController {
            @display("p=550,80");
        }
        nrHandoverController: HandoverController if hasNrHandoverController {
            @display("p=700,80");
        }
}
