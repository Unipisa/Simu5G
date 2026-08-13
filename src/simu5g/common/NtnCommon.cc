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

#include "simu5g/common/NtnCommon.h"

#include <algorithm>
#include <vector>

#include <inet/common/ModuleAccess.h>

#include "simu5g/common/GeoUtils.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/mobility/georeference/GeographicReferenceSystem.h"
#include "simu5g/stack/mac/LteMacBase.h"
#include "simu5g/world/radio/ChannelAccess.h"

namespace simu5g {

using namespace omnetpp;

namespace {

//
// The geographic reference system this network converts positions against.
//
// Resolved per call rather than cached in a file-scope variable: the pointer is only valid for the
// current run, and Cmdenv executes -r 0..N in a single process, so a cached one would dangle into a
// destroyed module tree from the second run onward.
//
GeographicReferenceSystem *referenceSystem(const char *context)
{
    GeographicReferenceSystem *referenceSystem = GeographicReferenceSystemAccess().get();
    if (referenceSystem == nullptr)
        throw cRuntimeError("%s - the network has no GeographicReferenceSystem module, so NTN geometry "
                "cannot be evaluated", context);
    return referenceSystem;
}

//
// Position of the radio a node uses on the given link.
//
// Deliberately not routed through the node's mobility submodule: on a MovingMobilityBase,
// getCurrentPosition() also advances that module's state and emits mobilityStateChangedSignal,
// which is not this code's business to trigger. ChannelAccess caches the position that the
// channel model and NtnPropagationDelay both use, so reading it here keeps the round-trip
// delay on exactly the same geometry as the per-hop delay and the path loss.
//
const inet::Coord& ntnRadioPosition(Binder *binder, MacNodeId nodeId, bool serviceLink)
{
    cModule *radioModule = nullptr;
    RanNodeType nodeType = getNodeTypeById(nodeId);

    if (nodeType == SATELLITE_NODE) {
        SatelliteInfo *info = binder->getSatelliteInfo(nodeId);
        if (info == nullptr || info->satelliteModule == nullptr)
            throw cRuntimeError("ntnRadioPosition - satellite %hu is not registered", num(nodeId));
        // The two NICs share one mobility module, so both report the same position today. The
        // link is still named explicitly, because that will stop being true as soon as a
        // satellite carries separately-placed antennas.
        cModule *nic = info->satelliteModule->getSubmodule(serviceLink ? "serviceNic" : "feederNic");
        radioModule = (nic != nullptr) ? nic->getSubmodule("phy") : nullptr;
    }
    else if (nodeType == NTN_GATEWAY_NODE) {
        NtnGatewayInfo *info = binder->getNtnGatewayInfo(nodeId);
        if (info == nullptr || info->gatewayModule == nullptr)
            throw cRuntimeError("ntnRadioPosition - NTN gateway %hu is not registered", num(nodeId));
        // A gateway radiates towards the satellite over the feeder link only; its gNodeB-facing
        // side is the wired fronthaul and has no radio position.
        cModule *nic = info->gatewayModule->getSubmodule("feederNic");
        radioModule = (nic != nullptr) ? nic->getSubmodule("phy") : nullptr;
    }
    else {
        radioModule = binder->getPhyByNodeId(nodeId);
    }

    if (radioModule == nullptr)
        throw cRuntimeError("ntnRadioPosition - node %hu has no %s-link radio module. A round-trip "
                "delay cannot be computed without the positions of both endpoints.",
                num(nodeId), serviceLink ? "service" : "feeder");

    return check_and_cast<ChannelAccess *>(radioModule)->getRadioPosition();
}

//
// The same position in ECEF, using the same WGS84 conversion as NtnPropagationDelay.
//
inet::Coord ntnRadioEcefPosition(Binder *binder, MacNodeId nodeId, bool serviceLink,
        const GeographicReferenceSystem *reference)
{
    return ecefFromWgs84(reference->wgs84FromOmnet(ntnRadioPosition(binder, nodeId, serviceLink)));
}

//
// Derives the cell's worst-case round-trip delay from the satellite's altitude and the cell's own
// bounds. Pure geometry: the caller is responsible for publishing the result.
//
simtime_t computeCellRoundTripDelay(Binder *binder, const GnbNtnAssociation& association)
{
    const GeographicReferenceSystem *reference = referenceSystem("ntnCellRoundTripDelay");

    inet::GeoCoord satelliteWgs84 =
            reference->wgs84FromOmnet(ntnRadioPosition(binder, association.satelliteId, true));
    double altitude = satelliteWgs84.altitude.get();

    // A satellite sitting at the geographic reference altitude is not a satellite. This is the
    // guard that catches a query issued before inet::INITSTAGE_SINGLE_MOBILITY, where every
    // ChannelAccess still reports (0,0,0) -- which converts to a perfectly plausible point on the
    // geoid, so no range check would notice.
    if (altitude < association.minSatelliteAltitude)
        throw cRuntimeError("ntnCellRoundTripDelay - satellite %hu is at an altitude of %g km, below "
                "the ntnMinSatelliteAltitude (%g km) of cell %hu. Either its position was never initialised -- "
                "the round-trip delay cannot be queried before inet::INITSTAGE_SINGLE_MOBILITY -- or the scenario "
                "misplaces it.",
                num(association.satelliteId), altitude / 1000.0, association.minSatelliteAltitude / 1000.0,
                num(association.gnbId));

    // The longest service link and the longest feeder link this cell will use, both bounded by the
    // lowest elevation it accepts, and both traversed twice per round trip.
    //
    // Nothing is added on top. The timers derived from this value must be an upper bound rather
    // than an estimate, and their own rounding to the next value TS 38.331 can signal is what
    // supplies that: for a GEO round trip the poll retransmit enumeration jumps 500ms -> 800ms.
    // A separate margin knob would only shift where in that gap the value lands, and near GEO
    // there is barely room for one anyway -- four round trips already sit just inside the 2200ms
    // ceiling of t-ReassemblyExt-r17. A scenario that needs more headroom should set the timer it
    // cares about explicitly.
    double maxSlantRange = computeSlantRangeAtElevation(altitude, association.minElevation);
    simtime_t roundTripDelay = 4 * maxSlantRange / SPEED_OF_LIGHT;

    // Reported once per cell, at INFO because EV_DEBUG is compiled out under NDEBUG and because a
    // round-trip delay that silently came out wrong is indistinguishable from a channel problem.
    EV_INFO << "ntnCellRoundTripDelay - cell " << association.gnbId << " via satellite "
            << association.satelliteId << " at altitude[" << altitude / 1000.0 << "km]: worst-case slant range["
            << maxSlantRange / 1000.0 << "km] at elevation[" << association.minElevation << "deg], round-trip delay["
            << roundTripDelay.dbl() * 1000.0 << "ms]" << endl;

    return roundTripDelay;
}

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
        // TS 38.321 Table 7.2-1, indices 0..13; 14 and 15 are Reserved. The table starts at 5ms --
        // there is no zero code point, because "no backoff" is signalled by omitting the BI field
        // from the random-access response rather than by indicating zero. The UE then draws its
        // backoff uniformly between zero and the indicated value, which is why the *lower* bound
        // of that draw is a separate parameter fixed at zero and only the upper bound is snapped
        // to this table.
        return TimerEnumeration{"backoff indicator", {5, 10, 20, 30, 40, 60, 80, 120, 160, 240, 320, 480, 960, 1920}};
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

simtime_t ntnCellRoundTripDelay(Binder *binder, MacNodeId gnbId)
{
    const GnbNtnAssociation *association = binder->getGnbNtnAssociation(gnbId);
    if (association == nullptr)
        return SIMTIME_ZERO;

    if (association->cellRoundTripDelay > SIMTIME_ZERO)
        return association->cellRoundTripDelay;

    simtime_t roundTripDelay = computeCellRoundTripDelay(binder, *association);
    binder->setGnbNtnCellRoundTripDelay(gnbId, roundTripDelay);
    return roundTripDelay;
}

simtime_t ntnCellRoundTripDelay(cSimpleModule *entity)
{
    auto *mac = inet::getModuleFromPar<LteMacBase>(entity->par("macModule"), entity);
    auto *binder = inet::getModuleFromPar<Binder>(entity->par("binderModule"), entity);

    // getMacCellId() is the serving gNodeB at a UE and the node itself at a gNodeB, which is the
    // identifier the Binder keys its NTN associations by in both cases.
    return ntnCellRoundTripDelay(binder, mac->getMacCellId());
}

simtime_t ntnRoundTripDelay(Binder *binder, MacNodeId gnbId, MacNodeId ueId)
{
    const GnbNtnAssociation *association = binder->getGnbNtnAssociation(gnbId);
    if (association == nullptr)
        return SIMTIME_ZERO;

    const GeographicReferenceSystem *reference = referenceSystem("ntnRoundTripDelay");

    inet::Coord ueEcef = ntnRadioEcefPosition(binder, ueId, true, reference);
    inet::Coord satelliteServiceEcef = ntnRadioEcefPosition(binder, association->satelliteId, true, reference);
    inet::Coord satelliteFeederEcef = ntnRadioEcefPosition(binder, association->satelliteId, false, reference);
    inet::Coord gatewayEcef = ntnRadioEcefPosition(binder, association->ntnGatewayId, false, reference);

    double serviceRange = ueEcef.distance(satelliteServiceEcef);
    double feederRange = satelliteFeederEcef.distance(gatewayEcef);

    // The gNodeB-to-gateway fronthaul is a wired connection whose delay, if any, belongs to that
    // connection's channel rather than here.
    simtime_t roundTripDelay = 2 * (serviceRange + feederRange) / SPEED_OF_LIGHT;

    EV_DEBUG << "ntnRoundTripDelay - UE " << ueId << " in cell " << gnbId << ": service link["
             << serviceRange / 1000.0 << "km] feeder link[" << feederRange / 1000.0 << "km] round-trip delay["
             << roundTripDelay.dbl() * 1000.0 << "ms]" << endl;

    return roundTripDelay;
}

int ntnHarqTransmissions(cSimpleModule *entity)
{
    auto *mac = inet::getModuleFromPar<LteMacBase>(entity->par("macModule"), entity);
    return mac->par("maxHarqRtx").intValue() + 1;
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
