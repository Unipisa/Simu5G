# Simu5G NTN Implementation

This document describes the non-terrestrial network (NTN) implementation on the `ntn` branch: what is modelled, the assumptions it rests on, and what is still missing. Treat the source files cited below as authoritative if the branch evolves. Propagation delay and protocol timers are covered in more depth in `ntn-delay-and-timers.md`.

## Scope

The branch implements a transparent, frequency-translating (bent-pipe) satellite path for a **system-level** simulator:

```text
downlink: gNB -> wired fronthaul -> gateway -> feeder radio -> satellite -> service radio -> UE
uplink:   UE -> service radio -> satellite -> feeder radio -> gateway -> wired fronthaul -> gNB
```

The satellite and gateway relay PHY airframes. They do not terminate the NR stack: scheduling, HARQ, CSI processing and protocol termination stay at the terrestrial gNB and UE. The gateway computes the final uplink reception result.

The branch builds on `cqi-computation`, whose CSI-RS/SRS-based local feedback it depends on.

### What is modelled

- Dedicated NTN gNB, gateway, satellite, relay, fronthaul, feeder-link and service-link modules, with a static Binder association among one gNB, one gateway and one satellite.
- Bidirectional forwarding of data and control airframes over the transparent path.
- Per-hop channel evaluation, combined end to end under a fixed-output payload model, for data, CSI-RS, SRS and beacons. A frame that reaches its final receiver without a first-hop measurement aborts the run.
- Per-hop carrier frequency: the feeder hop is evaluated at the translated (e.g. Ka-band) carrier.
- Transmit power taken from the antenna model of whichever node radiates the current hop.
- GEO placement and TLE/SGP4-based LEO motion, including the Earth-fixed velocity.
- WGS84, ECEF and local OMNeT++ coordinate conversion through GeographicLib.
- Satellite (TR 38.821), gateway, VSAT and isotropic antenna models.
- A channel model based on TR 38.811 (path loss, clutter, LOS probability, shadowing, building penetration, atmospheric absorption, scintillation, clustered fading).
- Geometry-derived per-hop propagation delay, with the frame duration charged once end to end.
- NTN-aware RLC, RRC, RAC/BSR and HARQ timers derived at runtime from the cell's round-trip delay, and uplink grant timing (K_offset-like). See `ntn-delay-and-timers.md`.
- GEO and LEO smoke scenarios, including 30 s multi-UE GEO configurations.

## Modelling Assumptions

These are deliberate simplifications, acceptable for a system-level simulator. Results should state them; they are not bugs to fix.

| Assumption | What it means in the code | Justification |
|---|---|---|
| **Fixed-output transparent payload** | The relay always radiates its configured `txPower`; end-to-end SINR is `s1·s2/(s1 + s2 + 1)` (see "Transparent Payload") | No gain modes, power sharing among RBs/UEs, passband filtering, HPA saturation, output backoff, AM/AM-AM/PM or intermodulation. The standard system-level abstraction |
| **Perfect Doppler and frequency pre-compensation** | Satellite velocity is not used by the channel; no residual carrier-frequency offset | 3GPP Rel-17 assumption (UE uses GNSS and ephemeris; feeder link compensated by the network). The residual is negligible at system level. The satellite's motion only gives a Doppler shift common to all paths, which leaves per-RB fading magnitude unchanged |
| **Perfect time and frequency synchronisation** | Uplink grants arrive in the slot they were booked for | Grant timing models ideal timing advance once a grant exists; random access is the exception (see "Timing advance" under What Is Missing) |
| **Clear-sky Ka feeder, non-limiting gateway** | No rain or cloud attenuation on the feeder hop | Gateways are dimensioned with margin and site diversity, and a strong feeder hop barely moves the two-hop combination |
| **Simplified clustered fading** | Cluster delays, powers, phases and Doppler projections give a per-RB complex response | No rays, angular spreads, cross-polarisation, cluster evolution or spatial consistency between users. Standard for system-level |
| **Transmit power per RB** | `txPower` is applied to every RB, as in terrestrial Simu5G | Configure it as EIRP density converted to per-RB power, which is how TR 38.821 specifies satellite power |
| **RSRP per RB, not L3-filtered** | As everywhere in Simu5G | Adequate for comparing cells; not for absolute 3GPP thresholds |
| **Static association** | One fixed gNB/gateway/satellite tuple per cell, no beams | Fine for GEO and for LEO runs shorter than a pass |
| **Ideal fronthaul** | The wired gNB-gateway link has zero delay and infinite datarate | Not part of the radio path; see "Fronthaul channel" under What Is Missing |
| **Single, isolated beam** | No interference in either direction (enabling it aborts the run) | Valid for single-beam studies. Multi-beam studies need the interference work under What Is Missing |

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

