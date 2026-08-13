# Simu5G NTN Implementation

This document describes the partial non-terrestrial network (NTN) implementation on the `ntn` branch. It was derived from the code and branch history and reviewed through commit `1aa1083d` (`Make two-hop SINR evaluation mandatory on the transparent NTN path`). Treat the source files cited below as authoritative if the branch evolves.

## Scope and Status

The branch implements a prototype transparent, frequency-translating satellite path:

```text
downlink: gNB -> wired fronthaul -> gateway -> feeder radio -> satellite -> service radio -> UE
uplink:   UE -> service radio -> satellite -> feeder radio -> gateway -> wired fronthaul -> gNB
```

The satellite and gateway relay PHY airframes. They do not terminate the NR stack or UE procedures; scheduling, HARQ, CSI processing, and protocol termination remain at the terrestrial gNB and UE, although the gateway computes the final uplink radio-reception result. This is the intended architecture of a bent-pipe payload, but the current physical and protocol behavior is incomplete.

The `ntn` branch adds 62 commits (after squashing from 92) on top of `cqi-computation` (`d9c2b126`). It depends on that branch's CSI-RS/SRS-based local feedback work. `cqi-computation` has been rebased onto `master`, so NTN is now positioned downstream of the latest master history.

Available now:

- Dedicated NTN gNB, gateway, satellite, relay, fronthaul, feeder-link, and service-link modules.
- Static Binder associations among one gNB, one gateway, and one satellite.
- Bidirectional forwarding of data and control airframes over the transparent path.
- Per-hop data-channel evaluation and harmonic combination of two-hop data SINR.
- Current-hop transmit power selected from the antenna model of the UE, gateway, or satellite that actually radiates the frame.
- GEO placement and TLE/SGP4-based LEO position updates.
- WGS84, ECEF, and local OMNeT++ coordinate conversion through GeographicLib.
- Satellite, gateway, isotropic terminal, and VSAT antenna models.
- A channel model based partly on 3GPP TR 38.811 tables and TR 38.821 antenna parameters.
- Per-link carrier frequencies: a feeder-link NIC retunes its own channel models to the translated carrier, so every frequency-dependent term of that hop is computed at the feeder frequency.
- Enforced two-hop evaluation: a frame that reaches the final receiver without a first-hop measurement, or a hop that cannot be evaluated for want of a channel model, aborts the run rather than degrading silently. This applies to SINR (data, CSI-RS, SRS) and, since the beacon RSSI fix, to RSRP as well.
- Two-hop beacon RSSI: a `BEACONPKT` reaches attached UEs through the satellite the same way CSI-RS does, and `NtnChannelModel::computeReceptionRsrp()` combines both hops for `NtnPhyUe::computeReceivedBeaconPacketRssi()`, feeding the (still unimplemented) handover machinery a correct measurement instead of a fictitious direct-geometry one.
- Geometry-derived per-hop propagation delay, with the frame duration charged once end to end as a bent-pipe transponder requires. See item 2 below and `ntn-delay-and-timers.md`.
- NTN-aware protocol timers throughout: RLC and RRC (`NtnNrRlcAmEntity`, `NtnNrRlcUmEntity`), the RAC and BSR counters (`NtnNrMacUe`), and HARQ (32 processes, downlink feedback disabled). Every value is derived at runtime from the cell's own round-trip delay, which `ntnCellRoundTripDelay()` computes from the satellite's altitude and the cell's configured minimum elevation, and is then rounded to a value TS 38.331 can actually signal. There is no orbit label to set or to get wrong. See `ntn-delay-and-timers.md`.
- Graceful teardown on a delayed link: MAC and RLC both discard data belonging to bearers released while frames were still in flight, which over a satellite link means for a further round-trip time.
- Minimal GEO and LEO bidirectional CBR smoke scenarios, plus RLC AM variants.

Not available as a complete model:

- Validation at realistic load. Every measurement so far is one UE at 30 kB/s over 2 s. GEO downlink now reaches 85% of that offered load, but GEO uplink remains grant-limited at about a quarter of it — the BSR/grant round trip, not HARQ — and nothing has been run with many UEs, saturating load, or across a full LEO pass. A UE also never stops retrying random access after its satellite sets, since `LteMacUe::macHandleRac()` resets the attempt counter on exhaustion.
- Validation of the LEO timer values specifically. They are now derived from geometry rather than hand-set, and the derivation is demonstrably live, but at an 8.3 ms round trip under the smoke load none of the RAC/BSR counters ever binds — `LeoSat` gives identical results whatever they are set to. See "RAC and BSR as built" in `ntn-delay-and-timers.md`.
- An input-dependent bent-pipe transponder model with payload gain, added noise, bandwidth limits, output backoff, and saturation.
- Satellite-relative Doppler beyond the carrier-frequency scaling (the terrestrial-endpoint speed is still the only source of relative motion).
- Satellite-relative Doppler, compensation, and residual frequency error.
- Interference, beams, coverage management, satellite selection, and handover.
- Automated NTN tests or a current trusted result baseline.

## Architecture

### Nodes and NICs

`src/simu5g/nodes/NtnGNodeB.ned` extends `gNodeB` and replaces its cellular NIC with `NtnNrNic`. Its `NtnPhyGnb` sends all broadcast and unicast airframes through a wired `NtnFronthaulNic` toward the configured gateway. The parameters `ntnGatewayId` and `satelliteId` identify a fixed path.

`src/simu5g/nodes/NtnGateway.ned` contains:

