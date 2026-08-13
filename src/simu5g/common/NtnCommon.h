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

#ifndef _NTNCOMMON_H_
#define _NTNCOMMON_H_

#include "simu5g/common/LteTypes.h"

namespace simu5g {

class Binder;

//
// Shared support for the NTN protocol paths, which all face the same questions: what is the
// round-trip delay of the cell a bearer runs over, was this timer left to be derived from it, and
// what is the nearest value a real gNodeB could actually signal.
//
// The geometry is only meaningful once the radios hold valid positions -- nothing does until
// inet::INITSTAGE_SINGLE_MOBILITY, which runs after every Simu5G stage -- so the round-trip delay
// must not be queried during the setup stages. The RLC entities are created per bearer at
// simulation time and are therefore safe by construction.
//

//
// Worst-case round-trip delay of the given cell: the longest round trip any UE it serves can have.
// Returns zero for a gNodeB with no NTN association, which is what leaves a terrestrial bearer
// untouched.
//
// The bound comes from the lowest elevation the cell is willing to use and the satellite's
// altitude, both published on the association by the cell itself. It depends on the altitude but
// not on where the satellite currently is, so for a circular orbit it is constant for the whole
// run; it is derived on the first query for a cell and published back onto the association, so
// every consumer reads one number derived once.
//
omnetpp::simtime_t ntnCellRoundTripDelay(Binder *binder, MacNodeId gnbId);

//
// The same value, for a module created per bearer.
//
// Resolved through the entity's own macModule parameter -- the enclosing entity compound already
// points it at this bearer leg's MAC -- so a UE reads its serving gNodeB and a gNodeB reads
// itself, and both ends of a bearer therefore derive their timers from the same delay. That
// symmetry is the point: RLC is a peer protocol, and a receiver whose reassembly timer is shorter
// than its transmitter's poll interval is worse than either value alone.
//
omnetpp::simtime_t ntnCellRoundTripDelay(omnetpp::cSimpleModule *entity);

//
// Instantaneous round-trip delay between the given gNodeB and one of its UEs, from the actual
// positions of the UE, the satellite and the gateway. Returns zero for a gNodeB with no NTN
// association.
//
// Unlike ntnCellRoundTripDelay() this tracks satellite motion, so over a LEO pass it varies
// continuously and is never cached. No timer consumes it yet: it exists for per-UE diagnostics --
// it reproduces the per-hop delays applied by NtnPropagationDelay by an independent route -- and
// for grant timing (k2/K_offset), which will need the per-UE value rather than the cell bound.
//
omnetpp::simtime_t ntnRoundTripDelay(Binder *binder, MacNodeId gnbId, MacNodeId ueId);

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
