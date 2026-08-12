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
        // TS 38.331 PollRetransmit
        TimerEnumeration e{"t-PollRetransmit", {}};
        appendRun(e.valuesMs, 5, 250, 5);
        e.valuesMs.insert(e.valuesMs.end(), {300, 350, 400, 450, 500, 800, 1000, 2000, 4000});
        return e;
    }();

    static const TimerEnumeration reassembly = [] {
        // TS 38.331 T-Reassembly, plus the Rel-17 t-ReassemblyExt-r17 extension introduced for
        // NTN. The base enumeration stops at 200ms, which does not reach even one GEO round trip;
        // 2200ms is the ceiling RAN2 chose for the extension, and a GEO reassembly timer derived
        // as four round trips lands just under it.
        TimerEnumeration e{"t-Reassembly", {}};
        appendRun(e.valuesMs, 0, 200, 5);
        e.valuesMs.insert(e.valuesMs.end(), {750, 1000, 1250, 1500, 1750, 2000, 2200});
        return e;
    }();

    static const TimerEnumeration statusProhibit = [] {
        // TS 38.331 T-StatusProhibit
        TimerEnumeration e{"t-StatusProhibit", {}};
        appendRun(e.valuesMs, 0, 250, 5);
        e.valuesMs.insert(e.valuesMs.end(), {300, 350, 400, 450, 500, 800, 1000, 1200, 1600, 2000, 2400});
        return e;
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