- `gnbNic: NtnFronthaulNic` for the wired gNB-facing side;
- `feederNic: NtnFeederLinkNic` for the satellite radio side;
- `relay: NtnRelay` between the two NICs;
- stationary mobility and a `GatewayAntennaModel` by default.

`src/simu5g/nodes/Satellite.ned` contains:

- `serviceNic: NtnServiceLinkNic` for UE-facing radio traffic;
- `feederNic: NtnFeederLinkNic` for gateway-facing radio traffic;
- `relay: NtnRelay` between the two NICs;
- one mobility module shared by both radio sides.

`GeoSatellite`, `Leo600Satellite`, and `Leo1200Satellite` select mobility and platform antenna defaults. These variants differ in position/orbit and antenna parameters, not in relay behavior.

`src/simu5g/nodes/NtnRelay.{ned,cc,h}` forwards every message between its left and right NIC gates with `sendDelayed()`. `relayDelay` defaults to zero and represents processing delay only. It is not slant-range propagation delay.

`src/simu5g/stack/NtnFeederLinkNic.ned` and `NtnServiceLinkNic.ned` each contain an `NtnPhyBase`, one or more `NtnChannelModel` modules, and an antenna model. The same C++ PHY class handles both sides according to its `linkType` parameter.

Gateway-to-satellite (feeder) and UE-to-satellite (service) links have no static NED wireless connection at all: `feederLinkRadioIn`/`serviceLinkRadioIn` are `@directIn` gates, and `NtnPhyBase::handleUpperMessage()`/`NtnPhyUe::sendUnicastViaNtn()` resolve the peer module through the Binder at runtime and deliver with `sendDirect()`. The only static NED radio-facing wiring is the gNB-to-gateway wired fronthaul (`gnb.ntn <--> ntnGateway.gnbLink`). Any beam/coverage/handover work (see item 5 below) changes Binder-driven peer resolution, not NED topology.

### Node IDs and Binder Associations

NTN node ID ranges are reserved in `src/simu5g/common/LteTypes.h`:

- gateways: 16384 through 17407;
- satellites: 17408 through 18431.

`NtnRelay` registers gateway and satellite modules during `INITSTAGE_SIMU5G_REGISTRATIONS` and unregisters them in `finish()`.

`NtnIp2Nic` registers a `GnbNtnAssociation` during `INITSTAGE_SIMU5G_NODE_RELATIONSHIPS`. It retries at simulation time zero because a satellite created by `SatelliteInserter` may finish registration after the gNB's normal relationship stage. All current associations set `isTransparent = true`.

The Binder relation is:

```cpp
struct GnbNtnAssociation {
    MacNodeId gnbId;
    MacNodeId ntnGatewayId;
    MacNodeId satelliteId;
    bool isTransparent;
};
```

Relevant APIs are in `src/simu5g/common/binder/Binder.{h,cc}`. Current lookup rules imply:

- each gNB has one active NTN tuple;
- multiple gNBs may share the same identical gateway/satellite tuple;
- one gateway cannot map to different satellites;
- one satellite cannot map to different gateways;
- associations are static and have no beam, validity interval, coverage, or handover state.

The one-gateway-to-one-satellite and one-satellite-to-one-gateway rules are enforced at runtime, not just by convention: `Binder::getAssociatedSatelliteForGateway()` and `getAssociatedGatewayForSatellite()` linear-scan all associations and `throw cRuntimeError` if they find more than one distinct peer. Any dynamic-association or handover work (item 5 below) must change these lookups, or they will abort the simulation the moment two gNBs briefly reference different satellites through the same gateway.

Turning that geometry into a round-trip delay — which is what every NTN protocol timer is dimensioned from — is *not* the Binder's job; it lives in `common/NtnCommon.{h,cc}`, which reads the associations the Binder holds:

- `ntnCellRoundTripDelay(binder, gnbId)` — the worst case anywhere in the cell, bounded by the cell's own `ntnMinElevation` rather than by where the satellite currently is. Derived on first query and published back onto the association, so it is derived once and read identically by the gNodeB and by every UE it serves; constant for a circular orbit. This is the value the timers use, and the one to use before a UE has attached.
- `ntnRoundTripDelay(binder, gnbId, ueId)` — the instantaneous delay from the actual UE, satellite and gateway positions. Tracks satellite motion, never cached. No timer consumes it yet; it exists for per-UE diagnostics and for grant timing (item 6 of `ntn-delay-and-timers.md`).

`ntnMinElevation` and `ntnMinSatelliteAltitude` are per-cell parameters on `NtnGNodeB`, published onto the association by `NtnIp2Nic` at registration.

Both return zero for a gNodeB with no NTN association, which is what leaves terrestrial scenarios untouched. Positions come from `ChannelAccess::getRadioPosition()` — the same cached snapshot the channel model and `NtnPropagationDelay` use, so delay, path loss and round-trip delay cannot drift apart — and never from a mobility module directly, since `getCurrentPosition()` advances a `MovingMobilityBase` and emits a signal.

The UE still attaches to the logical gNB using the ordinary serving-node relation. It discovers the satellite path by looking up the serving gNB's NTN association. The actual satellite is deliberately not exposed as the logical source of downlink frames.

### NTN Airframe Metadata

`src/simu5g/stack/phy/packet/NtnAirFrame.msg` extends `LteAirFrame` with:

- `relayHopSinr[]`: first radio-hop SINR, one value per band;
- `relayHopRsrp[]`: first radio-hop RSRP, one value per band, populated for beacon frames;
- `attachedUes[]`: CSI-RS and beacon fan-out targets;
- `gatewayReceptionResultValid` and `gatewayReceptionResult`: uplink decoding decision made at the gateway;
- `endToEndSinrValid` and `endToEndSinr[]`: combined two-hop uplink SINR for SRS and control-frame CSI, stored by the gateway.

