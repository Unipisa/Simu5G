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

#include "simu5g/stack/mac/scheduler/NtnSchedulerGnbUl.h"

#include "simu5g/stack/mac/NtnNrMacGnb.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(NtnSchedulerGnbUl);

simsignal_t NtnSchedulerGnbUl::ntnRtxGrantsSuppressedSignal_ = registerSignal("ntnRtxGrantsSuppressed");

void NtnSchedulerGnbUl::initialize(int stage)
{
    NrSchedulerGnbUl::initialize(stage);

    // mac_ is null before INITSTAGE_SIMU5G_AMC_SETUP, where the parent resolves it.
    if (stage == INITSTAGE_SIMU5G_AMC_SETUP) {
        if (dynamic_cast<NtnNrMacGnb *>(mac_.get()) == nullptr)
            throw cRuntimeError("NtnSchedulerGnbUl::initialize - %s is used by a MAC that is not an "
                    "NtnNrMacGnb. This scheduler suppresses a retransmission grant until the previous "
                    "one has had time to arrive, and it measures that in the reception slots only that "
                    "MAC books. Use NrSchedulerGnbUl on a terrestrial cell.", getFullPath().c_str());
    }
}

unsigned int NtnSchedulerGnbUl::schedulePerAcidRtx(MacNodeId nodeId, GHz carrierFrequency, Codeword cw,
        unsigned char acid, std::vector<BandLimit> *bandLim, Remote antenna, bool limitBl)
{
    auto mac = check_and_cast<NtnNrMacGnb *>(mac_.get());

    if (!mac->ntnGrantTimingReady())
        return NrSchedulerGnbUl::schedulePerAcidRtx(nodeId, carrierFrequency, cw, acid, bandLim, antenna, limitBl);

    int64_t target = mac->ntnTargetSlotFor(carrierFrequency);
    auto& granted = ntnGrantedRtx_[carrierFrequency][nodeId];
    auto key = std::make_pair(acid, cw);
    auto outstanding = granted.find(key);

    if (outstanding != granted.end()) {
        if (target < outstanding->second) {
            EV << NOW << " NtnSchedulerGnbUl::schedulePerAcidRtx - UE " << nodeId << " acid " << (int)acid
               << " cw " << cw << " already has a retransmission granted; suppressed until reception slot "
               << outstanding->second << endl;
            emit(ntnRtxGrantsSuppressedSignal_, 1);
            return 0;
        }

        granted.erase(outstanding); // heard by now; parent re-grants below if still corrupted
    }

    unsigned int rtxBytes = NrSchedulerGnbUl::schedulePerAcidRtx(nodeId, carrierFrequency, cw, acid,
            bandLim, antenna, limitBl);

    if (rtxBytes > 0)
        granted[key] = target + mac->ntnGrantOffsetSlotsFor(carrierFrequency); // known by one offset later

    return rtxBytes;
}

} //namespace
