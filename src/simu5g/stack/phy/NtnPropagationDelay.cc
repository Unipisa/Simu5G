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

#include "simu5g/stack/phy/NtnPropagationDelay.h"

#include <cmath>

#include "simu5g/common/GeoUtils.h"
#include "simu5g/world/radio/ChannelAccess.h"

namespace simu5g {

using namespace omnetpp;
using namespace inet;

simsignal_t NtnPropagationDelay::hopPropagationDelaySignal_ = cComponent::registerSignal("ntnHopPropagationDelay");
simsignal_t NtnPropagationDelay::hopSlantRangeSignal_ = cComponent::registerSignal("ntnHopSlantRange");

void NtnPropagationDelay::initialize(cSimpleModule *owner, GeographicReferenceSystem *referenceSystem)
{
    owner_ = owner;
    referenceSystem_ = referenceSystem;
    if (referenceSystem_ == nullptr)
        throw cRuntimeError("NtnPropagationDelay::initialize - %s was given no GeographicReferenceSystem",
                owner->getFullPath().c_str());

    enabled_ = owner->par("useGeometricPropagationDelay");
    transparentPayloadRelay_ = owner->par("transparentPayloadRelay");
    maxHopDelay_ = owner->par("maxHopPropagationDelay");
    reportThreshold_ = owner->par("propagationDelayReportThreshold");

    // A silently zero-delay NTN path is indistinguishable from a broken one, so say so once
    // at INFO rather than leaving the user to infer it from implausible latencies.
    if (!enabled_)
        EV_INFO << "NtnPropagationDelay::initialize - geometric propagation delay is DISABLED on "
                << owner->getFullPath() << "; its NTN radio hops will be instantaneous" << endl;
    if (!transparentPayloadRelay_)
        EV_INFO << "NtnPropagationDelay::initialize - transparent-payload relaying is DISABLED on "
                << owner->getFullPath() << "; the frame duration is charged on every radio hop, so a "
                << "two-hop path costs two frame durations end to end" << endl;
}

NtnPropagationDelay::Endpoint NtnPropagationDelay::resolveEndpoint(const inet::Coord& omnetPosition) const
{
    Endpoint endpoint;
    endpoint.wgs84 = referenceSystem_->wgs84FromOmnet(omnetPosition);
    endpoint.ecef = ecefFromWgs84(endpoint.wgs84);
    return endpoint;
}

inet::Coord NtnPropagationDelay::ecefFromRadioPosition(const inet::Coord& omnetPosition) const
{
    return resolveEndpoint(omnetPosition).ecef;
}

simtime_t NtnPropagationDelay::transmissionDuration(RanNodeType transmitterType, simtime_t frameDuration) const
{
    // A transparent payload is an analog repeater: it re-radiates as it receives rather than
    // storing the frame, so the frame's transmission time is spent once end to end, not once
    // per radio hop. Charging it on the hop where the satellite transmits -- satellite->UE
    // for the downlink, satellite->gateway for the uplink -- puts end-of-reception at
    // t + d1/c + d2/c + duration, which is the physically correct instant. Charging it on the
    // hop *into* the satellite instead would model a store-and-forward relay.
    //
    // None of the radio gates on the NTN path declare deliverImmediately, so sendDirect()
    // delivers at t + propagationDelay + duration; passing zero here is what makes the
    // intermediate hop cut-through.
    return (transparentPayloadRelay_ && transmitterType != SATELLITE_NODE) ? SIMTIME_ZERO : frameDuration;
}

ChannelAccess *NtnPropagationDelay::resolveReceivingRadio(cGate *targetGate) const
{
    // The receiving PHY's cached radio position is exactly what the receiving channel model
    // will use for this hop (NtnChannelModel::getSINR() reads phy_->getRadioPosition()), so
    // resolving it here keeps delay and path loss on one geometry.
    //
    // Deliberately not routed through the receiver's mobility submodule: on a
    // MovingMobilityBase, getCurrentPosition() also advances that module's state and emits
    // mobilityStateChangedSignal, which is not this module's business to trigger from inside
    // a transmitter's send path.
    cGate *endGate = targetGate->getPathEndGate();
    auto *radio = dynamic_cast<ChannelAccess *>(endGate->getOwnerModule());
    if (radio == nullptr)
        throw cRuntimeError("NtnPropagationDelay::resolveReceivingRadio - the connection path from gate %s "
                "ends at %s, which is not a radio. A transparent NTN hop cannot be given a propagation delay "
                "without the receiving PHY's position; check that the receiving node wires its @directIn radio "
                "gate through to a PHY module.",
                targetGate->getFullPath().c_str(), endGate->getFullPath().c_str());
    return radio;
}

simtime_t NtnPropagationDelay::computeHopDelay(const inet::Coord& txPosition, cGate *targetGate,
        MacNodeId transmitterId, MacNodeId receiverId)
{
    // Note there is deliberately no horizon check here. The slant range is finite and well
    // defined for any relative position, and NtnChannelModel::getSINR() already handles a
    // below-horizon link by returning -INFINITY per band while letting the frame propagate.
    // Rejecting such a hop here would change frame *delivery* rather than only its delay, and
    // would silently diverge from the channel model.
    if (!enabled_)
        return SIMTIME_ZERO;

    ChannelAccess *receivingRadio = resolveReceivingRadio(targetGate);
    Endpoint transmitter = resolveEndpoint(txPosition);
    Endpoint receiver = resolveEndpoint(receivingRadio->getRadioPosition());
    double range = transmitter.ecef.distance(receiver.ecef);

    if (!std::isfinite(range) || range <= 0.0)
        throw cRuntimeError("NtnPropagationDelay::computeHopDelay - hop %hu -> %hu has a slant range of %g m, "
                "which is not a usable distance. Either the two radios are co-located, or one of their positions "
                "was never initialised.", num(transmitterId), num(receiverId), range);

    simtime_t delay = range / SPEED_OF_LIGHT;

    if (maxHopDelay_ > SIMTIME_ZERO && delay > maxHopDelay_)
        throw cRuntimeError("NtnPropagationDelay::computeHopDelay - hop %hu -> %hu spans %g km, i.e. a one-way "
                "propagation delay of %g ms, which exceeds maxHopPropagationDelay (%g ms). Either the scenario "
                "places a radio implausibly far away, or a radio position was never initialised. Raise the "
                "parameter if the geometry is intended.",
                num(transmitterId), num(receiverId), range / 1000.0, delay.dbl() * 1000.0,
                maxHopDelay_.dbl() * 1000.0);

    reportHop(transmitterId, receiverId, range, delay, transmitter, receiver);

    owner_->emit(hopPropagationDelaySignal_, delay);
    owner_->emit(hopSlantRangeSignal_, range);

    EV_DEBUG << "NtnPropagationDelay::computeHopDelay - hop " << transmitterId << " -> " << receiverId
             << " txEcef" << transmitter.ecef << " rxEcef" << receiver.ecef
             << " range[" << range << "m] delay[" << delay << "]" << endl;

    return delay;
}

void NtnPropagationDelay::reportHop(MacNodeId transmitterId, MacNodeId receiverId, double range,
        simtime_t delay, const Endpoint& transmitter, const Endpoint& receiver)
{
    auto [it, inserted] = lastReportedDelay_.insert({{transmitterId, receiverId}, delay});
    if (!inserted) {
        if (reportThreshold_ > SIMTIME_ZERO && std::fabs((delay - it->second).dbl()) < reportThreshold_.dbl())
            return;
        it->second = delay;
    }

    EV_INFO << "NtnPropagationDelay::reportHop - radio hop " << transmitterId << " -> " << receiverId
            << ": slantRange[" << range / 1000.0 << "km], oneWayDelay[" << delay.dbl() * 1000.0 << "ms]";

    // Elevation is reported alongside the range so this line can be checked against
    // NtnChannelModel::reportSatelliteVisibility() without correlating two logs. It is taken
    // at whichever endpoint is not the satellite; a hop with no terrestrial endpoint is not
    // reachable on the transparent path, and simply omits it.
    bool txIsSatellite = getNodeTypeById(transmitterId) == SATELLITE_NODE;
    bool rxIsSatellite = getNodeTypeById(receiverId) == SATELLITE_NODE;
    if (txIsSatellite != rxIsSatellite) {
        const Endpoint& terrestrial = txIsSatellite ? receiver : transmitter;
        const Endpoint& satellite = txIsSatellite ? transmitter : receiver;
        double elevation = computeElevationFromEcefEndpoints(terrestrial.wgs84, terrestrial.ecef, satellite.ecef);
        EV_INFO << ", elevation[" << elevation << "deg]";
    }

    EV_INFO << endl;
}

} // namespace simu5g