`UserControlInfo` in `src/simu5g/common/LteControlInfo.msg` carries actual current-hop metadata separately from logical source/destination IDs:

- radio transmitter and receiver IDs;
- transmitter local and ECEF coordinates;
- transmitter antenna model;
- current-hop transmit power in dBm.

This separation is fundamental to the transparent design. `sourceId` remains the gNB or UE for stack semantics while `radioTransmitterId` identifies the gateway or satellite currently radiating the frame.

Do not edit generated `*_m.cc` or `*_m.h` files. Change the `.msg` source and regenerate through the normal build.

## Packet Flow

### Downlink Data

1. The normal gNB stack builds a MAC frame.
2. `NtnPhyGnb::createAirFrame()` creates an `NtnAirFrame`.
3. `NtnPhyGnb::sendBroadcast()` and `sendUnicast()` send it over the wired NTN gate rather than terrestrial radio.
4. The gateway fronthaul NIC and relay pass the frame to the feeder PHY.
5. Gateway `NtnPhyBase` adds the configured feeder frequency offset, records gateway radio metadata and feeder-antenna transmit power, and directly sends the frame to the associated satellite.
6. Satellite feeder `NtnPhyBase` computes and stores feeder-hop SINR in the frame.
7. The satellite relay sends the frame to the service PHY.
8. Satellite service `NtnPhyBase` subtracts the offset, records satellite radio metadata and service-antenna transmit power, and directly sends the frame to the UE.
9. `NtnPhyUe` selects its NTN channel model from the logical source gNB's Binder association. `NtnPhyUe::getReceptionChannelModel()` carries an explicit in-code review note: it infers "this frame arrived via satellite" from the *source gNB's* transparent association rather than the actual radio sender, because `sourceId` intentionally stays logical; the comment flags this as something to revisit if downlink frames ever expose the real satellite ID.
10. `NtnChannelModel::computeReceptionSinr()` combines feeder and service SINR in linear units using `1 / (1/SINR1 + 1/SINR2)` before ordinary BLER evaluation.

### Uplink Data

1. `NtnPhyUe::sendUnicast()` checks whether the serving gNB has a transparent NTN association.
2. If needed, it converts the ordinary `LteAirFrame` into `NtnAirFrame`, preserving duration, priority, control information, and payload.
3. The UE records actual transmitter metadata and directly sends the frame to the associated satellite service-link input.
4. Satellite service `NtnPhyBase` stores first-hop SINR in `relayHopSinr[]`.
5. The satellite relay sends the frame to feeder `NtnPhyBase`, which shifts frequency, selects the satellite feeder-antenna transmit power, and directly sends it to the gateway.
6. Gateway feeder `NtnPhyBase` combines both hops, calls `isReceptionSuccessful()`, and stores the Boolean result in the frame.
7. For SRS frames, the gateway also combines `relayHopSinr[]` with its own measurement using the harmonic formula and stores the result in `endToEndSinr[]`.
8. The gateway fronthaul NIC translates the frequency back to the service carrier and forwards the frame to the gNB.
9. `NtnPhyGnb` validates the frame and trusts the gateway's stored reception result instead of recomputing the radio channel. `NtnPhyGnb::handleNtnAirFrame()` throws `cRuntimeError` if a non-control (data) `NtnAirFrame` arrives without `gatewayReceptionResultValid` set — there is no fallback recomputation path.
10. For SRS frames, `NtnPhyGnb::handleSrsReferenceSignal()` reads the stored `endToEndSinr[]` and passes it to `LteUlFeedbackGenerator::computeUlFeedback()`, which derives the uplink CQI from the actual satellite path rather than re-measuring a terrestrial link. `NtnPhyGnb` throws an error if `endToEndSinr[]` is not set.

### CSI-RS, SRS, and Control Frames

**Downlink CSI-RS**: `NtnPhyGnb` creates CSI-RS as `NtnAirFrame` and records attached UE IDs. Satellite feeder `NtnPhyBase` evaluates the feeder hop and stores it in `relayHopSinr[]`; satellite service `NtnPhyBase` then duplicates one frame per attached UE, and `NtnAirFrame::dup()` deep-copies the stored measurement. At the UE, `LteDlFeedbackGenerator::handleCsiReferenceSignal()` calls `computeReceptionSinr()` on the model returned by `NtnPhyUe::getReceptionChannelModel()`, so downlink CSI is combined over both hops. Downlink data decoding reaches the same method through `isReceptionSuccessful()`.

Since commit `1aa1083d` this is enforced rather than merely arranged: `computeReceptionSinr()` throws if a frame arrives without a first-hop measurement instead of silently degrading to the last hop, and `NtnPhyBase::handleAirFrame()` throws instead of relaying a frame it has no channel model to evaluate. D2D is delegated to the base implementation, as those links never transit the satellite.

`NtnChannelModel::getRSRP()` itself remains a single-hop primitive: it still reports only the hop described by the `UserControlInfo` it is given, the same as `getSINR()`. It now has a two-hop sibling, `computeReceptionRsrp()`, that combines it with a stored first hop the same way `computeReceptionSinr()` combines `getSINR()`; see the "Downlink Beacons" paragraph below and the note under Known Physical Inconsistencies.

