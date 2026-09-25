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
// The antenna of a cellular radio: its gain and, for a sectorial antenna, the
// direction and pattern. It holds nothing yet.
//
simple CellularAntenna
{
    parameters:
        @class(CellularAntenna);
        @display("i=device/antennatower");
}
