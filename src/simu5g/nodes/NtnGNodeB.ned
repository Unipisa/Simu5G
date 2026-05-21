//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.nodes;

import simu5g.nodes.gNodeB;
import simu5g.stack.NtnFronthaulNic;

//
// NTN-specific gNodeB. The base gNodeB remains terrestrial; this module owns
// the NTN gateway association and wired fronthaul toward the NTN gateway.
//
module NtnGNodeB extends gNodeB
{
    parameters:
        int ntnGatewayId = default(0);
        int satelliteId = default(0);
        cellularNic.typename = default("NtnNrNic");

    gates:
        inout ntn @loose; // wired NTN fronthaul toward the NTN gateway

    submodules:
        ntnIf: NtnFronthaulNic {
            parameters:
                @display("p=281,518");
        }

    connections:
        ntnIf.phys <--> ntn;
        ntnIf.upperLayerOut --> cellularNic.ntnIn;
        ntnIf.upperLayerIn <-- cellularNic.ntnOut;
}
