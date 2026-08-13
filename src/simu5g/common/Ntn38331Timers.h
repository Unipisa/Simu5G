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

#ifndef _NTN38331TIMERS_H_
#define _NTN38331TIMERS_H_

#include <omnetpp.h>

namespace simu5g {

//
// The 3GPP protocol timers whose values are enumerated rather than continuous.
//
// A real network cannot configure an arbitrary duration: each of these timers is carried in an
// ASN.1 ENUMERATED information element with a fixed list of legal values, so a derived value has
// to be rounded to one of them. Rounding is directional and the direction matters -- a timer that
// must cover a round trip is rounded up, one that must fit inside a budget is rounded down -- so
// the two are separate functions rather than a "nearest" one.
//
// This is what lets an NTN timer be derived from measured geometry and still be a value a real
// gNodeB could broadcast. The alternative, multiplying the round-trip delay by a tuned constant,
// produces durations no network could actually signal.
//
enum class Ntn38331Timer {
    PollRetransmit,     // TS 38.331 PollRetransmit, used for RLC AM t-PollRetransmit
    Reassembly,         // TS 38.331 T-Reassembly, extended by t-ReassemblyExt-r17 for NTN
    StatusProhibit,     // TS 38.331 T-StatusProhibit
    RetxBsrTimer,       // TS 38.331 BSR-Config retxBSR-Timer
    BackoffIndicator,   // TS 38.321 Table 7.2-1 backoff indicator values
};

//
// Smallest legal value of the given enumeration that is greater than or equal to `value`.
// Throws if `value` exceeds the largest legal value, because silently capping a timer that has to
// cover a round trip would reintroduce exactly the failure this rounding exists to prevent.
//
omnetpp::simtime_t ntn38331Ceil(omnetpp::simtime_t value, Ntn38331Timer which);

//
// Largest legal value of the given enumeration that is less than or equal to `value`.
//
// A `value` below the smallest legal value yields that smallest value, which is the one case where
// the result can exceed what was asked for. It is exact for t-StatusProhibit, whose enumeration
// starts at ms0, and reachable for the backoff indicator, whose lowest code point is 5ms -- but
// only for a round trip under 2.5ms, which is an order of magnitude below any satellite path.
// Rounding up there is the harmless direction anyway: it lengthens a backoff, it does not shorten
// a timer that has to cover a round trip.
//
omnetpp::simtime_t ntn38331Floor(omnetpp::simtime_t value, Ntn38331Timer which);

} // namespace simu5g

#endif