`GeoSatellite`, `Leo600Satellite` and `Leo1200Satellite` select mobility and platform antenna defaults. They differ in position/orbit and antenna parameters, not in relay behaviour.

`src/simu5g/nodes/NtnRelay.{ned,cc,h}` forwards every message between its left and right NIC gates with `sendDelayed()`. `relayDelay` defaults to zero and represents payload processing delay only, not propagation delay.

`src/simu5g/stack/NtnFeederLinkNic.ned` and `NtnServiceLinkNic.ned` each contain an `NtnPhyBase`, one or more `NtnChannelModel` modules, and an antenna model. The same C++ PHY class handles both sides according to its `linkType` parameter.

Feeder and service links have no static NED wireless connection: `feederLinkRadioIn`/`serviceLinkRadioIn` are `@directIn` gates, and `NtnPhyBase::handleUpperMessage()`/`NtnPhyUe::sendUnicastViaNtn()` resolve the peer through the Binder at runtime and deliver with `sendDirect()`. The only static radio-facing wiring is the gNB-to-gateway fronthaul (`gnb.ntn <--> ntnGateway.gnbLink`). Beam, coverage or handover work therefore changes Binder-driven peer resolution, not NED topology.

### Node IDs and Binder Associations

NTN node ID ranges are reserved in `src/simu5g/common/LteTypes.h`:

- gateways: 16384 through 17407;
- satellites: 17408 through 18431.

`NtnRelay` registers gateway and satellite modules during `INITSTAGE_SIMU5G_REGISTRATIONS` and unregisters them in `finish()`.

`NtnIp2Nic` registers a `GnbNtnAssociation` during `INITSTAGE_SIMU5G_NODE_RELATIONSHIPS`. It retries at simulation time zero because a satellite created by `SatelliteInserter` may finish registration after the gNB's normal relationship stage. All current associations set `isTransparent = true`.

```cpp
struct GnbNtnAssociation {
    MacNodeId gnbId;
    MacNodeId ntnGatewayId;
    MacNodeId satelliteId;
    bool isTransparent;
};
```

The APIs are in `src/simu5g/common/binder/Binder.{h,cc}`. The lookup rules imply:

- each gNB has one active NTN tuple;
- several gNBs may share the same gateway/satellite tuple (which is what a multi-beam satellite would use);
- one gateway cannot map to different satellites, and one satellite cannot map to different gateways;
- associations are static and carry no beam, validity interval, coverage or handover state.

The one-to-one gateway/satellite rules are enforced: `Binder::getAssociatedSatelliteForGateway()` and `getAssociatedGatewayForSatellite()` throw if they find more than one distinct peer. Any dynamic-association or handover work must change these lookups.

Round-trip delay is computed in `common/NtnCommon.{h,cc}`, which reads the associations the Binder holds:

- `ntnCellRoundTripDelay(binder, gnbId)` — the worst case anywhere in the cell, bounded by the cell's own `ntnMinElevation`. Derived once and published onto the association, so the gNB and every UE read the same value. All timers use it.
- `ntnRoundTripDelay(binder, gnbId, ueId)` — the instantaneous delay from the actual UE, satellite and gateway positions, never cached. Used for uplink grant activation times once a UE has been heard from.

`ntnMinElevation` and `ntnMinSatelliteAltitude` are per-cell parameters on `NtnGNodeB`. Both functions return zero for a gNB with no NTN association, which leaves terrestrial scenarios untouched. Positions come from `ChannelAccess::getRadioPosition()`, the same cached snapshot the channel model and `NtnPropagationDelay` use, so delay, path loss and round-trip delay cannot drift apart. Never read them from a mobility module directly: `getCurrentPosition()` advances a `MovingMobilityBase` and emits a signal.

The UE attaches to the logical gNB through the ordinary serving-node relation and discovers the satellite path from the serving gNB's NTN association. The satellite is deliberately not exposed as the logical source of downlink frames.