**Uplink SRS**: Satellite service `NtnPhyBase` measures SRS on the first hop and stores SINR in `relayHopSinr[]`. Gateway feeder `NtnPhyBase` combines both hops using `computeReceptionSinr()`, storing the result in `endToEndSinr[]`. `NtnPhyGnb::handleSrsReferenceSignal()` reads `endToEndSinr[]` and passes it to `LteUlFeedbackGenerator::computeUlFeedback()`, which derives the uplink CQI from the combined two-hop measurement rather than re-measuring a non-existent terrestrial link. This closes the uplink CSI measurement loop through the transparent path.

**Downlink beacons**: `LtePhyEnb::createBeaconMessage()` builds a `BEACONPKT` with no `destId` (ordinary eNBs deliver it as a physical broadcast). `NtnPhyGnb::sendBroadcast()` gives it the same attached-UE fan-out treatment as CSI-RS instead, populating `attachedUes[]` from `cellInfo_->getAttachedUes()` before forwarding it over the NTN path. Satellite feeder `NtnPhyBase` measures the first hop and stores it in `relayHopRsrp[]` (`NtnPhyBase::getHopAction()` treats `BEACONPKT` like `CSIRSPKT`: it must arrive on the feeder link). Satellite service `NtnPhyBase` then duplicates one frame per attached UE via the same `CSIRSPKT`/`BEACONPKT` fan-out branch in `handleUpperMessage()`, and `NtnAirFrame::dup()` deep-copies `relayHopRsrp[]` along with the other stored measurements. At the UE, `NtnPhyUe::computeReceivedBeaconPacketRssi()` calls `computeReceptionRsrp()` on the model returned by `getReceptionChannelModel()` when the frame arrived via a transparent NTN gNB, combining both hops with the same cascaded-hop harmonic-mean approximation `computeReceptionSinr()` uses for SINR (RSRP is a power, not a ratio, so this is an explicit approximation pending the transponder gain/noise model in item 1). `computeReceivedBeaconPacketRssi()` is only ever invoked from `HandoverController::beaconReceived()`, itself gated behind `enableHandover`, so this fixes the measurement without enabling or implementing handover switching. `LtePhyUe::findCandidateEnb()` (dynamic cell search, gated by `dynamicCellAssociation`) is unchanged and still evaluates a fictitious direct gNB-UE geometry through the terrestrial model; it remains the blocker for enabling NTN handover (see item 5 and the note under Known Physical Inconsistencies).

**Other control frames**: Data packets, SRS, and beacons receive explicit channel evaluation in `NtnPhyBase::handleAirFrame()`. Remaining control frames (grants, HARQ feedback, random access) are relayed without a per-hop failure decision, and this is intentionally out of scope (see Minimum Work item 4 below).

## Geographic and Orbital Model

### Reference Frames

`src/simu5g/mobility/georeference/GeographicReferenceSystem.{ned,cc,h}` anchors the simulation to WGS84 using GeographicLib. The local frame convention is:

- OMNeT++ `x`: east;
- OMNeT++ `y`: negative north;
- OMNeT++ `z`: up.

`src/simu5g/common/GeoUtils.{cc,h}` converts WGS84 to ECEF and computes elevation. `NtnChannelModel` uses ECEF endpoint distance, which correctly represents satellite slant range and Earth curvature. A negative local elevation produces `-INFINITY` SINR/RSRP.

The model assumes one authoritative `GeographicReferenceSystem` in the network. Access searches recursively and does not disambiguate multiple instances.

The root build links GeographicLib using the relative `GEOLIB` setting in `Makefile`; regenerate `src/Makefile` rather than relying on a machine-specific generated copy.

### GEO Mobility

`GeoSatMobility` extends INET `StationaryMobility`. It converts configured latitude, longitude, and altitude to the local frame once. The default altitude is 35,786 km. This is a fixed Earth-relative point, not an orbital or station-keeping model.

### LEO Mobility

`LeoSatMobility` extends `MovingMobilityBase`. It:

1. parses a TLE;
2. initializes the imported SGP4 implementation with WGS-72;
3. propagates from the TLE epoch using the configured UTC simulation start;
4. converts TEME position/velocity to ITRF;
5. treats ITRF2008 and WGS84 as equivalent for simulation purposes;
6. converts geodetic WGS84 position into the local OMNeT++ frame.

The SGP4 velocity is currently discarded: `LeoSatMobility::move()` sets `lastVelocity` to zero. The channel model only derives speed from the terrestrial endpoint and contains a TODO for moving satellites. LEO satellite Doppler is therefore effectively absent.

`SatelliteInserter` reads strict three-line TLE records and dynamically creates satellite vector entries. It recognizes STARLINK, IRIDIUM, ORBCOMM, SPACEBEE, SPACEBEENZ, ONEWEB, and GLOBALSTAR names. Unknown satellites use the catalog number as the vector index. The supplied unknown sample has catalog number 51472, so the LEO smoke configuration can expand a very large sparse vector to create one satellite. Parsing also lacks robust file-open, checksum, and delimiter validation.

## Channel and Antenna Model

### Implemented Effects

`NtnChannelModel` extends `LteRealisticChannelModel` and reuses its BLER and channel-model infrastructure. `NtnChannelModelTables.h` contains values attributed to 3GPP TR 38.811 v15.4.0.

Implemented effects include:

- ECEF slant range and local-horizon rejection;
- free-space path loss;
- elevation-dependent LOS probability;
- elevation/frequency-dependent clutter loss;
- correlated shadow fading;
- optional building penetration;
- atmospheric absorption;
- ionospheric or tropospheric scintillation;
- simplified frequency-selective clustered fading;
- conducted transmit power from the antenna model of the current radio transmitter;
- off-axis transmit and receive antenna gain;
- receiver feeder/lumped loss, thermal noise, and noise figure.

