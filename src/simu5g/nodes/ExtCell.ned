//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.nodes;

import inet.mobility.static.StationaryMobility;
import simu5g.stack.phy.radio.CellularPhantomRadio;

//
// The ExtCell module represents a fictitious cell that is used to model
// interference from neighboring cells, without actually instantiating them
// completely (i.e., without having actual UEs producing actual traffic). It can be
// configured with physical layer parameters (such as transmission power and
// direction) and the amount of resource occupation.
// Can be replaced by ~BackgroundCell modules, which provide more
// flexibility.
//
// It consists of a stationary mobility at the configured position, a phantom
// radio (~CellularPhantomRadio) with its antenna, and the allocator
// (~ExtCellAllocator), which decides the resource occupation.
//
module ExtCell
{
    parameters:
        @display("i=device/antennatower;bgb=300,300");

        string binderModule = default("binder");
        string radioMediumModule = default("radioMedium");

        // Coordinates
        double positionX @unit("m") = default(0m);
        double positionY @unit("m") = default(0m);
        double positionZ @unit("m") = default(0m);

        // Transmission power
        double txPower @unit(dBm) = default(46dBm);

        // Transmission direction
        string txDirection @enum(ANISOTROPIC,OMNI) = default("OMNI");

        // Transmission angle
        double txAngle @unit(deg) = default(0deg);

        // Operating carrier
        double carrierFrequency @unit(GHz) = default(2GHz);

        // Carrier bandwidth
        int numBands = default(6);

        // Allocation management
        string bandAllocationType @enum(FULL_ALLOC,RANDOM_ALLOC,CONTIGUOUS_ALLOC) = default("FULL_ALLOC");
        double bandUtilization = default(0.5);
        int startingOffset = default(0);

    submodules:
        mobility: StationaryMobility {
            initFromDisplayString = false;
            initialX = parent.positionX;
            initialY = parent.positionY;
            initialZ = parent.positionZ;
            @display("p=60,60;is=s");
        }
        radio: CellularPhantomRadio {
            radioMediumModule = parent.radioMediumModule;
            txPower = parent.txPower;
            antenna.txDirection = parent.txDirection;
            antenna.txAngle = parent.txAngle;
            antenna.gain = default(18dBi);
            noiseFigure = default(5dB);
            @display("p=60,150");
        }
        allocator: ExtCellAllocator {
            binderModule = parent.binderModule;
            radioMediumModule = parent.radioMediumModule;
            positionX = parent.positionX;
            positionY = parent.positionY;
            positionZ = parent.positionZ;
            txPower = parent.txPower;
            txDirection = parent.txDirection;
            txAngle = parent.txAngle;
            carrierFrequency = parent.carrierFrequency;
            numBands = parent.numBands;
            bandAllocationType = parent.bandAllocationType;
            bandUtilization = parent.bandUtilization;
            startingOffset = parent.startingOffset;
            @display("p=180,60");
        }
}