### NTN Airframe Metadata

`src/simu5g/stack/phy/packet/NtnAirFrame.msg` extends `LteAirFrame` with:

- `relayHopSinr[]`: first radio-hop SINR, one value per band, populated for data, CSI-RS, SRS and beacon frames;
- `attachedUes[]`: CSI-RS and beacon fan-out targets;
- `gatewayReceptionResultValid` and `gatewayReceptionResult`: uplink decoding decision made at the gateway;
- `endToEndSinrValid` and `endToEndSinr[]`: combined two-hop uplink SINR for SRS, stored by the gateway.

`UserControlInfo` in `src/simu5g/common/LteControlInfo.msg` carries the current hop's radio metadata separately from the logical source/destination IDs: radio transmitter and receiver IDs, transmitter local and ECEF coordinates, transmitter antenna model, and current-hop transmit power in dBm. This separation is fundamental to the transparent design: `sourceId` stays the gNB or UE for stack semantics, while `radioTransmitterId` identifies the gateway or satellite currently radiating the frame.

Do not edit generated `*_m.cc` or `*_m.h` files. Change the `.msg` source and regenerate through the normal build.

## Packet Flow

### Downlink Data

1. The normal gNB stack builds a MAC frame.
2. `NtnPhyGnb::createAirFrame()` creates an `NtnAirFrame`.
3. `NtnPhyGnb::sendBroadcast()` and `sendUnicast()` send it over the wired NTN gate rather than terrestrial radio.
4. The gateway fronthaul NIC and relay pass the frame to the feeder PHY.
5. Gateway `NtnPhyBase` adds the feeder frequency offset, records gateway radio metadata and feeder-antenna transmit power, and sends the frame directly to the associated satellite.
6. Satellite feeder `NtnPhyBase` computes the feeder-hop SINR and stores it in `relayHopSinr[]`.
7. The satellite relay passes the frame to the service PHY.
8. Satellite service `NtnPhyBase` subtracts the offset, records satellite radio metadata and service-antenna transmit power, and sends the frame directly to the UE.
9. `NtnPhyUe::getReceptionChannelModel()` selects the NTN channel model from the logical source gNB's association. It infers "this frame arrived via satellite" from the source gNB rather than the radio sender, because `sourceId` stays logical; an in-code note flags this for revisiting if downlink frames ever expose the satellite ID.
10. `NtnChannelModel::computeReceptionSinr()` combines feeder and service SINR as `s1·s2/(s1 + s2 + 1)` before ordinary BLER evaluation.

### Uplink Data

1. `NtnPhyUe::sendUnicast()` checks whether the serving gNB has a transparent NTN association.
2. If so, it converts the `LteAirFrame` into an `NtnAirFrame`, preserving duration, priority, control information and payload.
3. The UE records its transmitter metadata and sends the frame directly to the satellite's service-link input.
4. Satellite service `NtnPhyBase` stores the first-hop SINR in `relayHopSinr[]`.
5. The satellite relay passes the frame to feeder `NtnPhyBase`, which shifts frequency, applies the satellite feeder-antenna transmit power, and sends it directly to the gateway.
6. Gateway feeder `NtnPhyBase` combines both hops, calls `isReceptionSuccessful()`, and stores the result in the frame.
7. For SRS frames, the gateway also stores the combined two-hop SINR in `endToEndSinr[]`.
8. The gateway fronthaul NIC translates the frequency back to the service carrier and forwards the frame to the gNB.
9. `NtnPhyGnb` trusts the gateway's stored reception result instead of recomputing the channel, and throws if a data frame arrives without it.
10. For SRS frames, `NtnPhyGnb::handleSrsReferenceSignal()` passes `endToEndSinr[]` to `LteUlFeedbackGenerator::computeUlFeedback()`, so uplink CQI reflects the actual satellite path. It throws if `endToEndSinr[]` is missing.

### CSI-RS, SRS, Beacons and Other Control Frames

**Downlink CSI-RS**: `NtnPhyGnb` creates CSI-RS as an `NtnAirFrame` listing the attached UEs. The satellite feeder PHY stores the feeder-hop SINR; the service PHY then duplicates one frame per attached UE (`NtnAirFrame::dup()` deep-copies the stored measurement). At the UE, `LteDlFeedbackGenerator::handleCsiReferenceSignal()` calls `computeReceptionSinr()`, so downlink CQI is combined over both hops. Downlink data decoding uses the same method through `isReceptionSuccessful()`.

