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
// The transmitter of a cellular radio: it turns each frame the radio sends
// into the transmission the medium keeps -- power, resource-block allocation,
// carrier, direction, frame type, start and duration -- taken from the frame's
// control information.
//
simple CellularTransmitter
{
    parameters:
        @class(CellularTransmitter);
        @display("i=block/tx");
}
