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

//
// ExtCell Module
//
simple ExtCell
{
    parameters:
        @display("i=device/antennatower;bgb=300,300");

        // coordinates
        double position_x @unit("m") = default(0m);
        double position_y @unit("m") = default(0m);
        double position_z @unit("m") = default(0m);

        // transmission power
        double txPower @unit(dBm) = default(46dBm);

        // transmission direction
        string txDirection @enum(ANISOTROPIC,OMNI) = default("OMNI");

        // transmission angle
        double txAngle @unit(deg) = default(0deg);

        // operating carrier
        double carrierFrequency @unit(GHz) = default(2GHz);

        // carrier bandwidth
        int numBands = default(6);

        // --- ALLOCATION MANAGEMENT --- //
        string bandAllocationType @enum(FULL_ALLOC,RANDOM_ALLOC,CONTIGUOUS_ALLOC) = default("FULL_ALLOC");
        double bandUtilization = default(0.5);
        int startingOffset = default(0);
        // ----------------------------- //
}