**Uplink SRS**: measured on the first hop at the satellite, combined at the gateway into `endToEndSinr[]`, and used by the gNB for uplink CQI (see Uplink Data).

**Downlink beacons**: `NtnPhyGnb::sendBroadcast()` gives a `BEACONPKT` the same attached-UE fan-out as CSI-RS, and `NtnPhyBase::getHopAction()` treats it exactly like CSI-RS: the feeder-hop SINR is stored in `relayHopSinr[]`. At the UE, `NtnPhyUe::computeReceivedBeaconPacketRssi()` calls `NtnChannelModel::computeReceptionRsrp()` (see "Transparent Payload" for the formula). Its only consumer is `HandoverController::beaconReceived()`, which computes it only when `enableHandover` is on; radio link failure is driven by RLC retransmissions, not by beacons. So beacon RSRP never affects CQI or throughput.

`NtnChannelModel::getSINR()` and `getRSRP()` are single-hop primitives: each reports only the hop described by the `UserControlInfo` it is given. `computeReceptionSinr()` and `computeReceptionRsrp()` are their two-hop counterparts.

Both two-hop paths are enforced: `computeReceptionSinr()`/`computeReceptionRsrp()` throw if a frame arrives without a first-hop measurement, and `NtnPhyBase::handleAirFrame()` throws instead of relaying a frame it has no channel model to evaluate. D2D is delegated to the base implementation, since those links never transit the satellite.

**Other control frames** (grants, HARQ feedback, random access) are relayed without a per-hop failure decision. This is intentional.

## Geographic and Orbital Model

### Reference Frames

`src/simu5g/mobility/georeference/GeographicReferenceSystem.{ned,cc,h}` anchors the simulation to WGS84 using GeographicLib. The local frame convention is: OMNeT++ `x` = east, `y` = negative north, `z` = up. `omnetFromWgs84()`/`wgs84FromOmnet()` convert positions; `omnetVectorFromEcef()` rotates a free vector (e.g. a velocity) into the local axes.

`src/simu5g/common/GeoUtils.{cc,h}` converts WGS84 to ECEF, computes elevation, and bounds the slant range at a given elevation, all on the WGS84 ellipsoid. `NtnChannelModel` uses the ECEF endpoint distance, which correctly represents slant range and Earth curvature. A negative local elevation gives `-INFINITY` SINR/RSRP.

The model assumes one `GeographicReferenceSystem` in the network; access searches recursively and does not disambiguate multiple instances. The build links GeographicLib through the relative `GEOLIB` setting in `Makefile`.

### GEO Mobility

`GeoSatMobility` extends INET `StationaryMobility`. It converts the configured latitude, longitude and altitude (default 35,786 km) to the local frame once. It is a fixed Earth-relative point, not an orbital or station-keeping model, so its velocity is correctly zero.

### LEO Mobility

`LeoSatMobility` extends `MovingMobilityBase`. At every `updateInterval` it:

1. propagates the TLE with the imported SGP4 implementation (WGS-72) from the TLE epoch, using the configured UTC simulation start;
2. converts TEME position and velocity to ITRF, treating ITRF2008 and WGS84 as equivalent;
3. converts the geodetic position into the local frame, and rotates the Earth-fixed velocity into the same axes.

`getCurrentVelocity()` returns the Earth-fixed velocity in m/s. The imported `TEME_to_ITRF()` is a port of skyfield's and works in **km/day** (its Earth-rotation term uses `theta_dot` in rad/day), while SGP4 returns km/s, so `updateSatellitePosition()` converts on the way in and out rather than modifying the imported code. Position is piecewise constant between ticks; velocity is the instantaneous value at each tick.

The channel does not consume the satellite velocity, by assumption (see Modelling Assumptions). **Do not feed it into the fading `speed`**: the cluster model's Doppler spread comes from motion around the terrestrial endpoint, and a 7.5 km/s speed there would decorrelate the channel within a slot.

`SatelliteInserter` reads three-line TLE records and dynamically creates satellite vector entries. It recognises STARLINK, IRIDIUM, ORBCOMM, SPACEBEE, SPACEBEENZ, ONEWEB and GLOBALSTAR names; unknown satellites use the catalog number as the vector index.

