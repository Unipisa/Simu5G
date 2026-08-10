# Propagation Delay and NTN-Aware Timers

This document expands item 2 ("Add Propagation Delay") of `ntn-implementation.md`, and the timer half of item 4.

**Status: Part 1 (step 1 of the staging plan) is implemented; Parts 2-6 are still design discussion.** The propagation delay itself now exists — see "Step 1 as built" below for what landed and the measured results. Everything about MAC timers remains unimplemented. File and line references describe the state of the code when written and must be re-checked if the branch evolves.

The short version: adding the delay is mechanical; making the NR stack survive it is not. Simu5G assumes throughout that a frame sent in one TTI is received in the next, and several MAC procedures are timed as counters decremented once per TTI. Those counters conflate "how many of my own slots have passed" with "how long until the peer can possibly answer". Only the second meaning has to grow with propagation delay.

## Part 1 — Adding the Delay

### Where it goes

Four `sendDirect()` call sites passed a propagation delay of zero (line numbers as of before the step-1 commit):

- `src/simu5g/stack/phy/NtnPhyBase.cc:241` (feeder/service hop)
- `src/simu5g/stack/phy/NtnPhyBase.cc:273` (CSI-RS / beacon fan-out)
- `src/simu5g/stack/phy/NtnPhyBase.cc:297` (unicast to UE)
- `src/simu5g/stack/phy/NtnPhyUe.cc:111` (uplink service link)

The range must come from the *actual radio* endpoints — the `radioTransmitterId`/receiver ECEF coordinates already carried in `UserControlInfo` — not from the logical `sourceId`/`destId`, which stay at the gNB and UE by design.

Light-time is computed from the transmitter's position at transmission and the receiver's position at reception, which is what OMNeT++ gives you for free. A one-shot `range / c` ignores the satellite's motion during the hop (~50 m for a LEO 7 ms hop); the iterative light-time solution is not worth it, but the approximation should be documented rather than assumed.

### Trap 1: frame duration is charged per hop

None of the `radioIn` gates declare `deliverImmediately`:

- `src/simu5g/stack/NtnFeederLinkNic.ned:31`
- `src/simu5g/stack/NtnServiceLinkNic.ned:31`
- `src/simu5g/stack/phy/NtnPhyBase.ned:26`

So `sendDirect(frame, delay, duration, gate)` delivers at `t + delay + duration`. Today the downlink path already costs two slot durations (gateway to satellite, satellite to UE) with zero propagation delay. This is invisible only because it is currently the sole source of latency.

A transparent payload is an analog repeater: it does not re-buffer the frame. The physically correct model is

```text
end-to-end latency = d1/c + d2/c + relayDelay      (frame duration counted once)
```

not one frame duration per hop. Adding real propagation delay without fixing this silently turns the bent pipe into a store-and-forward relay.

### Trap 2: the fronthaul has no channel

`simulations/nr/ntn_smoke/NtnGeo.ned:87` wires `gnb.ntn <--> ntnGateway.gnbLink` as a bare NED connection: zero delay, infinite datarate. A real gateway is hundreds of kilometres from the gNB. That belongs in a `DatarateChannel` on the connection, not in `NtnRelay::relayDelay`, which must stay payload processing delay per item 2's own rule.

### Verification targets

Reference round-trip delays from TR 38.821:

| Configuration | Max RTD | Min RTD |
|---|---|---|
| GEO, transparent | ~541.5 ms | ~477.5 ms |
| GEO, regenerative | ~270.7 ms | ~238.7 ms |
| LEO 1200, transparent | ~41.8 ms | ~16 ms |
| LEO 600, transparent | ~25.8 ms | ~8 ms |
| LEO 600, regenerative | ~12.9 ms | ~4 ms |

The branch is transparent, so the two-hop rows apply. Note that the smoke LEO TLE (`space_Veins-1.txt`) describes a 350 km orbit, which lands below the LEO 600 row and is therefore a weak stress case. **GEO is the scenario that actually exercises the timer work.**

### Step 1 as built

`src/simu5g/stack/phy/NtnPropagationDelay.{h,cc}` holds the shared logic. `NtnPhyBase` derives from `ChannelAccess` and `NtnPhyUe` from `NrPhyUe`, so there is no common base to hang it on; both PHYs hold one instance **by value** and configure it from their own NED parameters.

