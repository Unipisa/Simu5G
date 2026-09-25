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
// The receiver of a cellular radio: noise figure and cable loss, the
// reception decision and its random draw, and the SINR statistics; the error
// model computes the packet error rate the decision is drawn against. So far
// it holds the noise figure and the cable loss.
//
module CellularReceiver
{
    parameters:
        double noiseFigure @unit(dB) = default(7dB);
        double cableLoss @unit(dB) = default(2dB);   // Loss between the antenna and the receiver
        @class(CellularReceiver);
        @display("i=block/rx");
    submodules:
        errorModel: BlerCurveErrorModel {
            @display("p=100,100");
        }
}