Terrestrial scenario names are mapped to NTN table categories:

- indoor hotspot and urban microcell: dense urban;
- urban macrocell: urban;
- suburban macrocell: suburban;
- rural macrocell: rural.

The fading implementation samples cluster delays, powers, phases, and Doppler projections, then computes a per-RB complex response. It is a system-level approximation, not a complete spatially consistent NTN model with rays, angular spreads, cross-polarization, cluster evolution, or correlated users.

`NtnPlatformAntennaModel` provides GEO, LEO-600, and LEO-1200 parameter sets based on TR 38.821. `GatewayAntennaModel` and `VSATAntennaModel` use a parabolic circular-aperture pattern. Antennas may use:

- `TRACK_PEER`, which always returns zero off-axis angle;
- `FIXED`, which uses ground azimuth/elevation or satellite off-nadir/azimuth.

The smoke scenario uses the UE's default isotropic NTN antenna, not `VSATAntennaModel`.

### Known Physical Inconsistencies

#### No Transparent-Payload Gain or Added Noise

The gateway and satellite now replace `UserControlInfo::txPower` with the transmitting antenna model's configured `txPower` before each outgoing radio hop. The channel model treats this as conducted power before transmit antenna gain and feeder loss. This is a fixed-output approximation: output power does not depend on received input power. The relay still has no transponder gain, input/output noise, bandwidth, filtering, saturation, output backoff, or nonlinear distortion. `relayDelay` is the only payload parameter.

#### `getSINR()` Is SNR

The method calculates received power plus fading minus thermal noise and noise figure. It does not include allocations from other UEs, neighboring beams/cells, gateways, satellites, background cells, or external cells. Existing inherited interference toggles do not make this an interference-aware calculation.

#### Noise Bandwidth Is Fixed

Noise is always computed over 180 kHz. RB center frequencies account for numerology, but noise bandwidth does not. This is only consistent with 15 kHz subcarrier spacing.

#### Polarization Loss Is Unused

`polarizationMismatchLoss` is declared and read, and antenna polarization is configured, but the loss is never applied.

#### RSRP-Based Cell Selection and Handover Measure a Link That Does Not Exist (beacon RSSI fixed; cell search still open)

**Previously**: `NtnChannelModel::getRSRP()` ignored the frame it was given, so it could not combine the stored relay hop and always reported the last hop alone. Both consumers -- `LtePhyUe::findCandidateEnb()` and `LtePhyUe::computeReceivedBeaconPacketRssi()` -- called `primaryChannelModel_`, the terrestrial model, against a synthesised direct gNodeB-UE geometry, and `NtnPhyUe` overrode neither. This was the same defect class as the original uplink CSI bug fixed in `9b265962`, and it was latent rather than live: `enableHandover`/`enableBeacons` default to false, and a `BEACONPKT` never set `destId`, so it was dropped at the satellite before reaching any UE.

**Now**: Beacon delivery mirrors the CSI-RS fan-out -- `NtnPhyGnb::sendBroadcast()` populates `attachedUes[]` for `BEACONPKT`, and `NtnPhyBase::getHopAction()`/`handleAirFrame()` store the feeder-hop measurement in a new `relayHopRsrp[]` field via `HopAction::STORE_RELAY_HOP_RSRP` -- so a beacon now reaches attached UEs carrying a first-hop measurement, the same way CSI-RS and data already did. `NtnChannelModel::computeReceptionRsrp()` -- a new virtual on `LteChannelModel`, alongside `computeReceptionSinr()`, defaulting to single-hop `getRSRP()` everywhere except `NtnChannelModel` -- combines it with the local hop using the same harmonic-mean-of-linear-power formula `computeReceptionSinr()` uses for SINR. `NtnPhyUe::computeReceivedBeaconPacketRssi()` overrides the base class to call it when the beacon's source gNB has a transparent NTN association, falling back to the terrestrial path otherwise. Like `computeReceptionSinr()`, this harmonic combination is an approximation that assumes a matched-power relay; it is not yet consistent with the transponder gain/noise model described under "No Transparent-Payload Gain or Added Noise" (item 1), which remains unimplemented. `NtnChannelModel::getRSRP()` itself is unchanged and still single-hop -- it remains the correct primitive for measuring one hop at a time, exactly as `getSINR()` does for `computeReceptionSinr()`.

`LtePhyUe::findCandidateEnb()` is unchanged and still calls `primaryChannelModel_` against a synthesised direct gNodeB-UE geometry: cell search/reselection still measures a link that does not exist. Only beacon-driven RSSI -- the input to handover feasibility once `enableHandover` is turned on -- is now two-hop-correct. Handover triggering and switching (`HandoverController::triggerHandover()`/`doHandover()`) are unmodified and remain absent as a decision-making capability for NTN cells (see item 5).

#### SINR Statistics Attribution (fixed in commit 6a9188f9)

**Previously**: `rcvdSinrUl` was emitted toward the UE's terrestrial channel model, but on a frequency-translating NTN path the frame arrives at the gateway on the feeder carrier (27 GHz), for which the UE has no channel model. This caused `rcvdSinrUl` to record as `nan` and could segfault when trying to attribute the measurement.

**Now**: A virtual hook `LteRealisticChannelModel::getSinrStatisticsTarget()` allows `NtnChannelModel` to translate the feeder carrier back to the service carrier and return the UE's NTN channel model for attribution. `rcvdSinrUl` now correctly records the actual satellite path SINR (0.117 dB for the default GeoSat configuration, 16.68 dB with `VSATAntennaModel`).

