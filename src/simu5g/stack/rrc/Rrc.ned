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
//   - bearerManagement: sets up and tears down data radio bearers
//
module Rrc like IRrc
{
    parameters:
        @display("i=block/control");

        string interfaceTableModule;
        string routingTableModule;

        *.interfaceTableModule = default(absPath(this.interfaceTableModule));
        *.routingTableModule = default(absPath(this.routingTableModule));

    submodules:
        bearerManagement: BearerManagement {
            @display("p=100,80");
        }
}
