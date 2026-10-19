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
// A channel model with no propagation at all: path loss, shadowing, fading and
// interference are not modelled, the attenuation is zero and the reported
// SINR/RSRP are a fixed large value, so the CQI reported to the scheduler is
// pinned at its maximum and the transport block size never varies with the
// channel.
//
// Whether a frame is received is decided, as for any channel model, by the
// receiver against its error model. The receivers' default error model
// (~BlerCurveErrorModel) loses nothing at that SINR; with a
// ~ConfiguredRateErrorModel at the receivers
// (receiver.errorModel.typename = "ConfiguredRateErrorModel") the only loss is
// a configured, per-direction error rate -- which is what this model is
// mainly used for: driving RLC AM/UM retransmission and radio link failure
// scenarios at a known loss rate.
//
// This model is intended for protocol validation, not for performance studies.
//
simple IdealChannelModel extends ChannelModelBase like IChannelModel
{
    parameters:
        @class("IdealChannelModel");
}