## Smoke Scenarios and Verification State

The only current examples are under `simulations/nr/ntn_smoke/`:

- `NtnGeo.ned` and `[Config GeoSat]` create one fixed GEO satellite;
- `NtnLeo.ned` and `[Config LeoSat]` dynamically insert one TLE-driven LEO satellite;
- both use one gNB, gateway, UE, and remote server;
- both run bidirectional CBR traffic for two seconds;
- the UE is an `NtnNrUe`;
- gNB/gateway/satellite IDs are fixed to 1/16384/17408;
- local CSI-RS and SRS-based feedback are enabled.

Run from the scenario directory after sourcing the required OMNeT++, INET, and Simu5G environments:

```sh
./run -u Cmdenv -c GeoSat
./run -u Cmdenv -c LeoSat
```

**Current delivery status** (2 s runs, with `scenario = "RURAL_MACROCELL"` and `fixedLos = true` defaults):

| Config | DL delivered | UL delivered | UL CQI |
|---|---|---|---|
| `GeoSat` | 58.5 kB | 56.4 kB | 4 |
| `GeoSat` + `VSATAntennaModel` | 58.5 kB | 56.1 kB | 11 |
| `LeoSat` | 58.5 kB | 56.1 kB | 10 |

Both configurations now deliver in both directions. Uplink CSI derives from the actual satellite path (`9b265962`, with the statistics hook in `6a9188f9`), the link-budget defaults no longer apply NLOS clutter to satellite links (`ebb7a181`), and the feeder hop is evaluated at its own carrier (`94160458`).

`LeoSat` previously delivered nothing, and the cause was geometric rather than a modelling defect: the configured epoch placed the satellite 22.2 degrees below the local horizon of the ground nodes. **A LEO scenario's start time must be chosen inside a pass.** The TLE in `space_Veins-1.txt` describes a 350 km circular orbit at 70 degrees inclination, whose visibility cap spans 18.56 degrees of arc, i.e. 2.6% of the Earth at any instant, so most instants are not usable. The pass schedule for the current ground-node position is recorded in a comment above `wall_clock_sim_start_time_utc` in `omnetpp.ini`. Satellite placement and visibility are now reported at INFO (see Implementation Conventions), so a badly chosen epoch is self-diagnosing.

There are no NTN entries in `tests/fingerprint`, no coordinate/orbit unit tests, and no channel/link-budget regression tests. Existing result files under the smoke scenario reference obsolete topology or parameters and must not be treated as validation of the current source.

## Minimum Work for Credible Bent-Pipe NTN

Complete these items before describing the implementation as a usable bent-pipe model.

### 1. Correct Per-Hop Link State

- ✓ **Per-hop carrier frequency (done)**: `NtnPhyBase::registerChannelModel()` retunes a feeder-link NIC's channel models to the translated carrier via `LteChannelModel::setCarrierFrequency()`, so path loss, clutter and fading band selection, building penetration, atmospheric absorption, scintillation, RB centre frequencies, and Doppler are all computed at the feeder frequency. This works because a channel model instance only ever serves one link; it is retuned at `INITSTAGE_SIMU5G_REGISTRATIONS2`, after any CellInfo carrier registration.
- Preserve the current convention that antenna `txPower` is conducted power before antenna gain and feeder loss, and validate configured values against the intended EIRP.
- Add an explicit transparent-payload model for transponder gain, noise figure/noise temperature, bandwidth, filtering, saturation, output backoff, and optional nonlinearity.
- Define whether the two-hop success calculation models amplify-and-forward, frequency translation, or another transparent payload. Use a formula consistent with that choice.

### 2. Add Propagation Delay

- ✓ **Geometry-derived per-hop delay (done)**: `NtnPropagationDelay` (`src/simu5g/stack/phy/NtnPropagationDelay.{h,cc}`) computes each radio hop's ECEF slant range divided by light speed and supplies it to all four `sendDirect()` sites in `NtnPhyBase` and `NtnPhyUe`. Enabled by default via `useGeometricPropagationDelay`; `transparentPayloadRelay` additionally charges the frame duration once end to end rather than once per hop, as a bent-pipe transponder requires. Measured GEO round-trip delay is 506.57 ms, inside the TR 38.821 transparent band, and LEO is 8.33 ms. Payload processing delay stays separate in `NtnRelay::relayDelay`.
- ✓ **NR timers adapted to long RTT (done)**: RLC, RRC, the RAC/BSR counters and HARQ were all re-dimensioned, recovering GEO downlink from the 9.3 kB the raw delay left it at to 85% of offered load. Adding packet delay alone was never sufficient, and this is where most of the work went.
- ✓ **Geometry-derived round-trip delay service (done)**: `ntnCellRoundTripDelay()` bounds the cell's round trip from the satellite altitude and the cell's own `ntnMinElevation`, reproducing TR 38.821's reference figures exactly. Every NTN timer is now derived from it and rounded to a value TS 38.331 can signal, so there is no orbit label to set or to get wrong. Remaining gap: grant/k2 restructuring (step 6 of `ntn-delay-and-timers.md`), which is what the per-UE `ntnRoundTripDelay()` exists for.
- The wired gNB-gateway fronthaul (`simulations/nr/ntn_smoke/NtnGeo.ned:87`) is still a bare NED connection with zero delay and infinite datarate; it needs a channel, not a module parameter. It is also the one leg the round-trip delay service does not account for, deliberately.

