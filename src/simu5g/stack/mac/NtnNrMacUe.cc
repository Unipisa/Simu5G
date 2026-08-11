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

#include "simu5g/stack/mac/NtnNrMacUe.h"

#include <cmath>

namespace simu5g {

using namespace omnetpp;

Define_Module(NtnNrMacUe);

namespace {

// Rounds a duration up to a whole number of slots. The inherited counters are
// slot counts, while the NTN parameters are expressed in time so that they stay
// correct under any numerology; rounding up keeps a timer from ever coming out
// shorter than the delay it is meant to cover.
unsigned int toSlots(double seconds, double slotDuration)
{
    // The epsilon keeps a duration that is already a whole number of slots from being
    // pushed to the next one by representation error: 0.542 + 0.040 does not divide
    // exactly by 0.001. At a 1ms slot it is worth a nanosecond, far below anything these
    // counters can express.
    return static_cast<unsigned int>(std::ceil(seconds / slotDuration - 1e-6));
}

} // namespace

void NtnNrMacUe::initialize(int stage)
{
    NrMacUe::initialize(stage);

    // The inherited RAC/BSR parameters are read at INITSTAGE_LOCAL, but ttiPeriod_ is only
    // known at INITSTAGE_SIMU5G_TTI_SETUP, several stages later. Converting any earlier
    // would divide by the default TTI rather than by the slot duration this UE actually
    // runs at. checkRAC() and macHandleRac() only ever read these members, so overwriting
    // them here is enough -- neither needs overriding.
    if (stage == INITSTAGE_SIMU5G_TTI_SETUP) {
        // 3GPP splits the wait into an offset that delays the start of the RAR window and
        // the window itself (TR 38.821 7.2.1.1.1.2). Simu5G has a single counter, so they
        // are summed here -- the only place the two are combined.
        double raResponseWindow = par("ntnRaResponseWindowOffset").doubleValue()
            + par("ntnRaResponseWindow").doubleValue();

        raRespWinStart_ = toSlots(raResponseWindow, ttiPeriod_);
        bsrRtxTimerStart_ = toSlots(par("ntnRetxBsrTimer").doubleValue(), ttiPeriod_);
        minRacBackoff_ = toSlots(par("ntnRacBackoffMin").doubleValue(), ttiPeriod_);
        maxRacBackoff_ = toSlots(par("ntnRacBackoffMax").doubleValue(), ttiPeriod_);

        // Reported at INFO because a counter silently left at its terrestrial value is
        // indistinguishable from a working one, and the resulting preamble storm looks like
        // a channel problem rather than a configuration one.
        EV_INFO << "NtnNrMacUe::initialize - UE " << nodeId_ << " orbit profile "
                << par("ntnOrbitProfile").stdstringValue() << ", slot " << ttiPeriod_ * 1000.0
                << "ms: raResponseWindow[" << raRespWinStart_ << " slots = "
                << par("ntnRaResponseWindowOffset").doubleValue() * 1000.0 << "ms offset + "
                << par("ntnRaResponseWindow").doubleValue() * 1000.0 << "ms window]"
                << ", retxBsrTimer[" << bsrRtxTimerStart_ << " slots]"
                << ", racBackoff[" << minRacBackoff_ << ".." << maxRacBackoff_ << " slots]" << endl;
    }
}

} //namespace
