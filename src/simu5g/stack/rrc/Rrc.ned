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
//   - handoverController: manages handover decisions and procedures (UE only)
//
module Rrc like IRrc
{
    parameters:
        @display("i=block/control");

        string interfaceTableModule;
        string routingTableModule;

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
        handoverController: HandoverController if hasHandoverController {
            @display("p=400,80");
        }
        nrHandoverController: HandoverController if hasNrHandoverController {
            @display("p=550,80");
        }
}