Two details worth knowing before changing it:

- **The receiver's position comes from `targetGate->getPathEndGate()->getOwnerModule()`**, cast to `ChannelAccess`. That is the receiving PHY, and its `getRadioPosition()` is exactly what the receiving channel model will use — so delay and path loss share one geometry by construction. It is deliberately *not* routed through the receiver's `mobility` submodule: on a `MovingMobilityBase`, `getCurrentPosition()` also advances that module's state and emits `mobilityStateChangedSignal`, which a transmitter has no business triggering from inside its send path.
- **Trap 1 is fixed behind its own flag.** `transparentPayloadRelay` charges the frame duration only on hops where the satellite is the *transmitter*, so a two-hop path costs `d1/c + d2/c + duration` rather than `+ 2 x duration`.

Four NED parameters, declared with identical names in both `NtnPhyBase.ned` and `NtnPhyUe.ned` so one `**.` line reaches all four radios: `useGeometricPropagationDelay` (default true), `transparentPayloadRelay` (default true), `maxHopPropagationDelay` (default 150ms), `propagationDelayReportThreshold` (default 0.1ms). Two statistics per PHY: `ntnHopPropagationDelay`, `ntnHopSlantRange`.

Each hop is reported once at `EV_INFO` and again only when its delay moves by more than the threshold, mirroring `NtnChannelModel::reportSatelliteVisibility()`. `GeoSat` therefore emits exactly four lines for a whole run:

```text
NtnPropagationDelay::reportHop - radio hop 16384 -> 17408: slantRange[37966.3km], oneWayDelay[126.642ms], elevation[37.5398deg]
NtnPropagationDelay::reportHop - radio hop 17408 -> 2049:  slantRange[37966.1km], oneWayDelay[126.641ms], elevation[37.5412deg]
```

### Measured results

Geometry, verified against an independent WGS84 computation from the configured lat/lon/alt (agrees to the metre) and against the elevations `reportSatelliteVisibility` computes by a different route:

| Scenario | Hop slant range | One-way per hop | Four-hop RTD | TR 38.821 band |
|---|---|---|---|---|
| `GeoSat` gateway↔sat | 37 966.260 km | 126.6418 ms | **506.57 ms** | 477.5–541.5 ms ✓ |
| `GeoSat` UE↔sat | 37 966.148 km | 126.6414 ms | | |
| `LeoSat` (350 km, pass peak) | 624.3 km | 2.0825 ms | **8.33 ms** | below LEO-600 row, as expected |

Delivery over the standard 2 s smoke run:

| Configuration | DL | UL | DL app delay (mean) | UL app delay (mean) |
|---|---|---|---|---|
| both flags off (baseline) | 58.5 kB | 56.4 kB | 6.29 ms | 28.77 ms |
| cut-through only | 58.5 kB | 56.7 kB | 5.22 ms | 24.05 ms |
| **delay on (default)** | **9.3 kB** | **2.1 kB** | 778 ms | 892 ms |
| `LeoSat`, delay on | 58.5 kB | 56.1 kB | 10.4 ms | 36.9 ms |

Four things to read from this table:

1. **The refactor is behaviour-neutral.** With both flags off the run reproduces the previously recorded 58.5 kB / 56.4 kB exactly.
2. **Cut-through removes exactly one slot** from the downlink (6.29 → 5.22 ms) with delivery unchanged, confirming the flag does only what it claims. The larger uplink drop compounds through the grant loop.
3. **GEO collapses, and that is the expected step-1 output**, not a regression: 58.5 → 9.3 kB downlink, 56.4 → 2.1 kB uplink. Minimum observed app-level delay is 269 ms (DL) and 258 ms (UL), both just above the 253.3 ms one-way propagation floor, so the delay itself is being applied correctly and the loss is a MAC-layer stall on top of it.
4. **`LeoSat` is essentially undamaged** at an 8.3 ms RTD. This is the sharpest available evidence that the collapse is genuine RTT physics rather than a defect in the delay code: the same code path, the same flags, two orders of magnitude difference in RTD, and only the long one breaks.

Terrestrial scenarios are structurally unaffected — the only NED types referencing `NtnPhyBase`/`NtnPhyUe` are `NtnFeederLinkNic`, `NtnServiceLinkNic` and `NtnNrNicUe`, no fingerprint test instantiates any of them, and `simulations/nr/standalone` still runs clean.

