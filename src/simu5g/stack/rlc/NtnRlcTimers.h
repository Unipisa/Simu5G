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

#ifndef _NTNRLCTIMERS_H_
#define _NTNRLCTIMERS_H_

#include <omnetpp.h>

namespace simu5g {

//
// Shared support for the NTN RLC entity profiles, which all face the same two questions: what is
// the round-trip delay of the cell this bearer runs over, and was this timer left to be derived
// from it.
//
// The entities are created per bearer at simulation time, so both answers are available where a
// module initialised during the setup stages would have found neither: no radio holds a valid
// position until inet::INITSTAGE_SINGLE_MOBILITY, which runs after every Simu5G stage.
//

//
// Worst-case round-trip delay of the cell serving the bearer this RLC entity belongs to, taken
// from the Binder. Returns zero if that cell has no NTN association, which is what leaves a
// terrestrial bearer untouched.
//
// Resolved through the entity's own macModule parameter -- the enclosing entity compound already
// points it at this bearer leg's MAC -- so a UE reads its serving gNodeB and a gNodeB reads
// itself, and both ends of a bearer therefore derive their timers from the same delay. That
// symmetry is the point: RLC is a peer protocol, and a receiver whose reassembly timer is shorter
// than its transmitter's poll interval is worse than either value alone.
//
omnetpp::simtime_t ntnCellRoundTripDelay(omnetpp::cSimpleModule *entity);

//
// Number of times a MAC PDU can be transmitted on this bearer's leg before HARQ gives up, i.e.
// maxHarqRtx + 1.
//
// This is the multiplier TR 38.821 clause 7.2.2.1 puts on the round-trip delay to size a
// reassembly timer: reassembly has to outlast the whole HARQ budget, because until HARQ has
// finished trying, the missing segment may still arrive. It is deliberately the HARQ count and
// not the RLC ARQ maxRtxThreshold -- RLC UM has no ARQ at all and needs the same timer.
//
int ntnHarqTransmissions(omnetpp::cSimpleModule *entity);

//
// True if a timer parameter still holds the "derive me from the round-trip delay" sentinel.
//
// A negative default is used rather than, say, zero because zero is a legal value for several of
// these timers, and because it makes the derivation visible in the NED file: a reader sees that
// the value is computed rather than configured, and an explicit assignment in an ini file is
// positive and therefore wins untouched.
//
inline bool ntnTimerIsDerived(omnetpp::simtime_t value) { return value < SIMTIME_ZERO; }

} // namespace simu5g

#endif
