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

} // namespace simu5g