**Recovering the pre-delay baseline** is one ini stanza:

```ini
**.useGeometricPropagationDelay = false
**.transparentPayloadRelay = false
```

### Notes for whoever does step 3

- `simulations/nr/ntn_smoke/` has **no `run` wrapper**, unlike `simulations/nr/standalone/`. Invoke `simu5g -u Cmdenv -c GeoSat omnetpp.ini` directly; the `./run` line in `ntn-implementation.md` does not work from that directory.
- The 2 s `sim-time-limit` is only four GEO round trips and yields no meaningful throughput number. Add a config inheriting `GeoSat` with a longer limit rather than editing `GeoSat`, so the baseline above stays comparable.
- For LEO delay runs, drop `*.leo*[*].mobility.updateInterval` from its 1 s default to 100 ms: at 1 s the cached satellite position is up to ~7.5 km stale, i.e. ~25 µs of delay error, which would otherwise dominate the modelling error.
- `ue[0].cellularNic.phy` (the LTE-side PHY, also an `NtnPhyUe`) records `nan` for both statistics because it never transmits over the NTN path. Harmless, but do not read it as a missing measurement.

## Part 2 — A Taxonomy of Simu5G Timers

The useful distinction is not "counter versus `cMessage`". It is **local processing budget versus waiting for a peer's response.** Simu5G has three kinds of timing and only one is broken by long RTT.

### Class 1: `cMessage` timers already expressed in seconds

RLC AM `tReordering`, `tStatusProhibit`, `tPollRetransmit` (`src/simu5g/stack/rlc/am/`), PDCP timers, `HandoverController`. These need new *values*, not new code.

### Class 2: wall-clock comparisons against slot duration

`LteHarqProcessRx::isEvaluated()` at `src/simu5g/stack/mac/buffer/harq/LteHarqProcessRx.cc:78`:

```cpp
if ((NOW - rxTime_.at(cw)) >= slotDuration * (harqFbEvaluationTimer_ - 1))
```