## Channel and Antenna Model

### Implemented Effects

`NtnChannelModel` extends `LteRealisticChannelModel` and reuses its BLER and channel-model infrastructure. `NtnChannelModelTables.h` contains values from 3GPP TR 38.811 v15.4.0. Implemented effects:

- ECEF slant range and local-horizon rejection;
- free-space path loss;
- elevation-dependent LOS probability;
- elevation- and frequency-dependent clutter loss;
- correlated shadow fading;
- optional building penetration;
- atmospheric absorption;
- ionospheric or tropospheric scintillation;
- simplified frequency-selective clustered fading;
- conducted transmit power from the antenna model of the current radio transmitter;
- off-axis transmit and receive antenna gain;
- receiver feeder/lumped loss, thermal noise and noise figure.

Every frequency-dependent term is computed at the current hop's carrier: `NtnPhyBase::registerChannelModel()` retunes a feeder-link NIC's channel models to the translated carrier via `LteChannelModel::setCarrierFrequency()`. This works because a channel model instance only ever serves one link.

Terrestrial scenario names map to NTN table categories: indoor hotspot and urban microcell to dense urban, urban macrocell to urban, suburban macrocell to suburban, rural macrocell to rural. `NTN_OPEN` enforces an outdoor LOS/AWGN channel.

`getSINR()` is an SNR: received power plus fading, minus thermal noise and noise figure. Inherited interference toggles do not make it interference-aware; on a satellite link they abort the run (see "Interference and multiple beams" under What Is Missing).

`LteRealisticChannelModel::getSinrStatisticsTarget()` lets `NtnChannelModel` attribute `rcvdSinrUl`, which is measured at the gateway on the feeder carrier, to the UE's NTN channel model on the service carrier.

### Antennas

`NtnPlatformAntennaModel` provides GEO, LEO-600 and LEO-1200 parameter sets from TR 38.821. `GatewayAntennaModel` and `VSATAntennaModel` use a parabolic circular-aperture pattern. Antennas point either with `TRACK_PEER` (zero off-axis angle) or `FIXED` (ground azimuth/elevation, or satellite off-nadir/azimuth). Antenna `txPower` is conducted power, before antenna gain and feeder loss. The smoke scenarios use the UE's default isotropic antenna.

### Transparent Payload: Fixed Output Power

Before each outgoing radio hop, `NtnPhyBase::setRadioTransmitterInfo()` replaces `UserControlInfo::txPower` with the transmitting antenna model's `txPower`. The relay therefore radiates a fixed power whatever it receives. The gateway is a ground transmitter/receiver, not a transponder, so its fixed `txPower` is simply correct.

A fixed-output relay re-radiates amplified first-hop noise along with the first-hop signal, splitting its power between them in the ratio `s1 : 1`. The second hop's `getSINR()` counts all of `txPower` as signal, so:

```text
SINR (computeReceptionSinr):  s = s1·s2 / (s1 + s2 + 1)
RSRP (computeReceptionRsrp):  RSRP = P_rx,local · s1/(1 + s1) = P_rx,local[dBm] − 10·log10(1 + 1/s1)
```

with `s1` the first-hop SINR and `s2` the second-hop SINR (linear). The harmonic formula `1/(1/s1 + 1/s2)` (Maral-Bousquet, chap. 5) is exact only for a fixed-gain payload; the two agree at high SINR and differ by 1.76 dB at `s1 = s2 = 0 dB`. In the smoke scenarios one hop dominates, so the difference is 0.05-0.1 dB.

The RSRP is the signal share of the received power, which is also what a UE measures, since the re-radiated noise does not correlate with the known reference-signal sequence. It tends to the local RSRP when the feeder hop is strong and to `-inf` when it is lost, and dividing it by the noise at the UE (its own plus the re-radiated share) gives back exactly the SINR formula.

## What Is Missing

Ordered by how much each would change results.

### PHY

**Link-budget calibration against 3GPP.** The link budget has never been compared with TR 38.821's reference link budgets (the CNR tables of §6.1.3.3 for its parameter sets). The default `GeoSat` gives roughly 10 dB downlink and 0 dB uplink; nobody has checked whether that is what TR 38.821 predicts for the configured antennas and powers, nor whether configured `txPower` values match the intended EIRP density. Method: configure one TR 38.821 parameter set, disable fading and shadowing (or use `NTN_OPEN`), and compare per-hop SNR against the table. This is what a reviewer will ask first, and it would be the first NTN regression test.