**See `ntn-delay-and-timers.md`** (in this directory) for the full design discussion: the four `sendDirect()` call sites and the two traps around them (frame duration charged per hop, and the channel-less fronthaul connection); a taxonomy separating timers that must scale with RTT from the ones that must not; the 3GPP K_offset/`ntn-Config` framing including whether the value is per-UE and how often it must be refreshed; per-mechanism treatment of HARQ, RAC, BSR, CQI aging, and grant timing; and a staging order with the diagnostics needed to debug each step, plus the measured step-1 results. The timer half of item 4 below is covered there as well.

### 3. Model Relative Motion and Doppler

- Preserve the SGP4-derived Earth-fixed velocity in `LeoSatMobility`.
- Compute radial relative velocity independently for service and feeder links.
- Use current-hop carrier frequency for Doppler.
- Add configured frequency pre-compensation and residual error rather than assuming perfect compensation implicitly.
- Update fading state coherently as satellite geometry changes.

### 4. Make Control and Measurement Paths Consistent

- ✓ **SRS (commit 9b265962, hook 6a9188f9)**: SRS is measured on both hops at the satellite and gateway, combined, and fed to uplink CQI computation via `endToEndSinr[]`. Uplink CSI measurement is consistent with the transparent path.
- ✓ **CSI-RS and two-hop enforcement (commit 1aa1083d)**: downlink CSI and data decoding both combine the two hops through `computeReceptionSinr()`, and a missing first-hop measurement is now an error rather than a silent single-hop fallback.
- ✓ **RSRP-based measurement, beacon RSSI**: beacon delivery to attached UEs is fixed (`NtnPhyGnb::sendBroadcast()` fans `BEACONPKT` out over `attachedUes[]`, and `NtnPhyBase::getHopAction()` stores the feeder-hop measurement in the new `relayHopRsrp[]` field), and `NtnPhyUe::computeReceivedBeaconPacketRssi()` now combines both hops through the new `NtnChannelModel::computeReceptionRsrp()`, using the same harmonic-mean formula as `computeReceptionSinr()`, when the beacon's source gNB is a transparent NTN cell. Cell search (`LtePhyUe::findCandidateEnb()`) and handover triggering/switching remain untouched and out of scope; see the note under Known Physical Inconsistencies.
- ✓ **HARQ, random access, BSR and RLC timers against GEO/LEO RTT (done)**: RLC and RRC timers on the NTN entity types, RAC/BSR counters on `NtnNrMacUe`, and HARQ raised to 32 processes with downlink feedback disabled. All values derived from TR 38.821 and TS 38.331; see `ntn-delay-and-timers.md` for the derivations and measurements. Note Simu5G models no Scheduling Request at all — a backlogged UE fires a RACH preamble instead — so `raResponseWindow` carries the RAR offset, the RAR window and `sr-ProhibitTimer` at once.
- Model timing advance/common timing reference and NTN assistance data assumptions explicitly. **Still open, and it now matters**: with no timing advance, the gNodeB's preamble-collision window is a fixed one-slot bucket, so under differential delay two UEs transmitting in the same slot arrive slots apart and never collide, while UEs transmitting slots apart can collide spuriously. Invisible with one UE; wrong with many.

### 5. Add Coverage, Beams, and Dynamic Associations

- Represent spot beams, footprints, beam IDs, steering, frequency/polarization reuse, and beam-specific interference.
- Select only visible satellites above a configurable minimum elevation.
- Replace fixed gNB/gateway/satellite tuples with time-varying associations.
- Support service-link satellite/beam handover and feeder-link gateway handover.
- Remove the current one-gateway-to-one-satellite and one-satellite-to-one-gateway lookup restrictions where the target topology requires it.
- Define behavior when no route is available instead of failing a Binder lookup.

### 6. Complete Propagation Effects and Interference

- Add co-channel interference from UEs, beams, satellites, and gateways.
- Apply polarization mismatch once per hop.
- Add rain/cloud attenuation and configurable environmental assumptions if Ka-band feeder fidelity is required. The feeder hop now runs at the real Ka carrier, so these are the remaining unmodelled Ka effects.
- Make thermal-noise bandwidth numerology-aware.
- Declare and test all NED parameters read by C++.

### 7. Add Verification Before Extending Scope

At minimum, add focused checks for:

- WGS84/local/ECEF round trips;
- reference SGP4 positions and velocities;
- known GEO/LEO elevation, slant range, and propagation delay;
- below-horizon suppression;
- feeder/service frequency translation in every channel effect;
- actual per-hop transmit power and antenna gain;
- transparent-payload gain/noise and two-hop SINR;
- numerology-dependent noise;
- control-frame loss and long-RTT protocol behavior;
- bidirectional CBR delivery for GEO and LEO;
- dynamic association and loss of visibility;
- deterministic NTN fingerprint scenarios after behavior is validated independently.

Do not accept `.UPDATED` fingerprint files merely because physical-model changes alter event trajectories. Validate geometry, link budget, packet flow, and protocol behavior first.

## Implementation Conventions

Follow the surrounding Simu5G and OMNeT++ style when extending NTN:

