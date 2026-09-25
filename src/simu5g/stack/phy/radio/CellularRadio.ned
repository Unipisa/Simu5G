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
// frame that arrives for the node's PHY leg enters through it; the PHY and its
// channel models still decide everything else.
//
// A frame arrives when its transmission starts. The radio holds it until the
// transmission ends and passes it up to the PHY then, at the frame's own
// scheduling priority; a frame that takes no air time (a beacon) is passed up
// on arrival.
//
module CellularRadio
{
    parameters:
        string radioMediumModule = default("radioMedium"); // The network's radio medium
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
