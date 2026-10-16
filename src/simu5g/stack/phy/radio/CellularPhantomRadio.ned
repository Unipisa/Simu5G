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
// The radio of a transmitter that is not a node with a PHY: an external cell
// (~ExtCell), a background cell (~BackgroundCell) or a background UE
// (~BackgroundUe). Like INET's NoiseSource it has an antenna and a
// transmitter but no receiver: it never receives frames, and the radio medium
// sends it none. Its owner adds it to the medium under the node id the
// owner's transmissions name, so that links to and from it are evaluated like
// any other radio's. A what-if at it (the CQI of a background UE) takes the
// noise figure and the cable loss given here. Its position and speed are those
// of its node's mobility, or the origin and 0 m/s without one.
//
module CellularPhantomRadio
{
    parameters:
        string radioMediumModule = default("radioMedium"); // The network's radio medium
        string mobilityModule = default("^.mobility"); // The mobility of the node; may be absent
        bool isNr = default(false); // Whether its links are evaluated with the medium's NR channel models
        double txPower @unit(dBm); // Transmit power
        double noiseFigure @unit(dB) = default(7dB); // Noise figure of a what-if at this radio
        double cableLoss @unit(dB) = default(2dB); // Loss between the antenna and a what-if's receiver
        @class(CellularPhantomRadio);
        @display("i=device/antennatower");
    submodules:
        antenna: CellularAntenna {
            @display("p=100,100");
        }
        transmitter: CellularTransmitter {
            @display("p=100,200");
        }
}