- **`EV_DEBUG` and `EV_TRACE` do not exist in release builds.** OMNeT++ sets `COMPILETIME_LOGLEVEL` to `LOGLEVEL_DETAIL` under `NDEBUG`, so those statements are compiled out entirely and no runtime `--cmdenv-log-level` can bring them back. Anything a user needs in order to understand why a scenario produced nothing must be at `EV`/`EV_INFO` or above. This is not hypothetical: the whole orbital subsystem once logged only at `EV_DEBUG`, which made a satellite parked below the horizon indistinguishable from a broken model. Satellite creation (`SatelliteInserter::createSatellite`), initial placement (`GeoSatMobility`, `LeoSatMobility`) and horizon crossings (`NtnChannelModel::reportSatelliteVisibility`) are now reported at INFO. Keep per-frame detail at `EV_DEBUG`, and report state *changes* rather than every evaluation so long runs stay readable.
- Prefer a hard error to a silent degradation on the transparent path. A frame that cannot be evaluated correctly should abort the run where the problem is, not produce a plausible number that is discovered later, or never. Existing examples: `NtnPhyGnb` on a missing `gatewayReceptionResult` or `endToEndSinr`, `NtnPhyBase::getHopAction()` on a reference signal arriving over the wrong link, and `NtnChannelModel::computeReceptionSinr()`/`computeReceptionRsrp()` on a missing first hop.
- Pair C++ behavior with NED declarations; every new `par()` access must have a corresponding NED parameter and scenario configuration where required.
- Use staged `initialize(int stage)` for Binder references, registrations, relationships, PHY setup, and dynamically inserted modules. Do not move setup into constructors.
- Keep logical stack identity (`sourceId`/`destId`) separate from current-hop radio identity unless intentionally redesigning the transparent architecture.
- Use `ModuleRefByPar`/`opp_component_ptr` for OMNeT++ component references that may participate in module lifecycle changes.
- Preserve message ownership: after `send()`, `sendDirect()`, or `sendDelayed()`, do not access the message; delete frames that cannot be forwarded; duplicate fan-out frames explicitly.
- Keep satellite/gateway association state in Binder rather than introducing unrelated global registries.
- Modify `.msg`, `.ned`, and source files, never generated `src/Makefile`, `features.h`, or `*_m.{cc,h}` outputs.
- Keep imported SGP4/TEME code isolated from native Simu5G style and document upstream provenance when modifying it.
- Prefer small NTN-specific subclasses over conditional branches in mature terrestrial paths, as done by `NtnGNodeB`, `NtnIp2Nic`, and `NtnPhyGnb`.
- Preserve terrestrial behavior in non-NTN scenarios; a plain `NrUe` carries no NTN parameter or submodule, and NTN is opted into by module type (`NtnNrUe` -> `NtnNrNicUe` -> `NtnPhyUe`).

## Key Files

Topology and registration:

- `src/simu5g/nodes/NtnGNodeB.ned`
- `src/simu5g/nodes/NtnGateway.ned`
- `src/simu5g/nodes/Satellite.ned`
- `src/simu5g/nodes/NtnRelay.{ned,cc,h}`
- `src/simu5g/stack/ip2nic/NtnIp2Nic.{ned,cc,h}`
- `src/simu5g/common/binder/Binder.{cc,h}`
- `src/simu5g/common/LteCommon.{msg,h,cc}`

Frame routing and channel evaluation:

- `src/simu5g/stack/NtnFronthaulNic.{ned,cc,h}`
- `src/simu5g/stack/NtnFeederLinkNic.ned`
- `src/simu5g/stack/NtnServiceLinkNic.ned`
- `src/simu5g/stack/NtnNrNic.ned`
- `src/simu5g/stack/NtnNrNicUe.ned`
- `src/simu5g/nodes/NtnNrUe.ned`
- `src/simu5g/stack/phy/NtnPhyBase.{ned,cc,h}`
- `src/simu5g/stack/phy/NtnPhyGnb.{ned,cc,h}`
- `src/simu5g/stack/phy/NtnPhyUe.{cc,h,ned}`
- `src/simu5g/stack/phy/packet/NtnAirFrame.msg`
- `src/simu5g/common/LteControlInfo.msg`
- `src/simu5g/stack/phy/channelmodel/NtnChannelModel.{ned,cc,h}`
- `src/simu5g/stack/phy/channelmodel/NtnChannelModelTables.h`

Mobility, coordinates, and antennas:

- `src/simu5g/mobility/georeference/GeographicReferenceSystem.{ned,cc,h}`
- `src/simu5g/common/GeoUtils.{cc,h}`
- `src/simu5g/mobility/satellite/GeoSatMobility.{ned,cc,h}`
- `src/simu5g/mobility/satellite/LeoSatMobility.{ned,cc,h}`
- `src/simu5g/mobility/satellite/SatelliteInserter/`
- `src/simu5g/mobility/satellite/SGP4.{cc,h}`
- `src/simu5g/mobility/satellite/TEME2ITRF.h`
- `src/simu5g/stack/phy/antennamodel/`

Round-trip delay and protocol timers:

- `src/simu5g/common/NtnCommon.{cc,h}` — every NTN free function: the round-trip delay geometry, the RLC-entity helpers, and the TS 38.331 value rounding
- `src/simu5g/stack/rlc/NtnNrRlcAmEntity.ned`, `NtnNrRlcUmEntity.ned`
- `src/simu5g/stack/rlc/am/NtnNrRlcAmTxEntity.{ned,cc,h}`, `NtnNrRlcAmRxEntity.{ned,cc,h}`
- `src/simu5g/stack/rlc/um/NtnNrRlcUmRxEntity.{ned,cc,h}`
- `src/simu5g/stack/mac/NtnNrMacUe.{ned,cc,h}`

Scenarios:

- `simulations/nr/ntn_smoke/NtnGeo.ned`
- `simulations/nr/ntn_smoke/NtnLeo.ned`
- `simulations/nr/ntn_smoke/omnetpp.ini`
- `simulations/nr/ntn_smoke/space_Veins-1.txt`
