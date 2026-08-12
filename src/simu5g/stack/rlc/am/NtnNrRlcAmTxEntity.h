//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _NTNNRRLCAMTXENTITY_H_
#define _NTNNRRLCAMTXENTITY_H_

#include "simu5g/stack/rlc/am/NrRlcAmTxEntity.h"

namespace simu5g {

//
// Transmitting side of an RLC AM entity on a bearer carried over a transparent NTN path.
//
// It differs from ~NrRlcAmTxEntity in exactly one value, t_PollRetransmit, which it derives from
// the round-trip delay the Binder computes for this cell instead of taking a configured constant.
// Everything else -- segmentation, polling, ARQ -- is inherited unchanged.
//
// It is also where the round-trip-delay coverage check lives. A poll that expires before its
// STATUS report can physically arrive is retransmitted regardless of whether the peer answered,
// so RETX_COUNT climbs on a link that is working perfectly and the bearer is released at both
// ends. That failure looks exactly like a channel problem, which is why it is an error rather
// than a warning; see the ntnCheckTimersCoverRoundTripDelay parameter for the opt-out.
//
class NtnNrRlcAmTxEntity : public NrRlcAmTxEntity
{
  protected:
    void initialize(int stage) override;
};

} // namespace simu5g

#endif
