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
// A channel model that replaces propagation modelling with a configurable packet
// error rate. Path loss, shadowing and fading are not modelled at all: the
// attenuation is zero and the reported SINR/RSRP are a fixed large value, so the
// CQI reported to the scheduler is pinned at its maximum and the transport block
// size never varies with the channel.
//
// Whether a frame is received is decided by a coin flip. The error probability of
// the n-th transmission attempt of a frame is
//
//     per * harqReduction^(n-1)
//
// so with the default harqReduction below 1 each HARQ retransmission is more
// likely to succeed than the previous attempt, as it would be with soft combining.
// Setting harqReduction to 1 makes the attempts independent and identically
// distributed, in which case a frame that exhausts its HARQ retransmissions -- and
// is therefore lost as far as RLC is concerned -- has probability
// per^(maxHarqRtx+1). That makes the residual loss rate seen by RLC an exact,
// configurable quantity, which is what this model is mainly useful for: driving
// RLC AM/UM retransmission and radio link failure scenarios at a known loss rate,
// without the loss rate being an emergent property of geometry and MCS selection.
//
// The error rate is settable per direction, because ARQ protocols need their
// feedback path to be treated separately from their data path: an RLC AM
// experiment typically wants a lossy downlink and a clean uplink so that STATUS
// PDUs are guaranteed to arrive.
//
// This model is intended for protocol validation, not for performance studies.
//
simple IdealChannelModel extends ChannelModelBase like IChannelModel
{
    parameters:
        @class("IdealChannelModel");

        // Error probability of the first transmission attempt; the per-direction
        // parameters below default to it
        double per = default(0.1);

        // Error probability of the first transmission attempt, per direction. Volatile,
        // so a coverage loss can be scripted as a function of time, for example
        // perDl = simTime() < 10s ? 0 : 1
        volatile double perDl = default(this.per);
        volatile double perUl = default(this.per);
        volatile double perD2D = default(this.per);

        // Factor the error probability is multiplied by for each HARQ retransmission;
        // 1 makes the transmission attempts independent
        double harqReduction = default(0.3);
}
