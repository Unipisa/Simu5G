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
// The ExtCell module represents a fictitious cell that is used to model
// interference from neighboring cells, without actually instantiating them
// completely (i.e., without having actual UEs producing actual traffic). It can be
// configured with physical layer parameters (such as transmission power and
// direction) and the amount of resource occupation.
// This module can be replaced by ~BackgroundCell modules, which provide more
// flexibility.
//
simple ExtCell
{
    parameters:
        @display("i=device/antennatower;bgb=300,300");

        string binderModule = default("binder");

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