This encodes decode/processing time (3GPP's k1). It must **not** scale with propagation delay. Leave it alone. Class 2 is already correct.

### Class 3: per-TTI decrement counters — the actual problem

- `raRespTimer_`, `racBackoffTimer_`, `bsrRtxTimer_` — `src/simu5g/stack/mac/LteMacUe.cc:816-880` (`checkRAC()`)
- `expirationCounter_`, `periodCounter_` — `src/simu5g/stack/mac/LteMacUe.cc:620-640`
- `numerologyPeriodCounter_` — `src/simu5g/stack/mac/LteMacBase.cc:381`
- the implicit `currentHarq_` cycling and HARQ process occupancy

Within class 3, only the response-waits change. Periodic-grant counters count the UE's own slots and stay as they are.

Converting class 3 to `cMessage` timers wholesale would be a large refactor that buys nothing: a counter decremented once per slot is a perfectly good timer as long as it counts the right number of slots.

## Part 3 — K_offset in the 3GPP Specifications

### What it solves

The problem is causality, not latency. In terrestrial NR the UE's uplink frame timing is the downlink timing advanced by TA, and every scheduling relation is written against that grid: DCI in slot *n* schedules PUSCH in *n + K2*, PDSCH in *n* is acknowledged in *n + K1*. Those offsets assume a TA of tens of microseconds.

Under NTN the UE applies a TA of up to ~541 ms. Its uplink slot *n + K2* then occurs, in wall-clock terms, before the gNB's downlink slot *n* has finished transmitting: the UE would have to answer before receiving the grant. K_offset restores causality by turning every DL-to-UL timing relation into

```text
n + K + K_offset
```

expressed in slots of a reference numerology. TS 38.213 applies it to DCI-scheduled PUSCH, RAR-grant-scheduled Msg3, HARQ-ACK on PUCCH, aperiodic SRS triggering, CSI reference-resource timing, and configured-grant/SPS activation.

**The property that governs implementation: K_offset must be an upper bound, not an estimate.** Overestimating costs latency; underestimating breaks the protocol. This is why one conservative broadcast value per cell is sufficient.

`k_mac` is the downlink-direction companion — the delay between a MAC CE being acknowledged and its configuration taking effect. It is signalled only when the network does *not* compensate the feeder-link delay, i.e. exactly the transparent-payload case with the reference point at the gNB. **That is this branch's architecture, so the k_mac-shaped problem applies here too.**

### Where it is signalled

All of it sits in `ntn-Config`, broadcast in SIB19 and mirrored in dedicated signalling:

| Field | Meaning |
|---|---|
| `cellSpecificKoffset` | the K_offset; range dimensioned to cover GEO worst case at the reference SCS |
| `kmac` | downlink-direction offset (above) |
| `ta-Common`, `ta-CommonDrift`, `ta-CommonDriftVariation` | polynomial coefficients for the common (feeder-link + reference-point) delay |
| `ephemerisInfo` | satellite state vectors or orbital elements |
| `epochTime` | reference time those coefficients are valid from |
| `ntn-UlSyncValidityDuration` | how long they stay valid before re-acquisition |

Rel-17 also mandates GNSS in the UE. That is the architectural pivot: the UE computes its own service-link delay from its known position plus the ephemeris, adds the broadcast common TA, and applies full open-loop TA autonomously. Closed-loop TA commands are a correction on top, not the primary mechanism.

Separately, TS 38.321 delays the start of `ra-ResponseWindow` and `ra-ContentionResolutionTimer` by the UE's computed RTD, and Rel-17 extended the value ranges of several MAC timers so they can be configured large enough for GEO.

### Is it per-UE?

Both, deliberately, and the split is worth copying.

**Cell-specific** (`cellSpecificKoffset`, broadcast) covers the worst case anywhere in the cell — largest footprint, lowest elevation, plus the feeder link. It has to, because it is used by UEs the gNB does not yet know about. **Initial access necessarily runs on the conservative broadcast value**, since no per-UE value can exist before attach.

**UE-specific** refinement arrives by MAC CE once the gNB knows the UE's actual delay. The saving is the differential delay across the beam — order 10 ms for a large GEO beam, a few ms for LEO. Real but second-order, which is why the UE-specific path is optional.

For Simu5G the faithful and cheap split is **cell-worst-case for RAC, per-UE for everything after attach.** Exact geometry is free from the Binder, but using the per-UE value for RACH models a UE that knew its own delay before connecting — precisely the assumption the spec refuses to make.

### Does it need periodic updating?

The "upper bound, not estimate" property answers this.

**GEO with fixed ground nodes: never.** The 477-541 ms spread is spatial (which UE, which elevation), not temporal. Compute once at first use and cache it. Station-keeping motion is irrelevant at slot granularity.

**LEO: yes, but slowly.** Service-link range rate peaks near the horizon at roughly `v · Re/rs`, about 6.9 km/s for a 600 km orbit, so one-way delay drifts at ~23 µs/s and a two-hop RTD at up to ~0.1 ms/s worst case. Across a pass the RTD swings from about 8 ms at zenith to ~26 ms near the horizon. At µ=1 (0.5 ms slots) that crosses a slot boundary every ~5 s at worst.

So:

- **TA must be accurate and continuously updated** — hence mandatory ephemeris, GNSS, and `ta-CommonDrift`.
- **K_offset must merely be sufficient** — recompute rarely, round *up*, and add a margin covering the maximum drift over one refresh interval.

A 1 s refresh with a margin of a couple of slots is comfortably safe for LEO and costs nothing. `ntn-UlSyncValidityDuration` is the spec's version of that bound and maps naturally to a NED parameter, with a hard error if the geometry moved further than the margin between refreshes (see the "hard error over silent degradation" convention in `ntn-implementation.md`).

One detail worth taking from real UEs: **latch the value when a procedure starts.** The RAR window length is fixed when the preamble is sent, not re-evaluated each slot. Lazy recomputation on every read is exact and free in a simulator, but lets a counter shrink under a procedure already in flight.

## Part 4 — The Mechanism to Introduce in Simu5G

Simu5G has no K1/K2/K_offset in the spec sense: there is no explicit DL-to-UL timing relation to shift. The UE transmits when it holds a grant; the HARQ RX evaluates after a wall-clock interval; RAC counts down its own slots.

So what is needed is not K_offset but **the RTD expressed in slots, used as an additive term on response-wait durations.** Name it accordingly in code (`ntnRtdSlots_`, not `koffset_`) so nobody later assumes it carries the spec's semantics.

Source it from the Binder, alongside the existing NTN associations:

```cpp
Binder::getNtnRoundTripDelay(gnbId, ueId) -> simtime_t   // same ECEF geometry as the delay itself
Binder::getNtnCellRoundTripDelay(gnbId)   -> simtime_t   // worst case in cell, for pre-attach use
```

This is the simulation analogue of broadcast assistance data, consistent with the branch's existing assumption of perfect Doppler compensation. Document it as such.

**Make every use additive and default to zero**, so terrestrial behaviour is bit-identical and the fingerprint suite does not move.

## Part 5 — Per-Mechanism Treatment

### HARQ — the real fight

Two separate failure modes.

**Correctness.** `src/simu5g/stack/mac/buffer/harq/LteHarqProcessRx.cc:45` throws `"New data arriving in busy HARQ process"` when a process is reused before its feedback returns. With GEO RTD ~540 ms and 5 processes at 0.5 ms slots this fires almost immediately.

**Throughput.** Both directions use `firstAvailable()` (`src/simu5g/stack/mac/NrMacUe.cc:523` and the gNB DL path), so nothing is sent when all processes are busy. That is N-process stop-and-wait with a ceiling of `N · TBS / RTT` — about 9 transport blocks per second for GEO. The scenario flatlines rather than crashing.

Raising `harqProcesses` does not rescue this: keeping a GEO pipe full at 0.5 ms slots would need ~1080 processes, and Rel-17 caps at 32. That gap is exactly why 3GPP's answer for NTN is **HARQ feedback disabling** (`downlinkHARQ-FeedbackDisabled`), pushing reliability onto RLC ARQ. Model both:

- Raise `harqProcesses` (already a NED parameter; `NrMacGnb.ned:26` and `NrMacUe.ned:25` default to 5).
- Add a feedback-disabled mode: the TX unit never enters `TXHARQ_PDU_WAITING` and completes at transmission, optionally with blind repetitions reusing `maxHarqRtx`; the RX side skips `createFeedback()`; the gNB suppresses `signalProcessForRtx`. Confined to `LteHarqUnitTx::extractPdu()`/`pduFeedback()` and `LteHarqProcessRx::createFeedback()`.
- Reliability then rests on RLC AM, whose timers must exceed the RTT — with a startup assertion that `tPollRetransmit > rtd`, because getting that wrong degrades silently.

Two pieces of dead or misleading code to remove rather than leave as traps:

- `currentHarq_ = harqProcesses_ - 2` at `src/simu5g/stack/mac/LteMacUe.cc:660`, commented "the eNB will receive the first PDU in 2 TTIs". `NrMacUe` uses `firstAvailable()`, so it is largely vestigial in NR, but it is a hard-coded two-slot assumption.
- `#define HARQ_TX_INTERVAL 7 * TTI` at `src/simu5g/common/LteCommon.h:301` — defined, never used anywhere in the tree.

### Random access

`raRespTimer_ = raRespWinStart_` (default 3 slots, `LteMacUe.ned:35`) decremented once per TTI in `checkRAC()`. Without the RTD term the UE fires hundreds of preambles before a response can physically arrive, exhausts `maxRacTryouts_`, and produces a RAC storm plus spurious preamble collisions at the gNB. Use the **cell-worst-case** RTD here, per Part 3.

### Buffer status reporting

`bsrRtxTimer_` from `retxBsrTimer` (default 40 slots, `LteMacUe.ned:36`) — same additive treatment, or the UE re-requests before a grant can physically arrive and generates grant storms.

### Periodic grants

`expirationCounter_`/`periodCounter_` count the UE's own slots and are about its own transmission cadence, not a response wait. Leave them in slots. But the grant's *phase* relative to the gNB's intent shifts by the one-way delay: if that matters, the grant should carry an absolute activation time rather than starting on receipt. This is the k_mac-shaped problem in Part 3.

### CQI aging

`src/simu5g/stack/phy/feedback/LteDlFeedbackGenerator.cc:36-37` computes `fbPeriod_`/`fbDelay_` as `int(par(...)) * TTI` — already numerology-blind, and geometry-independent. With ~240 ms one-way, the gNB schedules on roughly 480 ms stale CSI. Tolerable for GEO, not for LEO 600 where elevation and Doppler change fast. Minimum: timestamp the feedback so the AMC knows its age, and state that outer-loop link adaptation is required. Overlaps item 3.

### Uplink interference bookkeeping

`Binder::storeUlTransmissionMap()` keeps a two-slot `CURR_TTI`/`PREV_TTI` window (`src/simu5g/common/binder/Binder.cc:684`), assuming a UL transmission is heard in the same or previous slot. Under NTN delays that window is meaningless. NTN interference is item 6 and unimplemented, so per the branch convention this should abort rather than quietly return the wrong neighbours.

### Grant and resource timing

The gNB allocates RBs for "this TTI" while the UE transmits an RTT/2 later. Harmless with one UE and no interference; wrong as soon as items 5 and 6 land. The correct fix is the 3GPP one: the grant carries an absolute valid slot (k2 + K_offset), the UE holds it until then, and the gNB books resources in a future-slot allocation map. This is the largest structural change here — stage it last, behind a flag.

### Frame alignment and timing advance

Simu5G aligns every node's TTI tick to absolute simulation time with no TA at all. Two options:

- **(A) Ideal TA.** Grids stay aligned in absolute time; propagation delay contributes latency but not misalignment. Simplest, matches the perfect-assistance assumption, consistent with the branch's existing perfect-Doppler-compensation assumption.
- **(B) Explicit TA.** The UE advances its transmission by `2·d_service/c` plus common TA. Needed only to study TA acquisition, TA error, differential delay across a beam, or contention.

Do (A) first with an explicit note. Defer (B) until beams exist (item 5), since differential delay is a beam-scale phenomenon.

## Part 6 — Where the Code Should Live

The branch convention is NTN-specific subclasses (`NtnGNodeB`, `NtnIp2Nic`, `NtnPhyGnb`) rather than conditionals in mature terrestrial paths. But there is currently **no `NtnMacUe`/`NtnMacGnb`**, and MAC is exactly where this work lands.

Add them: thin subclasses of `NrMacUe`/`NrMacGnb` overriding only the counter reload points, with a few members promoted to `protected`/`virtual` in `LteMacUe`. `NtnNrNicUe.ned` and `NtnNrNic.ned` already exist, so the wiring is cheap. This makes the "no effect on terrestrial scenarios" guarantee structural rather than a matter of care.

Guardrails:

- Every change must be a no-op when the RTD term is zero. Prefer additive offsets defaulting to zero over redefining existing NED parameters.
- Per the branch convention, abort if a MAC computes a zero RTD term while its PHY has non-zero propagation delay configured.

## Part 7 — Staging

1. ✓ **Delay only (done).** See "Step 1 as built" in Part 1 for what landed and the measured failure signature: GEO drops from 58.5/56.4 kB to 9.3/2.1 kB while LEO is untouched at an 8.3 ms RTD.
2. Binder RTD service and NED parameters. No behavioural effect yet.
3. RAC and BSR offsets.
4. HARQ: process count, feedback-disabled mode, remove the `-2` initialisation and `HARQ_TX_INTERVAL`.
5. RLC AM timer values, plus the `tPollRetransmit > rtd` assertion.
6. Grant/k2 restructuring, if at all.

## Part 8 — Diagnostics and Verification

**Step 1 is undebuggable without diagnostics that survive release builds.** Per the logging convention in `ntn-implementation.md`, `EV_DEBUG` and `EV_TRACE` are compiled out under `NDEBUG`. The following must be at `EV_INFO` or they do not exist where it matters:

- measured end-to-end RTT per UE;
- HARQ process occupancy and stall counts;
- RAC attempts and outcomes;
- BSR retransmission counts;
- grant-to-transmission latency.

Without these, a GEO smoke run that drops from 58 kB to 0 kB gives no way to distinguish a HARQ stall from a RAC storm from an RLC timeout.

Verification beyond the geometry checks already listed in item 7:

- one-way delay for GEO at nadir and at cell edge, and for the LEO TLE at a known epoch, against the TR 38.821 table above;
- a "delay added, timers untouched" run recorded as the documented failure baseline;
- delivered bytes for GEO and LEO after each staged fix;
- **the analytical check that actually validates the timer work**: with feedback-enabled HARQ, delivered throughput should equal `N_proc · TBS / RTT`. If the simulation does not reproduce that number, the HARQ pipelining is wrong regardless of what the delivery counters say.

Note also that the current 2 s smoke runs are no longer adequate: at a 540 ms RTT that is barely four round trips. GEO needs tens of seconds of simulated time, and LEO must still start inside a pass.
