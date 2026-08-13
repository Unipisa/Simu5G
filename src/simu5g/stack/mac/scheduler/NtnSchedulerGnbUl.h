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

#ifndef _NTNSCHEDULER_GNB_UL_H_
#define _NTNSCHEDULER_GNB_UL_H_

#include <map>
#include <utility>

#include "simu5g/stack/mac/scheduler/NrSchedulerGnbUl.h"

namespace simu5g {

//
// NR gNodeB uplink scheduler for a cell on a transparent NTN path; see
// NtnSchedulerGnbUl.ned.
//
// It suppresses retransmission grants for a HARQ process that already has one in
// flight. Everything else is inherited unchanged.
//
class NtnSchedulerGnbUl : public NrSchedulerGnbUl
{
  protected:
    static omnetpp::simsignal_t ntnRtxGrantsSuppressedSignal_;

    // Per carrier, per UE, per (process, codeword): the earliest reception slot at
    // which a further retransmission grant may be issued, i.e. the first slot by which
    // the outstanding one will have been heard.
    std::map<inet::GHz, std::map<MacNodeId, std::map<std::pair<unsigned char, Codeword>, int64_t>>> ntnGrantedRtx_;

    void initialize(int stage) override;

    // Returns zero -- which the caller reads as "not scheduled" -- while this process
    // already has a retransmission grant that has not had time to arrive.
    unsigned int schedulePerAcidRtx(MacNodeId nodeId, GHz carrierFrequency, Codeword cw, unsigned char acid,
            std::vector<BandLimit> *bandLim = nullptr, Remote antenna = MACRO, bool limitBl = false) override;
};

} //namespace

#endif // _NTNSCHEDULER_GNB_UL_H_
