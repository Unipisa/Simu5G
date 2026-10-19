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
// An error model whose packet error rate is configured instead of computed
// from the reception: the SINR, the CQI and the allocation do not enter. The
// error rate of the n-th transmission attempt of a frame is
//     per * harqReduction^(n-1)
// with per the direction's (perDl, perUl, perD2D). With the default
// harqReduction below 1 each HARQ retransmission is more likely to succeed
// than the previous attempt, as it would be with soft combining. Setting
// harqReduction to 1 makes the attempts independent and identically
// distributed, so a frame that exhausts its HARQ retransmissions -- lost as far
// as RLC is concerned -- has probability per^(maxHarqRtx+1): an exact,
// configurable residual loss rate, for driving RLC AM/UM retransmission and
// radio link failure scenarios at a known loss rate rather than one that
// emerges from geometry and MCS selection.
//
// The rate is settable per direction, because ARQ protocols need their
// feedback path treated separately from their data path: an RLC AM experiment
// typically wants a lossy downlink and a clean uplink, so that STATUS PDUs are
// guaranteed to arrive. With ~IdealChannelModel as the radio medium's channel
// model it makes an ideal channel whose only loss is the configured one.
//
// A direction's rate is read at the end that receives it: a base station reads
// perUl, a UE perDl and perD2D, so a loss in a direction needs this error model
// at its receivers. The rate is the same on all carriers of a radio.
//
simple ConfiguredRateErrorModel like ICellularErrorModel
{
    parameters:
        // Error rate of the first transmission attempt; the per-direction rates default to it
        double per = default(0.1);
        // Error rate of the first transmission attempt, per direction. Volatile, so a coverage loss can be scripted as a function of time, for example perDl = simTime() < 10s ? 0 : 1
        volatile double perDl = default(this.per);
        volatile double perUl = default(this.per);
        volatile double perD2D = default(this.per);
        // Factor the error rate is multiplied by for each HARQ retransmission; 1 makes the transmission attempts independent
        double harqReduction = default(0.3);
        @class(ConfiguredRateErrorModel);
        @display("i=block/control");
}
