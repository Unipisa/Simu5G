//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.stack.phy.errormodel;

//
// Base module for PHY error models. It provides common parameters and helper
// code used by concrete decoding-error algorithms.
//
simple ErrorModel like IErrorModel
{
    parameters:
        @display("i=block/classifier");
        @class("ErrorModel");

        string binderModule = default("binder");

        // HARQ reduction factor applied after the raw packet error probability
        // has been computed.
        double harqReduction = default(0.2);
}

