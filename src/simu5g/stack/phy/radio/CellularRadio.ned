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
// The radio of a cellular PHY leg: the node-side counterpart of the radio
// medium, holding the antenna, the transmitter and the receiver. Every frame
// the PHY transmits passes through it (the PHY says where it goes), and every
// frame that arrives for the node's PHY leg enters through it. The radio's
// receiver decides the reception, against its error model, on the SINR the
// radio medium's channel model computes.
//
// A frame arrives when its transmission starts. The radio holds it until the
// transmission ends and passes it up to the PHY then, at the frame's own
// scheduling priority; a frame that takes no air time (a beacon) is passed up
// at an event of its own, at the instant it arrives.
//
module CellularRadio
{
    parameters:
        string radioMediumModule = default("radioMedium"); // The network's radio medium
        bool insideBuilding = default(false); // Whether the radio is inside a building, which counts on a UE's radio: a link's building penetration loss is its UE end's. Its distance from the wall (uniformly in [0, 25] m) and the random term of its penetration loss (standard normal, scaled by the study's sigma_P) are drawn once
        @class(CellularRadio);
        @display("i=device/antennatower");
    gates:
        input radioIn @directIn;    // frames from other radios
        input upperLayerIn;         // frames to transmit, from the PHY
        output upperLayerOut;       // received frames, to the PHY
    submodules:
        antenna: CellularAntenna {
            @display("p=100,100");
        }
        transmitter: CellularTransmitter {
            @display("p=100,200");
        }
        receiver: CellularReceiver {
            @display("p=100,300");
        }
}
