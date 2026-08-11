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

package simu5g.simulations.nr.ntn_smoke;

import inet.mobility.static.StationaryMobility;

// A geographic marker associated with a local simulation frame.
module LocalFrameMarker
{
    parameters:
        @networkNode;
        @display("i=misc/town;is=vs");
    submodules:
        mobility: StationaryMobility {
            @display("p=60,60;is=s");
        }
}
