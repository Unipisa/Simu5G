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

package simu5g.stack.phy.channelmodel;

//
// This is the base module for developing more realistic models of the radio channel.
// It primarily provides the initialization of parameters such as the reference to the
// Component Carrier (CC) and its associated parameters related to the channel
// model itself.
//
simple ChannelModelBase like IChannelModel
{
    parameters:
        @display("i=block/classifier2");
        @class("ChannelModelBase");

        string binderModule = default("binder");
        string cellInfoModule = default("^.^.cellInfo");  // Optional

        // Component carrier for this channel model
        string componentCarrierModule = default("carrierAggregation.componentCarrier[0]");
}

