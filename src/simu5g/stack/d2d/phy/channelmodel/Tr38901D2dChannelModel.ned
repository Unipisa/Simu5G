//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.stack.d2d.phy.channelmodel;

//
// ~D2dChannelModel preset with the propagation formulas of 3GPP TR 38.901: the
// radio medium's NR channel model in a network with D2D-capable NR nodes
// (CellularRadioMedium.nrChannelModelType).
//
simple Tr38901D2dChannelModel extends D2dChannelModel
{
    parameters:
        pathLossType = default("Tr38901");
}