**LOS state and shadowing are frozen during a LEO pass.** `NtnChannelModel::getAttenuation()` re-draws the LOS/NLOS state only when the **terrestrial endpoint** has moved more than `correlationDistance`, and shadowing is re-drawn only as `time × speed` of the terrestrial endpoint accumulates. A static UE therefore keeps the LOS state and shadowing sample it drew at the start of the pass, although both are elevation-dependent in TR 38.811 and elevation sweeps from the minimum to near 90 degrees and back. Path loss and clutter do follow the geometry. Fix: also re-draw when elevation changes by more than a threshold, e.g. one 10-degree TR 38.811 table bin. Matters for any LEO result over more than a few seconds; invisible in the smoke scenarios, which use `fixedLos = true`, and irrelevant for GEO.

**Interference and multiple beams.** Only needed for multi-beam studies, but there it often dominates: TR 38.821 system-level evaluations use multi-beam layouts with frequency reuse 1 or 3, where inter-beam interference rather than noise limits SINR. Today `NtnChannelModel::initialize()` aborts if `uplinkInterference` or `downlinkInterference` is enabled on a satellite link (`ntnAllowUplinkInterference`/`ntnAllowDownlinkInterference` override the guards), because both inherited mechanisms assume a transmission from one or two slots ago is what is arriving now. Two options:

- *Interference margin (cheap)*: a configurable C/I or I/N per hop, added as a noise-like term before the two-hop combination. Keeps the single-beam architecture and covers "beam at the edge of a reuse-1 layout" studies.
- *Explicit beams*: one gNB per beam on the same satellite (the Binder already allows several gNBs to share a gateway/satellite tuple), with satellite antennas in `FIXED` pointing for per-beam off-nadir and azimuth. Downlink interference then needs the other cells' RB bookings **at transmission time**, not the current ones; since co-located beams share the propagation delay to a given UE, a booking history suffices. Uplink needs `Binder::getUlTransmissionMap()` keyed by **arrival slot at the receiver**, for which the reception slot each grant is booked for is already the key. Do not reach for the allocator's `prevAllocatedRbsPerBand_` by analogy with the terrestrial downlink path: on an NTN cell it holds the allocation for a reception slot still in the future.

**Small cleanups.**

- `polarizationMismatchLoss` is read but never applied (TODO in `NtnChannelModel::initialize()`). Either apply it once per hop or remove the parameter.
- Noise is computed over a fixed 180 kHz, consistent only with numerology 0. This is a Simu5G-wide convention (terrestrial uses a fixed `thermalNoise` too) and every current NTN scenario is at numerology 0; fix it before using μ > 0, e.g. for a Ka-band/FR2 service link.
- `LeoSatMobility::updateSatellitePosition()` double-counts a day once simulation time exceeds 86,400 s: `daysToAdd` is added to the Julian date while `currentWallClockTimeFrac_` already includes the elapsed days. That shifts Earth's rotation angle by about 1 degree per extra day. Only affects runs longer than a day.
- `SatelliteInserter` lacks file-open, checksum and delimiter validation, and an unknown satellite's catalog number becomes its vector index (the sample TLE, 51472, expands a very large sparse vector to create one satellite).

### Beyond the PHY

- **Timing advance.** Without it, the gNB's preamble-collision window is a fixed one-slot bucket: under differential delay, UEs transmitting in the same slot need not collide while UEs slots apart may collide spuriously. In `GeoSatMultiUeLoad` at the default seed this locks one UE out of uplink for the whole 30 s run, so that configuration is meaningful only when runs are compared at equal seeds. Grant timing is ideal TA but does not help, since random access happens before any grant exists. This will show up in multi-UE results before any of the PHY gaps.
- **Coverage, beams and dynamic association.** No beam footprints, visibility-based satellite selection, time-varying associations, service-link or feeder-link handover, or defined behaviour when no route is available. `LtePhyUe::findCandidateEnb()` (cell search) still measures a direct gNB-UE geometry through the terrestrial model, and handover triggering/switching are unmodified. A UE also never stops retrying random access after its satellite sets, since `LteMacUe::macHandleRac()` resets the attempt counter on exhaustion. Needed for LEO runs longer than one pass.
- **Fronthaul channel.** The gNB-gateway link (`gnb.ntn <--> ntnGateway.gnbLink`) needs a `DatarateChannel`, not `NtnRelay::relayDelay`, which must stay payload processing delay.
- **Protocol open items** (CQI aging, LEO timer validation, PDCP discard timer, diagnostics): see the last section of `ntn-delay-and-timers.md`.

