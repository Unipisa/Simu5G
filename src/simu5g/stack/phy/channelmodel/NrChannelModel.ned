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

package simu5g.stack.phy.channelmodel;


//
// This channel model implements path loss, LOS probability, and shadowing according to
// the following specifications:
//     3GPP TR 36.873, "Study on 3D channel model for LTE", v12.7.0, December 2017
//
simple NrChannelModel extends LteRealisticChannelModel
{
    parameters:
        @class("NrChannelModel");
}

