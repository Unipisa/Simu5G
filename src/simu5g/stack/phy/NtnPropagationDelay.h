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

#ifndef __SIMU5G_NTNPROPAGATIONDELAY_H_
#define __SIMU5G_NTNPROPAGATIONDELAY_H_

#include <map>
#include <utility>

#include <inet/common/geometry/common/Coord.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/mobility/georeference/GeographicReferenceSystem.h"

namespace simu5g {

class ChannelAccess;

//
// Geometry-derived propagation delay for the transparent NTN radio hops.
//
// NtnPhyBase derives from ChannelAccess and NtnPhyUe from NrPhyUe, so the two PHYs that
// radiate over the satellite path have no common base class. This is the shared answer to
// "how long does a frame take to reach the radio behind that gate, and how much of the
// frame's transmission time does this hop owe". It is not a module: both PHYs hold one by
// value, configure it from their own NED parameters and emit its statistics on themselves.
//
// Both endpoints are converted OMNeT++ -> WGS84 -> ECEF. That is what NtnChannelModel does
// for a terrestrial endpoint and, transitively, for the satellite endpoint too: the PHYs
// fill radioTransmitterEcefCoord by this same route, and that is the value getSINR() trusts
// for a SATELLITE_NODE transmitter. Delay and path loss therefore describe one geometry by
// construction rather than by coincidence.
//
// The delay is a one-shot light time evaluated from the endpoint positions at transmission.
// It ignores the receiver's motion during the hop -- about 50m, i.e. 0.17us, for a 7ms LEO
// hop -- and the receiver's position snapshot may additionally be up to one mobility
// updateInterval stale. The iterative light-time solution is not worth its cost at this
// fidelity; the approximation is stated rather than assumed.
//
class NtnPropagationDelay
{
  public:
    // Call from the owning PHY's initialize() at INITSTAGE_LOCAL, after its node id and
    // node type have been read.
    void initialize(omnetpp::cSimpleModule *owner, GeographicReferenceSystem *referenceSystem);

    bool isEnabled() const { return enabled_; }

    // ECEF position of a radio placed at the given local OMNeT++ position. Also used by the
    // call sites to fill UserControlInfo::radioTransmitterEcefCoord, so the geometry the
    // delay uses and the geometry the channel model uses cannot drift apart.
    inet::Coord ecefFromRadioPosition(const inet::Coord& omnetPosition) const;

    // Transmission time this hop owes. Under the transparent-payload convention the
    // satellite is an analog repeater that does not re-buffer, so the frame duration is
    // charged once end-to-end, on the hop where the satellite is the transmitter.
    omnetpp::simtime_t transmissionDuration(RanNodeType transmitterType,
            omnetpp::simtime_t frameDuration) const;

    // One-way light time from the transmitting radio to the radio that will receive a frame
    // sent to targetGate. txPosition is the transmitter's local OMNeT++ position, i.e. its
    // getRadioPosition(). Reports the hop and emits the per-hop statistics. Returns
    // SIMTIME_ZERO when disabled. Aborts the run when the receiving radio cannot be
    // resolved, when the range is zero or not finite, or when the delay exceeds
    // maxHopPropagationDelay.
    omnetpp::simtime_t computeHopDelay(const inet::Coord& txPosition, omnetpp::cGate *targetGate,
            MacNodeId transmitterId, MacNodeId receiverId);

  private:
    // One radio's position in the two frames this class needs: WGS84 for the elevation
    // report, ECEF for the range.
    struct Endpoint {
        inet::GeoCoord wgs84 = inet::GeoCoord::NIL;
        inet::Coord ecef;
    };

    Endpoint resolveEndpoint(const inet::Coord& omnetPosition) const;

    // Follows the connection path from a node- or NIC-level @directIn gate to the PHY that
    // will actually receive the frame.
    ChannelAccess *resolveReceivingRadio(omnetpp::cGate *targetGate) const;

    // Reports a hop the first time it is used, and afterwards only once its delay has moved
    // by more than reportThreshold_. Mirrors NtnChannelModel::reportSatelliteVisibility().
    void reportHop(MacNodeId transmitterId, MacNodeId receiverId, double range,
            omnetpp::simtime_t delay, const Endpoint& transmitter, const Endpoint& receiver);

    omnetpp::opp_component_ptr<omnetpp::cSimpleModule> owner_;
    GeographicReferenceSystem *referenceSystem_ = nullptr;
    bool enabled_ = true;
    bool transparentPayloadRelay_ = true;
    omnetpp::simtime_t maxHopDelay_ = 0;
    omnetpp::simtime_t reportThreshold_ = 0;
    std::map<std::pair<MacNodeId, MacNodeId>, omnetpp::simtime_t> lastReportedDelay_;

    static omnetpp::simsignal_t hopPropagationDelaySignal_;
    static omnetpp::simsignal_t hopSlantRangeSignal_;
};

} // namespace simu5g

#endif