### Verification

There are no NTN entries in `tests/fingerprint`, no coordinate or orbit unit tests, and no link-budget regression tests; every result so far was checked by comparing recorded scalars against a baseline rebuild. Worth adding, in order:

- the link-budget calibration above;
- WGS84/local/ECEF round trips, reference SGP4 positions and velocities, known GEO/LEO elevation, slant range and propagation delay, below-horizon suppression;
- feeder/service frequency translation in every channel effect, per-hop transmit power and antenna gain;
- two-hop SINR and RSRP against the fixed-output formulas;
- a LEO scenario across a full pass;
- deterministic NTN fingerprint scenarios, once behaviour is validated independently.

Do not accept `.UPDATED` fingerprint files merely because physical-model changes alter event trajectories. Validate geometry, link budget, packet flow and protocol behaviour first.

## Smoke Scenarios

All examples are under `simulations/nr/ntn_smoke/` (`NtnGeo.ned`, `NtnLeo.ned`, `omnetpp.ini`). There is **no `run` wrapper** in this directory; run from it after sourcing the OMNeT++, INET and Simu5G environments:

```sh
simu5g -u Cmdenv -c GeoSat omnetpp.ini
```

| Config | Content |
|---|---|
| `GeoSat` | One fixed GEO satellite, one gNB, gateway, UE and remote server, bidirectional CBR for 2 s |
| `LeoSat` | Same, with one TLE-driven LEO satellite inserted dynamically |
| `GeoSatAm` | RLC AM, to exercise the NTN-dimensioned AM timers (the default UM path never reaches them) |
| `GeoSatAmTerrestrialTimers` | `GeoSatAm` with terrestrial RLC timers forced back: shows Radio Link Failure, and is the regression test for the post-teardown discard guards |
| `GeoSatMultiUe` | Four UEs, 30 s: the smallest configuration with resource-block contention |
| `GeoSatMultiUeLoad` | Eight UEs, 30 s, past cell capacity. **Not seed-stable** (see Timing advance) |
| `GeoSatSpreadUe` | Four UEs over 450 km, 30 s: one-way delays differ by whole slots, so per-UE grant timing is visible |

All use `NtnNrUe`, fixed gNB/gateway/satellite IDs 1/16384/17408, local CSI-RS and SRS feedback, and the `NtnChannelModel` defaults `scenario = "NTN_RURAL"` and `fixedLos = true`.

Use the 30 s multi-UE configurations for steady-state figures. `GeoSatMultiUe` carries about 105 kB/s uplink and 94 kB/s downlink against roughly 120 kB/s offered each way; downlink application delay stays at 0.26 s, essentially the one-way propagation delay, and the uplink residual is the buffer-status-report and grant round trip. A 2 s run is four GEO round trips, mostly startup, and understates uplink badly.

**A LEO scenario's start time must be chosen inside a pass.** The TLE in `space_Veins-1.txt` describes a 350 km circular orbit at 70 degrees inclination whose visibility cap covers 2.6% of the Earth at any instant, so most start times see the satellite below the horizon and deliver nothing. The pass schedule for the current ground position is recorded in a comment above `wall_clock_sim_start_time_utc` in `omnetpp.ini`. Satellite placement and horizon crossings are reported at INFO, so a badly chosen epoch is self-diagnosing.

Result files already under the scenario's `results/` directory reference obsolete topology or parameters and must not be treated as validation.

## Implementation Conventions

Follow the surrounding Simu5G and OMNeT++ style when extending NTN:

