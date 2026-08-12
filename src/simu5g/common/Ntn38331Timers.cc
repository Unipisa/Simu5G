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

#include "simu5g/common/Ntn38331Timers.h"

#include <algorithm>
#include <vector>

namespace simu5g {

using namespace omnetpp;

namespace {

struct TimerEnumeration {
    const char *name;
    std::vector<int> valuesMs;   // ascending, milliseconds
};

// The specifications spell these enumerations out in code-point order, which is not always
// ascending: the Rel-16 additions to t-PollRetransmit and t-StatusProhibit are values *below* the
// base enumeration's floor, appended at the end of the ASN.1 list. Both lookups below assume
// ascending order, so each table is sorted once on construction rather than being transcribed in
// an order that happens to work.
TimerEnumeration sorted(TimerEnumeration e)
{
    std::sort(e.valuesMs.begin(), e.valuesMs.end());
    return e;
}

// Appends the arithmetic run first..last step `step`, which is how the specifications write the
// dense low end of these enumerations (ms0, ms5, ms10, ...). Spelling out fifty literals would
// hide the two or three values that actually matter.
void appendRun(std::vector<int>& values, int first, int last, int step)
{
    for (int value = first; value <= last; value += step)
        values.push_back(value);
}

const TimerEnumeration& enumerationFor(Ntn38331Timer which)
{
    // Function-local statics: built on first use, so there is no static initialisation order
    // dependency on anything else in the library.
    static const TimerEnumeration pollRetransmit = [] {
        // TS 38.331 T-PollRetransmit. The ms1..ms4 values are the Rel-16 additions carried in the
        // separate ms{1,2,3,4}-v1610 code points; they sit below the base enumeration's ms5 floor
        // rather than extending it upwards, and are included so the list is the full set a network
        // could signal.
        TimerEnumeration e{"t-PollRetransmit", {1, 2, 3, 4}};
        appendRun(e.valuesMs, 5, 250, 5);
        e.valuesMs.insert(e.valuesMs.end(), {300, 350, 400, 450, 500, 800, 1000, 2000, 4000});
        return sorted(e);
    }();

    static const TimerEnumeration reassembly = [] {
        // TS 38.331 T-Reassembly, plus the Rel-17 T-ReassemblyExt-r17 extension introduced for
        // NTN. Note the base enumeration is NOT a uniform run: it steps by 5ms to ms100 and by
        // 10ms from there to ms200, so ms105 and its odd neighbours are not configurable.
        //
        // The base stops at 200ms, which does not reach even one GEO round trip. The extension is
        // a short, irregular list rather than a continuation of the run, and 2200ms is its
        // ceiling -- which a GEO reassembly timer derived as four round trips lands just inside,
        // with about 34ms to spare.
        TimerEnumeration e{"t-Reassembly", {}};
        appendRun(e.valuesMs, 0, 100, 5);
        appendRun(e.valuesMs, 110, 200, 10);
        e.valuesMs.insert(e.valuesMs.end(), {210, 220, 340, 350, 550, 1100, 1650, 2200});
        return sorted(e);
    }();

    static const TimerEnumeration statusProhibit = [] {
        // TS 38.331 T-StatusProhibit. As for t-PollRetransmit, ms1..ms4 are the Rel-16
        // T-StatusProhibit-v1610 code points below the base enumeration's step.
        TimerEnumeration e{"t-StatusProhibit", {1, 2, 3, 4}};
        appendRun(e.valuesMs, 0, 250, 5);
        e.valuesMs.insert(e.valuesMs.end(), {300, 350, 400, 450, 500, 800, 1000, 1200, 1600, 2000, 2400});
        return sorted(e);
    }();

    static const TimerEnumeration retxBsrTimer = [] {
        // TS 38.331 BSR-Config retxBSR-Timer, in subframes (1ms each)
        return TimerEnumeration{"retxBSR-Timer", {10, 20, 40, 80, 160, 320, 640, 1280, 2560, 5120, 10240}};
    }();

    static const TimerEnumeration backoffIndicator = [] {
        // TS 38.321 Table 7.2-1. The UE draws its random-access backoff uniformly from zero to the
        // indicated value, so only the indicated values themselves are legal.
        return TimerEnumeration{"backoff indicator", {0, 5, 10, 20, 30, 40, 60, 80, 120, 160, 240, 320, 480, 960, 1920}};
    }();

    switch (which) {
        case Ntn38331Timer::PollRetransmit: return pollRetransmit;
        case Ntn38331Timer::Reassembly: return reassembly;
        case Ntn38331Timer::StatusProhibit: return statusProhibit;
        case Ntn38331Timer::RetxBsrTimer: return retxBsrTimer;
        case Ntn38331Timer::BackoffIndicator: return backoffIndicator;
    }
    throw cRuntimeError("ntn38331 - unknown timer enumeration %d", static_cast<int>(which));
}

simtime_t fromMs(int milliseconds)
{
    return SimTime(milliseconds, SIMTIME_MS);
}

} // namespace

const char *ntn38331TimerName(Ntn38331Timer which)
{
    return enumerationFor(which).name;
}

simtime_t ntn38331Ceil(simtime_t value, Ntn38331Timer which)
{
    const TimerEnumeration& enumeration = enumerationFor(which);

    for (int candidate : enumeration.valuesMs) {
        if (fromMs(candidate) >= value)
            return fromMs(candidate);
    }

    throw cRuntimeError("ntn38331Ceil - %s cannot be configured to %gms: the largest value the "
            "specification allows is %dms. The scenario's round-trip delay is longer than 3GPP "
            "dimensioned this timer for.",
            enumeration.name, value.dbl() * 1000.0, enumeration.valuesMs.back());
}

simtime_t ntn38331Floor(simtime_t value, Ntn38331Timer which)
{
    const TimerEnumeration& enumeration = enumerationFor(which);

    simtime_t result = fromMs(enumeration.valuesMs.front());
    for (int candidate : enumeration.valuesMs) {
        if (fromMs(candidate) > value)
            break;
        result = fromMs(candidate);
    }
    return result;
}

} // namespace simu5g
