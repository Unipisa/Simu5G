//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

package lte.simulations.NR.mec_rest.Test_onboardAppl;

import lte.nodes.mec.MECOrchestrator.MecOrchestrator;

network mecOrch
{
    parameters:

        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)
        @display("bgb=805.92,499.56");

    submodules:

        mecOrch: MecOrchestrator {
            @display("p=690,104.88;is=s");
        }



    connections allowunconnected:


}