- **`EV_DEBUG` and `EV_TRACE` do not exist in release builds.** OMNeT++ sets `COMPILETIME_LOGLEVEL` to `LOGLEVEL_DETAIL` under `NDEBUG`, so those statements are compiled out and no runtime `--cmdenv-log-level` brings them back. Anything a user needs to understand why a scenario produced nothing must be at `EV`/`EV_INFO` or above. Satellite creation (`SatelliteInserter::createSatellite`), initial placement (`GeoSatMobility`, `LeoSatMobility`) and horizon crossings (`NtnChannelModel::reportSatelliteVisibility`) are reported at INFO. Keep per-frame detail at `EV_DEBUG`, and report state *changes* rather than every evaluation.
- Prefer a hard error to a silent degradation on the transparent path. A frame that cannot be evaluated correctly should abort the run where the problem is. Existing examples: `NtnPhyGnb` on a missing `gatewayReceptionResult` or `endToEndSinr`, `NtnPhyBase::getHopAction()` on a reference signal arriving over the wrong link, `computeReceptionSinr()`/`computeReceptionRsrp()` on a missing first hop, and the interference guards.
- Pair C++ behaviour with NED declarations: every `par()` access needs a NED parameter.
- Use staged `initialize(int stage)` for Binder references, registrations, relationships, PHY setup and dynamically inserted modules. Do not move setup into constructors.
- Keep logical stack identity (`sourceId`/`destId`) separate from current-hop radio identity.
- Use `ModuleRefByPar`/`opp_component_ptr` for component references that may participate in module lifecycle changes.
- Preserve message ownership: after `send()`, `sendDirect()` or `sendDelayed()`, do not access the message; delete frames that cannot be forwarded; duplicate fan-out frames explicitly.
- Keep satellite/gateway association state in the Binder rather than in new global registries.
- Modify `.msg`, `.ned` and source files, never generated `src/Makefile`, `features.h` or `*_m.{cc,h}` outputs.
- Keep imported SGP4/TEME code isolated from native Simu5G style; adapt units and conventions at the call site, and document upstream provenance when modifying it.
- Prefer small NTN-specific subclasses over conditional branches in mature terrestrial paths, as done by `NtnGNodeB`, `NtnIp2Nic` and `NtnPhyGnb`.
- Preserve terrestrial behaviour: a plain `NrUe` carries no NTN parameter or submodule, and NTN is opted into by module type (`NtnNrUe` -> `NtnNrNicUe` -> `NtnPhyUe`).

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

Mobility, coordinates and antennas:

- `src/simu5g/mobility/georeference/GeographicReferenceSystem.{ned,cc,h}`
- `src/simu5g/common/GeoUtils.{cc,h}`
- `src/simu5g/mobility/satellite/GeoSatMobility.{ned,cc,h}`
- `src/simu5g/mobility/satellite/LeoSatMobility.{ned,cc,h}`
- `src/simu5g/mobility/satellite/SatelliteInserter/`
- `src/simu5g/mobility/satellite/SGP4.{cc,h}`
- `src/simu5g/mobility/satellite/TEME2ITRF.h`
- `src/simu5g/stack/phy/antennamodel/`

Round-trip delay and protocol timers:

- `src/simu5g/common/NtnCommon.{cc,h}` — round-trip delay geometry, RLC-entity helpers, TS 38.331 value rounding
- `src/simu5g/stack/phy/NtnPropagationDelay.{h,cc}` — per-hop propagation delay
- `src/simu5g/stack/rlc/NtnNrRlcAmEntity.ned`, `NtnNrRlcUmEntity.ned`
- `src/simu5g/stack/rlc/am/NtnNrRlcAmTxEntity.{ned,cc,h}`, `NtnNrRlcAmRxEntity.{ned,cc,h}`
- `src/simu5g/stack/rlc/um/NtnNrRlcUmRxEntity.{ned,cc,h}`
- `src/simu5g/stack/mac/NtnNrMacUe.{ned,cc,h}` — RAC/BSR counters, grant hold
- `src/simu5g/stack/mac/NtnNrMacGnb.{ned,cc,h}` — uplink grant offset and activation times
- `src/simu5g/stack/mac/scheduler/NtnSchedulerGnbUl.{ned,cc,h}` — one retransmission grant per round trip
- `src/simu5g/common/LteControlInfo.msg` — `grantActivationTime`, `grantIssueTime` (3GPP's K_offset)

Scenarios:

- `simulations/nr/ntn_smoke/NtnGeo.ned`
- `simulations/nr/ntn_smoke/NtnLeo.ned`
- `simulations/nr/ntn_smoke/omnetpp.ini`
- `simulations/nr/ntn_smoke/space_Veins-1.txt`
