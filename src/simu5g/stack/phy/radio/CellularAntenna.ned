//
//                  Simu5G
//
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.stack.phy.radio;

//
// The antenna of a cellular radio: its gain, and whether it radiates in all
// directions or sectorially, towards txAngle. The sector's pattern is the
// propagation study's (see the channel model's pathLossType).
//
simple CellularAntenna
{
    parameters:
        double gain @unit(dBi) = default(0dBi);     // Antenna gain, transmitting and receiving
        string txDirection @enum(ANISOTROPIC,OMNI) = default("OMNI");
        double txAngle @unit(deg) = default(0deg);   // Boresight of a sectorial (ANISOTROPIC) antenna
        @class(CellularAntenna);
        @display("i=device/antennatower");
}
