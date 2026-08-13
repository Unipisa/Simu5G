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

    // INITSTAGE_SIMU5G_AMC_SETUP is where the parent resolves mac_; it is null before that.
    if (stage == INITSTAGE_SIMU5G_AMC_SETUP) {
        // The suppression window is measured in the reception slots that NtnNrMacGnb
        // books, so this scheduler is only meaningful under that MAC.
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

    if (!mac->ntnTargetSlotValid())
        return NrSchedulerGnbUl::schedulePerAcidRtx(nodeId, carrierFrequency, cw, acid, bandLim, antenna, limitBl);

    int64_t target = mac->ntnTargetSlot();
    auto& granted = ntnGrantedRtx_[carrierFrequency][nodeId];
    auto key = std::make_pair(acid, cw);
    auto outstanding = granted.find(key);

    if (outstanding != granted.end()) {
        if (target < outstanding->second) {
            // A receive process stays corrupted until its retransmission physically
            // arrives, which over a satellite link is a whole round trip rather than the
            // two slots this loop was written for. Without this the gNodeB would grant
            // the same process again in every slot of that round trip -- hundreds of
            // grants, each booking resource blocks, for one transport block.
            EV << NOW << " NtnSchedulerGnbUl::schedulePerAcidRtx - UE " << nodeId << " acid " << (int)acid
               << " cw " << cw << " already has a retransmission granted; suppressed until reception slot "
               << outstanding->second << endl;
            emit(ntnRtxGrantsSuppressedSignal_, 1);
            return 0;
        }

        // The outstanding retransmission has been heard by now. If the process is still
        // corrupted the parent will grant again, and this entry is replaced below.
        granted.erase(outstanding);
    }

    unsigned int rtxBytes = NrSchedulerGnbUl::schedulePerAcidRtx(nodeId, carrierFrequency, cw, acid,
            bandLim, antenna, limitBl);

    if (rtxBytes > 0) {
        // The retransmission arrives in the slot being booked now. The gNodeB only knows
        // whether it succeeded once it has reached that slot, which is a further offset
        // ahead of the slot it is booking today.
        granted[key] = target + mac->ntnGrantOffsetSlots();
    }

    return rtxBytes;
}

} //namespace
