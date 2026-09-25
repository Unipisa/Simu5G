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
// The error model of a cellular receiver: the packet error rate of a
// reception from the BLER curves of its CQI at the SINR of each allocated
// resource block, with the HARQ combining gain. It holds nothing yet.
//
simple BlerCurveErrorModel
{
    parameters:
        @class(BlerCurveErrorModel);
        @display("i=block/control");
}
