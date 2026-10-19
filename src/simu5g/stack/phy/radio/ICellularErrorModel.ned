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
// The error model of a cellular receiver (~CellularReceiver): the packet
// error rate of a reception, which the receiver decides the reception
// against. ~BlerCurveErrorModel computes it from the reception's SINR.
//
moduleinterface ICellularErrorModel
{
    parameters:
        @display("i=block/control");
}
