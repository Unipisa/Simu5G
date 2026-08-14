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

#include <map>
#include <set>

#include "simu5g/stack/mac/NrMacGnb.h"

namespace simu5g {

//
// NR MAC for a gNodeB serving its cell through a transparent NTN path; see
// NtnNrMacGnb.ned.
//
class NtnNrMacGnb : public NrMacGnb
{
  public:
    // Grant timing is per carrier (each carrier's numerology fixes its own slot
    // duration), not per this module's tick period. All three below are stateless
    // functions of time and carrier, so ~NtnSchedulerGnbUl and sendLowerPackets() agree
    // regardless of call order.
    bool ntnGrantTimingReady() const { return ntnCellRoundTripDelay_ > SIMTIME_ZERO; }

    double ntnSlotDurationFor(inet::GHz carrierFrequency);
    long ntnGrantOffsetSlotsFor(inet::GHz carrierFrequency);
    int64_t ntnTargetSlotFor(inet::GHz carrierFrequency);

  protected:
    // Uplink transport blocks inside the HARQ decode window (fixed budget, not
    // RTT-scale). ~NtnNrMacUe measures the transmit-side occupancy that does scale.
    static omnetpp::simsignal_t ntnHarqRxOccupancySignal_;
    static omnetpp::simsignal_t ntnGrantActivationLeadSignal_;

    // Constant for a circular orbit.
    omnetpp::simtime_t ntnCellRoundTripDelay_ = SIMTIME_ZERO;

    // Per carrier, memoised on first use.
    std::map<inet::GHz, double> ntnCarrierSlotDuration_;
    std::map<inet::GHz, long> ntnGrantOffsetSlots_;

    // UEs heard from; TS 38.331 cell-specific K_offset (SIB19) applies before, refined
    // per UE by MAC CE after access.
    std::set<MacNodeId> ntnHeardFrom_;

    void handleSelfMessage() override;
    void sendLowerPackets(omnetpp::cPacket *pkt) override;
    void macPduUnmake(omnetpp::cPacket *pkt) override;
    void refreshNtnGrantTiming();

    // A UE's own delay once heard from, else the cell bound.
    omnetpp::simtime_t ntnRoundTripDelayFor(MacNodeId ueId);

    void emitNtnHarqRxOccupancy();
};

} //namespace

#endif
