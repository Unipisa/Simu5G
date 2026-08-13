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

#ifndef _NTNNRMACGNB_H_
#define _NTNNRMACGNB_H_

#include "simu5g/stack/mac/NrMacGnb.h"

namespace simu5g {

//
// NR MAC for a gNodeB serving its cell through a transparent NTN path; see
// NtnNrMacGnb.ned.
//
class NtnNrMacGnb : public NrMacGnb
{
  protected:
    // Uplink transport blocks currently inside the receive evaluation window at
    // this gNodeB. NOT a measure of stop-and-wait occupancy: a receive process is
    // held only from insertion until harqFbEvaluationTimer expires, which is a
    // fixed processing budget and does not grow with propagation delay. It says
    // whether uplink data is arriving at all, which over a satellite link is the
    // first thing to establish. The round-trip-scale occupancy is on the
    // transmitter, and is measured by ~NtnNrMacUe.
    static omnetpp::simsignal_t ntnHarqRxOccupancySignal_;

    void handleSelfMessage() override;

    // Walks the uplink HARQ receive buffers and emits their occupancy. Read-only:
    // it inspects process status and changes nothing.
    void emitNtnHarqRxOccupancy();
};

} //namespace

#endif
