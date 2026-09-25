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
// medium, holding the antenna, the transmitter and the receiver. It is not
// used by the model yet; the PHY and its channel models still transmit and
// receive.
//
module CellularRadio
{
    parameters:
        @class(CellularRadio);
        @display("i=device/antennatower");
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
