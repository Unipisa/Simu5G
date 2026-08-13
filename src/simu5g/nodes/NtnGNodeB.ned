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

        // Lowest elevation at which this cell is willing to use its satellite. It sets the longest
        // service and feeder links the cell can have, and therefore the worst-case round-trip delay
        // that ntnCellRoundTripDelay() reports and the NTN protocol timers are dimensioned
        // against. TR 38.821 clause 6.1.1 uses 10 degrees, which is what makes the reported delays
        // reproduce the 541.46ms (GEO) and 25.77ms (LEO-600) of its clause 7.2.
        double ntnMinElevation @unit(deg) = default(10deg);

        // Sanity guard on the resolved satellite position. Below this altitude the geometry is
        // rejected rather than used, which catches a round-trip-delay query issued before
        // inet::INITSTAGE_SINGLE_MOBILITY -- at that point every radio still reports (0,0,0),
        // which converts to a plausible point on the geoid and would otherwise pass unnoticed.
        double ntnMinSatelliteAltitude @unit(m) = default(100km);

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
