# Propagation Delay and NTN-Aware Timers

This document expands item 2 ("Add Propagation Delay") of `ntn-implementation.md`, and the timer half of item 4.

**Status: the staging plan is complete — steps 1 through 6 are implemented.** Propagation delay, the class-1 timers (RLC, RRC), the class-3 RAC/BSR counters, HARQ, the geometry-derived round-trip-delay service and uplink grant timing all now exist — see the "as built" sections in Parts 1, 2, 4 and 5 for what landed and the measured results. `ntnOrbitProfile` is gone: every NTN timer is now derived from the scenario's own geometry. File and line references describe the state of the code when written and must be re-checked if the branch evolves.

**Two figures in this document were wrong and are corrected below.** The uplink delivery shortfall attributed to the grant loop was measured over a 2 s limit, which is four GEO round trips and mostly startup; over 30 s the same path reaches 75% of offered load rather than 25%. And the future-slot ring buffer that Part 5 called "the largest structural change here" was never needed — see "Step 6 as built" in Part 5.

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

### Class 1: `cMessage` timers already expressed in seconds — **done**

RLC AM `t_Reassembly`/`t_Reordering`, `t_StatusProhibit`, `t_PollRetransmit` (`src/simu5g/stack/rlc/am/`), RLC UM `t_Reassembly` (`src/simu5g/stack/rlc/um/`), RRC `t301`. These needed new *values*, not new code — and they now have them. See "Class 1 as built" below for the 3GPP derivation and the measured effect.

### Class 2: wall-clock comparisons against slot duration

`LteHarqProcessRx::isEvaluated()` at `src/simu5g/stack/mac/buffer/harq/LteHarqProcessRx.cc:78`:

```cpp
if ((NOW - rxTime_.at(cw)) >= slotDuration * (harqFbEvaluationTimer_ - 1))
```

This encodes decode/processing time (3GPP's k1). It must **not** scale with propagation delay. Leave it alone. Class 2 is already correct.

### Class 3: per-TTI decrement counters — the actual problem

- `raRespTimer_`, `racBackoffTimer_`, `bsrRtxTimer_` — `src/simu5g/stack/mac/LteMacUe.cc:816-880` (`checkRAC()`) — **done, see "RAC and BSR as built"**
- `expirationCounter_`, `periodCounter_` — `src/simu5g/stack/mac/LteMacUe.cc:620-640`
- `numerologyPeriodCounter_` — `src/simu5g/stack/mac/LteMacBase.cc:381`
- the implicit `currentHarq_` cycling and HARQ process occupancy — still open, step 4

Within class 3, only the response-waits change. Periodic-grant counters count the UE's own slots and stay as they are.

Converting class 3 to `cMessage` timers wholesale would be a large refactor that buys nothing: a counter decremented once per slot is a perfectly good timer as long as it counts the right number of slots.

### Class 1 as built

3GPP publishes no recommended timer *values* — those are network configuration. It publishes three things that together determine them:

- **TR 38.821 §7.2.2.1** gives a formula, `t-Reassembly = RTD * nrof_HARQ_retrans + scheduling_offset`, and states that "No modification of the t-PollRetransmit timer and of the t-statusProhibit timer are needed to support NTN." That sentence is about value *ranges*, not values: the TS 38.331 enumerations already reach ms4000 and ms2400, so only `t-Reassembly` needed the Rel-17 `t-ReassemblyExt` extension to 2200 ms. The configured values still have to change.
- **TR 38.821 §7.2.2.2** gives the reference round-trip delays — GEO transparent **541.46 ms**, LEO transparent **25.77 ms** — the budget `RetransmissionTime = RTD * (maxRetxThreshold+1)`, and "1 or 4 RLC retransmissions are considered here". It tabulates GEO and LEO **separately**, so per-orbit profiles are the report's own structure.
- **TS 38.331** gives the legal enumerations to round to.

With Simu5G's `maxHarqRtx = 3` (4 HARQ transmissions):

| Parameter | Derivation | GEO (RTD 541.46 ms) | LEO smoke (RTD 17.59 ms) |
|---|---|---|---|
| `t_Reassembly` (AM RX, UM RX) | RTD × HARQ transmissions, rounded up | 2166 → **2200 ms** | 70 → **75 ms** |
| `t_PollRetransmit` (AM TX) | smallest 38.331 value > RTD | **800 ms** | **20 ms** |
| `t_StatusProhibit` (AM RX) | < `t_PollRetransmit` − RTD, rounded down | **250 ms** | **2 ms** |
| `maxRtxThreshold` | §7.2.2.2 "1 or 4" | **4** (unchanged) | **4** (unchanged) |
| `t301` (RRC) | 38.331 T301 enumeration, not derived | **2000 ms** | **2000 ms** |

Two independent cross-checks on the GEO column: `t_Reassembly` lands on 2200 ms, exactly the ceiling RAN2 chose for `t-ReassemblyExt-r17`; and `t_PollRetransmit` = 800 ms matches the GEO profile in Amarisoft's production NR-NTN configuration.

The LEO column is what step 2's geometry produces for the 350 km smoke TLE, and it is *shorter* than the hand-set LEO profile this section originally carried (150 / 80 / 50 ms, which were derived for LEO-600 at 25.77 ms). `t_StatusProhibit` coming out at 2 ms is legitimate, not a bug: the relation `< t_PollRetransmit − RTD` only leaves slack when the next legal poll value sits well above the round trip, and the enumeration is dense (5 ms steps) down there where it is sparse up at GEO (ms500 → ms800). It is only non-zero at all because TS 38.331 Rel-16 added `T-StatusProhibit-v1610` = `{ms1, ms2, ms3, ms4}` beneath the base enumeration's floor. A short prohibit timer costs control overhead and nothing else.

**Where it lives.** Two new NED types, `src/simu5g/stack/rlc/NtnNrRlcAmEntity.ned` and `NtnNrRlcUmEntity.ned`, extend the NR entities; `NtnNrNicUe.ned`/`NtnNrNic.ned` redirect `BearerManagement`'s `nrRlc*EntityModuleType` strings to them and set `t301`. Entities are created with the **NIC** as parent (`BearerManagement.cc:372`), not under `bearerManagement`.

*Superseded by step 2:* when this landed, the two compounds carried the timer values themselves as `ntnOrbitProfile`-selected constants. They are now pure `tx.typename`/`rx.typename` redirects to the NTN entity profiles, which derive each value from the cell round-trip delay. The values below are what those derivations produce for GEO — see "Step 2 as built" in Part 4.

**Measured.** UM (what the default configs run) holds or improves: GeoSat DL 4650 → 4950 B/s, LeoSat unchanged. AM is only reachable via the new `[Config GeoSatAm]`, since `Ip2Nic` defaults every class to UM; there, RLF moves from **t = 4.4 s** with terrestrial timers to **t = 25.8 s** with NTN timers, a 5.9x improvement.

**RLF was delayed rather than avoided, until the layers below were fixed — and that vindicated not tuning around it.** When these timers landed, the class-3 counters and the HARQ pool were still unfixed, so MAC stalling pushed the real RLC status round trip past the 800 ms `t_PollRetransmit` was sized for, and RLF still fired at t = 25.8 s. The temptation was to inflate `t_PollRetransmit` to absorb the stall; that would have been tuning to a defect. With steps 3 and 4 done, **`GeoSatAm` no longer reaches RLF at all**: the AM transmit window stops stalling, and a stalled window is the only route by which `RETX_COUNT` accumulates. The spec-derived value needed no adjustment. Exercising the teardown paths now requires forcing a window stall — see the note in `simulations/nr/ntn_smoke/omnetpp.ini`.

Two things deliberately left alone: PDCP (no `discardTimer` is modelled at all, and `NrPdcpRxEntity.timeout` only runs on dual-connectivity split bearers, so it is dead code on a single-leg NTN bearer — a real gap, but not a value problem), and `t311`, whose `0s` default is a scenario choice rather than an NTN property.

**Defect exposed, not caused, by this work — now fixed:** once RLF fires on a satellite link, frames that were in flight keep arriving for a further round-trip time and find the MAC connections already deleted. That used to abort the run, on an unguarded `map::at` in `NrMacUe` on the transmission side and on an empty `connDescIn_` entry fabricated by `macPduUnmake` on the reception side. Both are now discarded gracefully, mirroring how `deleteQueuesRadioLinkFailure` already makes in-flight HARQ feedback inert via `resetHarq_`; `LteMacUe::deleteQueuesRadioLinkFailure` additionally drops the TTI's transmission plan, which referred to the connections just deleted.

The **same defect existed one layer up** and was only found afterwards: `RlcMux::fromMacLayer()` asserted that an arriving PDU has an RX gate index and that a MAC SDU request has a TX buffer, on the reasoning "bearers are established duplex: both sides exist". A teardown unregisters those entities while PDUs are still in flight; in a release build, where the assertion compiles out, the end iterator was dereferenced and the run **segfaulted**. Both lookups are now guarded the same way.

The terrestrial `simulations/nr/rlc` `[Config AM-RLF]` never exercises any of this, because nothing is in flight there. Worth remembering when fixing a teardown path: fixing one layer just moves the crash up to the next, and a release build turns a would-be assertion into a segfault.

**Still open on this path:** a `LteMacSduRequest` already sent to the RLC when the teardown lands reaches `RlcMux::fromMacLayer` after its TX entity was deleted, where `lookupRlcTxBuffer()` returns null and the guarding `ASSERT` is compiled out in release builds (`RlcMux.cc:101-104`). The MAC cannot recall a message it has already sent, so the drop has to happen in `RlcMux`. Not reproducible with the default seed; `-c GeoSatAm --seed-set=3` (or `4`) segfaults on it.

**On mixed LEO/GEO.** Step 2 removed the orbit label, so timer values now follow the serving cell's actual geometry rather than a scenario-wide constant. What still blocks a genuinely mixed constellation is the absence of NTN handover and dynamic association (item 5), not these timers — `Binder::getAssociatedSatelliteForGateway()` throws on a second distinct peer. Note that `BearerManagement` creates entities per (peer, DRB) and re-creates them on teardown, so re-established bearers pick up fresh values at construction; that is how RRC reconfiguration delivers new timer values in a real network, and it is preferable to mutating a live entity.

### RAC and BSR as built

**Simu5G models no Scheduling Request** — no PUCCH, no SR resource, no `sr-ProhibitTimer`. A backlogged UE with no grant fires a RACH preamble instead. So `raResponseWindow` carries three 3GPP roles at once: the RAR-window start offset, the RAR window, and `sr-ProhibitTimer`. Its total necessarily exceeds any legal TS 38.331 `ra-ResponseWindow` value (that enumeration caps at `sl80`) — a property of this model, not a spec-legal configuration. `ra-ContentionResolutionTimer` is not modelled at all.

TR 38.821 §7.2.1.1.1.2 is explicit that the NTN mechanism is **not** a longer window:

> "Introduce an offset for the start of the ra-ResponseWindow for NTN."
>
> "the RAR monitoring duration shall cover at least 2 * maximum differential delay"

so the parameters keep the offset and the window **separate** and sum them only where they are written into the single inherited counter. §7.2.1.3 separately confirms `sr-ProhibitTimer`'s 128 ms cap "is not sufficient" for GEO.

| Parameter | Derivation | GEO (RTD 541.46 ms) | LEO smoke (RTD 17.59 ms) |
|---|---|---|---|
| `ntnRaResponseWindowOffset` | = RTD, no rounding | **541.46 ms** | **17.59 ms** |
| `ntnRaResponseWindow` | 2 × max differential delay, a beam property — **not derived** | **40 ms** | **40 ms** |
| `ntnRetxBsrTimer` | smallest TS 38.331 `retxBSR-Timer` above the RTD | **640 ms** | **20 ms** |
| `ntnRacBackoffMax` | largest TS 38.321 backoff-indicator value ≤ 2 × RTD | **960 ms** | **30 ms** |
| `ntnRacBackoffMin` | — | 0 (unchanged) | 0 (unchanged) |
| `maxRacAttempts` | legal `preambleTransMax` `n10` | 10 (unchanged) | 10 (unchanged) |

The offset and the window are summed into the single inherited counter at the point of use, giving **582 slots** for GEO and **58 slots** for LEO at a 1 ms slot.

`ntnRaResponseWindow` stays a constant rather than becoming geometry-derived because it tracks differential delay across the *beam footprint*, not a path length — that needs the coverage model of item 5. 40 ms is the conservative choice for both orbits, and an over-long window costs nothing on the success path now that `macHandleRac()` clears `raRespTimer_` as soon as the response arrives.

**Values are declared in time and converted to slots against the UE's actual slot duration.** This is why `NtnNrMacUe` is C++ and not NED-only: the inherited counters are *slot counts*, and at µ=0 slots and milliseconds coincide, so a NED-only version would look correct in the smoke scenarios and silently halve every timeout at µ=1. Verified: at µ=0 the counts are 582/640/0..960, at µ=1 they are 1164/1280/0..1920 — exactly doubled, same durations.

*Superseded by step 2:* the conversion originally ran once at `INITSTAGE_SIMU5G_TTI_SETUP`, the first stage where `ttiPeriod_` is known. It now runs in `refreshNtnCounters()`, called from `handleSelfMessage()` and `macHandleRac()` — before every point that latches one of these counters. Two independent reasons force that. The round-trip delay comes from geometry, and no radio holds a valid position until `inet::INITSTAGE_SINGLE_MOBILITY`, which runs *after* every Simu5G stage including `TTI_SETUP`. And a real UE latches these durations when the procedure starts rather than once at configuration — the RAR window length is fixed when the preamble is sent. Still no `checkRAC()` override is needed, since it only ever reads these members.

**Measured**, 2 s runs. GEO preamble count **44 → 2**, with delivery 4950 → 8100 B/s DL and 1050 → 3150 B/s UL. LEO is unaffected as intended (52 preambles, 29250/28050 B/s unchanged). Delivery is still far below the 58.5/56.4 kB of a zero-delay run because the HARQ pool limit — 5 processes against a 507 ms RTD, about 10 transport blocks per second — is step 4 and untouched.

**Caveat on every 2 s uplink figure in this document**, added after step 6 built a 30 s scenario. Two seconds is four GEO round trips and is dominated by attach and the first grant loops, and at a 1.3 s uplink application delay most of what was sent is still in flight when the run ends. The same path over 30 s delivers 75% of offered load where the 2 s run reports about 25%. The 2 s numbers are still valid *as comparisons against each other* — every step measured them the same way — but they must not be read as steady-state throughput. Use `GeoSatMultiUe` for that.

**What deliberately got no offset**, verified so it is not re-investigated:

- **The gNB has no RAC counter.** `pendingRacRequests_` is drained unconditionally at the top of each `LteMacEnb::handleSelfMessage()`, and `signalRac()` feeds `racschedule()` in the *same* TTI. Preamble-to-grant turnaround at the gNB is zero TTIs; all latency is in the air hops. `NtnNrNic.ned` needs no change, and unlike RLC these counters are not a peer protocol — only the UE counts them.
- **The gNB preamble-collision window is a fixed one-TTI bucket with no RTD term**, so under NTN two UEs at different ranges transmitting in the same slot never collide, while UEs transmitting slots apart can falsely collide. That is a timing-advance/alignment gap, not a counter; no offset fixes it.
- **`maxRacTryouts_` bounds nothing.** The exhaustion branch (`LteMacUe.cc:798-806`) resets the counter and zeroes the backoff, so the UE retries immediately — the `//! TODO flush all buffers here` gap. Pre-existing.
- **`LteMacEnb::numPreambles_` is dead** (assigned at `LteMacEnb.cc:133`, never read); only the UE parameter has effect.

**Seam.** `ntnRaResponseWindowOffset` is the only value here that will become per-UE once a scenario has more than one cell, because it tracks that UE's own round trip rather than the cell's worst case. Step 2 deliberately left it on the *cell* value: a per-UE window at random-access time would model a UE that knew its own delay before connecting, which is precisely the assumption the specifications refuse to make. The window tracks differential delay across the beam and stays a cell-level constant, as does the BSR timer. That is the reason for the split.

**Not yet validated on LEO.** Step 2's derived LEO values (17.59 / 20 / 30 ms) differ from the hand-set profile this section originally carried (26 / 320 / 40 ms), and yet `LeoSat` produces byte-identical results. At an 8.3 ms round trip under a 30 kB/s load none of these counters ever binds. They are computed correctly — overriding `ntnRetxBsrTimer` to 50 ms on GEO moves uplink 7650 → 10650 B/s, and `ntnRaResponseWindowOffset` to 5 ms moves delivery 25350/7650 → 15450/14400 B/s, so the path is demonstrably live — but a saturating-load LEO scenario is needed before the LEO column can be said to be exercised at all.

### HARQ as built

Two mechanisms, and TR 38.821 §6.4.2 names both: "increase the number of HARQ processes to match the longer satellite round trip delay to avoid stop-and-wait", or "disable UL HARQ feedback to avoid stop-and-wait … and rely on RLC ARQ for reliability".

Keeping the pipe full needs one process per slot of round trip — ~26 for LEO-600, ~42 for LEO-1200, ~541 for GEO — against a Rel-17 ceiling of 32. **So the process count solves LEO and cannot solve GEO.** Measured, GEO at 32 processes delivers 18450 B/s downlink, and 18450 × 0.507 / 32 ≈ 292 B, one CBR packet per process cycle: still exactly process-limited.

**Feedback disabling is directional, and that turned out to matter more than expected.** TR 38.821 §7.2.1.4 states it twice, separately — "disable uplink HARQ feedback for downlink transmission at the UE receiver", and as a distinct decision "disable HARQ uplink retransmission at the UE transmitter". Measured over GEO:

| | GEO DL | GEO UL | LEO DL | LEO UL |
|---|---|---|---|---|
| feedback on, 32 processes | 18450 | 7650 | 29250 | 28050 |
| **DL off, UL on (the default)** | **25350** | **7650** | 28350 | 28050 |
| both off | 25350 | **1500** | 28350 | 28050 |

Disabling downlink feedback raises GEO downlink 37%; additionally disabling uplink feedback costs 80% of uplink. The reason is that an uplink MAC PDU also carries the **buffer status report**, so losing one with no retransmission stalls the UE for a whole `retxBsrTimer` before the gNodeB learns it has data. Downlink is pure data and only pays the block error rate. Hence the default: `harqFeedbackEnabledDl = false`, `harqFeedbackEnabledUl = true`, both orbits. Uplink therefore stays stop-and-wait, which is why 32 processes still matters — TR 38.821 §6.4.2 "Option 2: Greater than 16 HARQ process IDs with UL HARQ feedback enabled".

LEO is essentially indifferent (28350 vs 29250 downlink, a ~3% cost of losing HARQ error correction), confirming the process count alone is sufficient there. Note the smoke load of 30 kB/s does not stress LEO's pool at all — at an 8.3 ms RTD fewer than one packet is ever in flight — so the LEO comparison needs a saturating-load scenario to be meaningful.

**Two implementation traps, both found by measurement rather than reading:**

- `LteHarqProcessTx` tracks `numEmptyUnits_` *separately* from the unit's own status, and `isEmpty()`/`firstAvailable()` read the counter. Resetting the unit inside `LteHarqUnitTx::extractPdu()` left every process permanently "occupied" and throughput collapsed to a fifth. The completion has to go through the process, which is why it is `LteHarqProcessTx::extractPdu()` that calls `completeWithoutFeedback()` and increments the counter.
- With feedback disabled a process frees on transmission, so `firstAvailable()` — which scans from index 0 — hands out **ACID 0 every slot**. The receiver still needs `harqFbEvaluationTimer` slots to evaluate, so the next PDU aborts the run with "New data arriving in busy HARQ process". `LteHarqBufferTx::firstAvailable()` now advances a round-robin cursor when feedback is disabled, which is what a real scheduler does with the process ID it puts in the DCI. With feedback enabled it still starts at 0, so terrestrial is untouched.

Terrestrial behaviour is provably unchanged: with both flags true, all three modified paths reduce to the original code exactly.

Caveat worth recording: 32 exceeds the 4-bit HARQ process ID field of a real DCI. TR 38.821 §6.4.2 lists several ways round it — slot-number-based IDs, virtual IDs, reuse within the round-trip delay — without converging. Simu5G does not model DCI so the choice is free here, but it is a real constraint in a deployment.

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

A 1 s refresh with a margin of a couple of slots is comfortably safe for LEO and costs nothing. `ntn-UlSyncValidityDuration` is the spec's version of that bound, and step 6 built exactly this — a per-UE cache with a validity window and a drift-margin hard error — before removing it again. See "Step 6 as built" in Part 5: the cache bought staleness risk for no saving, since the computation it was caching is cheap enough to redo on every read.

One detail worth taking from real UEs, and worth being precise about which quantities it actually applies to: **latch the value when a procedure starts**, if the value is fixed by the procedure's own semantics rather than by the cost of computing it. The RAR window length genuinely is fixed when the preamble is sent — TS 38.321 defines it that way, so re-evaluating it mid-procedure would be modelling a different procedure. A UE's round-trip delay for grant timing is not like that: nothing in the procedure fixes it at any particular instant, so recomputing it on every read is not "lazy" in the sense that word usually carries a cost — it is simply more accurate, and the simulator can afford the geometry lookup that a real UE's own TA-acquisition procedure cannot. Latching it anyway, to mimic that real UE constraint, only introduces a staleness window with no corresponding constraint on this side to justify it.

## Part 4 — The Mechanism to Introduce in Simu5G

Simu5G has no K1/K2/K_offset in the spec sense: there is no explicit DL-to-UL timing relation to shift. The UE transmits when it holds a grant; the HARQ RX evaluates after a wall-clock interval; RAC counts down its own slots.

So what is needed is not K_offset but **the RTD expressed in slots, used as an additive term on response-wait durations.** Name it accordingly in code (`ntnRtdSlots_`, not `koffset_`) so nobody later assumes it carries the spec's semantics.

Source it from `common/NtnCommon.{h,cc}`, which reads the NTN associations the Binder holds:

```cpp
ntnRoundTripDelay(binder, gnbId, ueId) -> simtime_t   // same ECEF geometry as the delay itself
ntnCellRoundTripDelay(binder, gnbId)   -> simtime_t   // worst case in cell, for pre-attach use
```

This is the simulation analogue of broadcast assistance data, consistent with the branch's existing assumption of perfect Doppler compensation. Document it as such.

**Make every use additive and default to zero**, so terrestrial behaviour is bit-identical and the fingerprint suite does not move.

### Step 2 as built

**The result that made this cheap.** The standard slant-range bound at a minimum elevation ε,

```text
d(ε, h) = sqrt(Re²·sin²ε + h² + 2·Re·h) − Re·sin ε
```

applied at ε = 10° to *both* the service and the feeder link, reproduces TR 38.821's reference round-trip delays **to the digit** when evaluated on the report's own 6371 km sphere:

| Altitude | Slant range | One-way | RTD (×4) | TR 38.821 |
|---|---|---|---|---|
| 35 786 km (GEO) | 40 581.2 km | 135.3642 ms | **541.46 ms** | 541.46 ms |
| 600 km (LEO-600) | 1 931.6 km | 6.4432 ms | **25.77 ms** | 25.77 ms |
| 350 km (smoke TLE) | 1 303.3 km | 4.3473 ms | **17.39 ms** | — |

The code has since moved to a single Earth model — the WGS84 equatorial radius, the same constant
`ecefFromWgs84()` converts against — so the values it actually produces are 0.01% longer: GEO
**541.52 ms** (40 586.1 km), LEO-600 **25.78 ms**, smoke TLE **17.59 ms**. The equatorial radius
rather than a mean one because the slant range grows monotonically with the Earth radius, so it is
the choice that keeps the result an upper bound at every latitude. **Every rounded timer in the
tables below is identical under either radius**, because they all snap to a TS 38.331 enumeration.
The one unrounded value, `ntnRaResponseWindowOffset` (= RTD), shifts by the same 0.01% and still
converts to the same slot count — 582 at GEO, 58 at LEO. All four smoke configs are byte-identical
on every recorded scalar across the change. The tables are left at the TR 38.821 figures, since
reproducing those is what the derivation is checked against.

So the cell round-trip delay is not a new approximation of the orbit profile — **it is the profile's own derivation, computed instead of transcribed.** That is why every GEO timer came out unchanged. It also means the bound depends only on the satellite's *altitude*, not its position, so for a circular orbit it is constant for the whole run and the LEO drift concern of Part 3 does not arise for the cell value.

**Where it lives.** No new class: free functions in `common/NtnCommon.{h,cc}` — `ntnCellRoundTripDelay()` and `ntnRoundTripDelay()` public, two position helpers and the geometry itself file-static. The pure geometry is `computeSlantRangeAtElevation()` in `GeoUtils`, which along with `ecefFromWgs84()` and `computeElevationFromEcefEndpoints()` is all that file now holds — nine unused functions were deleted and the three survivors moved into `namespace simu5g`. `Binder::getPhyByNodeId()` is unusable for satellites and gateways — it hardcodes `cellularNic` — so the helpers descend through `serviceNic`/`feederNic` from the node modules the Binder holds, and read `ChannelAccess::getRadioPosition()` rather than the mobility module, keeping round-trip delay, per-hop delay and path loss on one geometry.

The Binder keeps only what is registry work: the satellite/gateway registration and the `GnbNtnAssociation` record. The cell's derived delay is published back onto that record via `Binder::setGnbNtnCellRoundTripDelay()` on the first query, so it is derived once and both ends of every bearer read the same number. The `GeographicReferenceSystem *` is resolved per call rather than cached in a file-scope static — it is only valid for the current run, and Cmdenv executes `-r 0..N` in one process.

Two NED parameters on `NtnGNodeB.ned`, not on the Binder: `ntnMinElevation` (10deg) and `ntnMinSatelliteAltitude` (100km), both published onto the association at registration by `NtnIp2Nic`. They are per-cell because the elevation a cell will accept is a property of that cell, and a UE must dimension its timers from its serving cell's value rather than a network-wide one. The second is the guard that catches a query issued before `inet::INITSTAGE_SINGLE_MOBILITY`, where every radio still reports `(0,0,0)` — which converts to a *plausible* point on the geoid, so no range check would notice.

**Nothing is added on top of the geometry.** An earlier version carried an `ntnRoundTripDelayMargin` knob; it was removed. The timers need an upper bound rather than an estimate, and their own rounding to the next value TS 38.331 can signal already supplies it — for GEO the `t-PollRetransmit` enumeration jumps ms500 → ms800, so the derived value clears the round trip by 259 ms without anything being added. A separate margin would only shift where in that gap the value lands, and near GEO there is barely room for one: four round trips already sit just inside the 2200 ms ceiling of `t-ReassemblyExt-r17`, leaving about 8 ms of headroom. A scenario that needs more should set the timer it cares about explicitly.

**How the timers reach it.** Not through NED functions. Once a value is `ceil38331(RTD × 4)` rather than a constant, the NED expression is a call into C++ either way — but stringly-typed, with an implicit evaluation point and no debugger. Instead each NTN timer's NED default is a **negative sentinel** meaning "derive me", and the derivation happens in C++ in the profile subclass. An explicit ini assignment is positive and wins untouched, which is what keeps `GeoSatAmTerrestrialTimers` working.

Three new thin subclasses do it — `NtnNrRlcAmTxEntity`, `NtnNrRlcAmRxEntity`, `NtnNrRlcUmRxEntity` — bound by `tx.typename`/`rx.typename` in the two entity compounds, which are now pure typename redirects. Four timer members were promoted from private to `protected` in the three NR entity headers; that is an access-specifier change only, and the same pattern `NtnNrMacUe` already relies on in `LteMacUe`. Rounding to legal values is `ntn38331Ceil`/`ntn38331Floor` over a typed `Ntn38331Timer` enumeration in `common/NtnCommon.{h,cc}`.

`NtnNrMacUe` moved from an eager precompute at `INITSTAGE_SIMU5G_TTI_SETUP` to `refreshNtnCounters()`, called from `handleSelfMessage()` and `macHandleRac()`. Two independent reasons agree on this: the geometry does not exist during any Simu5G init stage, and a real UE latches these durations when the procedure starts rather than once at configuration.

**Not derived, deliberately.** `t301` is pinned to 2000 ms — it is read into a private member of `BearerManagement` at `INITSTAGE_LOCAL`, so deriving it needs an NTN `BearerManagement`, for a timer that in this model is a resume delay after RLF rather than a protocol deadline. `ntnRaResponseWindow` is pinned to 40 ms because it tracks twice the maximum *differential* delay across the beam, which needs the coverage model of item 5, not a path-length bound.

**Measured.** All four smoke configs are **byte-identical on every recorded scalar** to the values the orbit profiles hard-coded, and the terrestrial fingerprint suite is unmoved (158/158 rows identical against a stashed-baseline rebuild). GEO reproduces exactly because the derivation lands on the same six enumerated values that were transcribed by hand:

| Value | Derived at RTD = 541.46 ms | Previous GEO constant |
|---|---|---|
| `t_PollRetransmit` | 800 ms | 800 ms |
| `t_Reassembly` | 2200 ms | 2200 ms |
| `t_StatusProhibit` | 250 ms | 250 ms |
| `ntnRaResponseWindowOffset` | 541.46 ms | 542 ms |
| `ntnRetxBsrTimer` | 640 ms | 640 ms |
| `ntnRacBackoffMax` | 960 ms | 960 ms |

LEO's derived values *do* differ from the old profile (`t_Reassembly` 75 ms vs 150 ms, `ntnRetxBsrTimer` 20 ms vs 320 ms, `ntnRacBackoffMax` 30 ms vs 40 ms) and yet its results are also identical — at an 8.3 ms round trip with a 30 kB/s load, none of those counters ever binds. Do not read that as evidence the derivation is inert: overriding `ntnRetxBsrTimer` to 50 ms on GEO moves uplink 7650 → 10650 B/s, and `ntnRaResponseWindowOffset` to 5 ms moves it 25350/7650 → 15450/14400 B/s. It does mean **LEO needs a saturating-load scenario before its timer values can be said to be validated at all.**

**Two things the hard errors caught during bring-up**, both worth keeping in mind:

- Inflating the round-trip delay on GEO aborts the run once it passes about 550 ms, because `t_Reassembly` at four round trips would exceed the 2200 ms ceiling of `t-ReassemblyExt-r17`. GEO sits that close to the ceiling. A scenario needing more has run out of what the specification can express, which is exactly what the error says — and it is why the margin knob was dropped rather than kept as a tuning surface it could not usefully provide.
- The `tPollRetransmit > rtd` assertion (deferred here from step 5) fires at both ends of every AM bearer and aborts `GeoSatAmTerrestrialTimers`, whose whole purpose is to violate it. That config now sets `ntnCheckTimersCoverRoundTripDelay = false` rather than having its values softened.

**`ntnRoundTripDelay(binder, gnbId, ueId)` now has its consumer.** It is verified against the step-1 per-hop measurements (506.57 ms GEO, 8.33 ms LEO at pass peak, reproduced to three decimals by an independent code path), and step 6 uses it for grant activation times once the gNodeB has heard from a UE. No *timer* consumes it, and that remains deliberate: every timer is cell-wide, because a per-UE value at random-access time would model a UE that knew its own delay before connecting. Grant timing is the one place a per-UE value is legitimate, and only after attach.

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

`Binder::storeUlTransmissionMap()` keeps a two-slot `CURR_TTI`/`PREV_TTI` window (`src/simu5g/common/binder/Binder.cc:684`), assuming a UL transmission is heard in the same or previous slot. Under NTN delays that window is meaningless — it reports the UEs that transmitted recently rather than the ones whose signals are arriving together, which is a wrong neighbour set rather than an imprecise one.

**Now guarded.** `NtnChannelModel::initialize()` aborts when `uplinkInterference` is enabled on a satellite link, with `ntnAllowUplinkInterference` as the named opt-out. The check is at configuration time and not on the write, because the map is still *populated* in NTN runs — `LtePhyUe.cc:361` writes it whenever `isD2DInterferenceEnabled()` is set, and that defaults to true — and writing it is harmless; only reading it is wrong. Guarding the write would abort every NTN run. Note `computeUplinkInterference()` is not virtual, so the guard is single-layer.

When NTN interference is implemented (item 6 of `ntn-implementation.md`), the fix belongs in `ulTransmissionMap_` keyed by **arrival slot at the receiver**, not in the allocator. The reception slot each grant is already booked for is exactly that key. Do not reach for the allocator's `prevAllocatedRbsPerBand_` by analogy with the downlink path: on an NTN cell it now holds the allocation for a reception slot still in the future.

### Grant and resource timing — **done, see "Step 6 as built" below**

The gNB allocated RBs for "this TTI" while the UE transmitted an RTT/2 later. The fix is the 3GPP one: the grant carries the time from which it is valid, the UE holds it until then, and the gNB books resources for the slot the transmission will be heard in.

*Superseded:* this section predicted "the gNB books resources in a future-slot allocation map. This is the largest structural change here — stage it last, behind a flag." **No future-slot map was needed.** Indexing by reception slot rather than transmission slot makes `n -> n + D` a bijection for a per-carrier offset `D`, so each reception slot is booked by exactly one scheduling round and the existing single-TTI allocator already *is* that slot's map. The whole gNodeB change is one stamping hook.

### Step 6 as built

**The result that made this cheap.** The doc expected a ring buffer of depth `ceil(RTD/slot)` — ~1083 entries at GEO/µ=1. That is the depth you need if the allocation map is indexed by the UE's *transmission* slot. Indexed by the gNodeB's *reception* slot with one offset `D` per carrier, the map `n -> n + D` is a bijection: each reception slot is booked in exactly one scheduling round, so the existing single-TTI `LteAllocationModule` already is that slot's map and only its meaning changes. Verified: every reader of allocator occupancy (`readPerUeAllocatedBlocks`, `readRbOccupation`) runs inside `sendGrants()` in the same TTI as `schedule()`, and `getInterferingBlocks()` — the sole consumer of `prevAllocatedRbsPerBand_` — has one caller, `LteMacEnb.cc:1004`, which reads the *downlink* allocator.

Reception-slot indexing is also the physically right frame. With no timing advance (option A below), orthogonality is a property of the arrival instant, so the per-UE difference belongs in the activation time rather than in a per-UE offset. That is ideal timing advance, not per-UE K_offset.

**The algebra**, in slots of the carrier the grant belongs to:

| symbol | meaning |
|---|---|
| `slot` | the *carrier's* slot duration, from the numerology the Binder holds for it |
| `D` | lookahead for that carrier, `ceil(R_cell/slot) + k2` |
| `r` | reception slot being booked, `n + D` |
| `a(u)` | activation time in the grant, `r - ceil((R_ue/2)/slot)` |

**Per carrier, not per cell.** Numerology is a property of the carrier — registered once by `Binder::registerCarrier()` and read back through `getNumerologyIndexFromCarrierFreq()` — so a gNodeB and the UEs on a carrier cannot disagree about how long its slots are, which is what lets an activation time computed at one end be interpreted correctly at the other. It also means the lookahead differs per carrier on an aggregating cell: covering the same round trip takes twice as many slots at µ=1 as at µ=0. Using the module's own `ttiPeriod_` (its *highest* numerology, i.e. shortest slot) would be right only when every carrier shares it.

All three quantities are pure functions of the time and the carrier, holding no per-slot state. That is what makes them safe to read from `NtnSchedulerGnbUl` during scheduling *and* from `sendLowerPackets()` after it: both see the same answer regardless of the order they ask in, which a mutable "current target slot" would not have guaranteed.

Hold time is `(D - ceil((R_ue/2)/slot)) * slot - R_ue/2`, which is non-negative for every UE because `R_cell >= R_ue` and the rounding goes the right way — so a late grant is impossible by construction, and none was observed. GEO gives 34.7 ms after attach and 0.27 ms for a cell-edge UE.

**Where it lives.** `NtnNrMacGnb` (the branch's first gNodeB-side NTN MAC) stamps grants; `NtnNrMacUe` holds them in `pendingGrants_`, keyed by carrier then activation time, and promotes them at the start of the slot they name. `NtnSchedulerGnbUl` suppresses a retransmission grant while one is already in flight.

**Per-UE refinement is read fresh, not latched.** A UE gets the cell-wide bound until the gNodeB has heard from it (`ntnHeardFrom_`, set in `macPduUnmake()`); after that, `ntnRoundTripDelayFor()` calls `ntnRoundTripDelay()` directly on every grant. An earlier version cached the result with a validity window and a drift-margin hard error, mirroring how a real UE would latch assistance data and rely on it for a while (see Part 3). That mirroring bought nothing here: the thing being cached is a cheap geometry lookup, not something a real acquisition procedure makes expensive, so every read is exact and the cache only added a window in which a stale value could be used. Removed once that was noticed. See Part 3's "Does it need periodic updating?" for the fuller argument, since the same reasoning generalises past this one field.

One second-order effect of always reading fresh: it also closes the "grants issued from a stale value" failure mode the due-grant check in `NtnNrMacUe::promoteDueGrants()` originally listed as its primary suspect. With no cache, no two grants can be stamped from inconsistent geometry, so if that check ever fires now, a missed UE tick is the only remaining explanation, and its message says so.

The activation time rides on `UserControlInfo`, not on `LteSchedulingGrant`: that is already the branch's carrier for NTN per-hop metadata, and the UE reads the tag before the grant packet is deleted. `LteSchedulingGrant.msg` is untouched. Terrestrial footprint is three things — one `virtual` keyword on `LteMacBase::sendLowerPackets()`, two zero-defaulted fields on `LteControlInfo.msg`, and one value added to the `ulSchedulerType` `@enum` (mandatory: the parameter is validated against that list).

**Stamping through `sendLowerPackets()` rather than a hook in `sendGrants()`** matters more than it looks. `NrMacGnb` derives from `LteMacEnbD2D`, so `LteMacEnbD2D::sendGrants()` is the live path and `LteMacEnb::sendGrants()` is not; hooking only the latter compiles, passes every gate, and does nothing. Two traps found by measurement rather than reading:

- `sendGrants()` sets the direction on the **grant chunk**, never on the `UserControlInfo` tag, which stays at its `DL` default. A filter on `userInfo->getDirection() == UL` matches nothing. Read the chunk.
- `mac_` is resolved at `INITSTAGE_SIMU5G_AMC_SETUP`, not `INITSTAGE_LOCAL`, so a scheduler that checks its MAC's type must do it there.

**Measured, and the opposite of what was predicted.** The expectation was that holding a grant would cost throughput and add its hold time to uplink delay. Over 30 s with four UEs:

| | before | after |
|---|---|---|
| UL delivered | 89 910 B/s | **101 920 B/s** (+13%) |
| UL app delay | 1.770 s | **1.028 s** (-42%) |
| DL delivered | 94 500 B/s | 94 370 B/s (unchanged) |

**The reason is a defect this exposes rather than introduces.** `schedulingGrant_` holds one grant per carrier, and a grant arriving while another is installed overwrites it (`LteMacUeD2D.cc:373-378`). Over a 507 ms round trip the gNodeB issues grants continuously — as many as 25 are outstanding at once here — so most were discarded and the blocks booked for them went unused. Holding each against its own activation slot means each is used in the slot it was booked for. Transmit HARQ occupancy rises from a peak of 13 processes to the full 32, and no grant is ever late or skipped in any configuration.

`GeoSatSpreadUe` demonstrates the per-UE half: its four UEs, spread over 450 km, hold their grants for 33.7, 33.8, 34.0 and 35.1 ms respectively. LEO moves by about 1%, as expected for a ~8 ms hold on an 8.3 ms round trip.

**Duplicate retransmission grants were a much smaller problem than expected.** The concern was that one corrupted process would draw a grant in every slot of a round trip. It does not: `LteMacEnb::signalProcessForRtx()` already counts processes awaiting retransmission and decrements on grant, so the scheduler stops looking until another corruption is signalled. What remains is the residual case where that count is non-zero for a different process and the loop re-finds an already-granted one first — 56 grants over 30 s with four UEs, 50 with them spread out, one in the 2 s single-UE run, none over LEO. Delivery is unaffected; per-UE uplink moves up to 13% either way with totals conserved, which is the scheduler reordering which UE wins a contended slot. `NtnSchedulerGnbUl` is kept as a correctness fix, since the pathology grows with block error rate and load.

**Still open.** A UE the gNodeB has not yet heard from can only be given the cell-wide bound, so its Msg3 arrives slightly *before* the booked slot. Bounded by the beam footprint, invisible in every current scenario, and the one thing in this design that would need a multi-slot allocation map — of depth `Δ + 1` (~21 slots at µ=1 for a large GEO beam), not `ceil(RTD/slot)`. Needs the coverage model of item 5.

### Frame alignment and timing advance

Simu5G aligns every node's TTI tick to absolute simulation time with no TA at all. Two options:

- **(A) Ideal TA.** Grids stay aligned in absolute time; propagation delay contributes latency but not misalignment. Simplest, matches the perfect-assistance assumption, consistent with the branch's existing perfect-Doppler-compensation assumption.
- **(B) Explicit TA.** The UE advances its transmission by `2·d_service/c` plus common TA. Needed only to study TA acquisition, TA error, differential delay across a beam, or contention.

Do (A) first with an explicit note. Defer (B) until beams exist (item 5), since differential delay is a beam-scale phenomenon.

## Part 6 — Where the Code Should Live

The branch convention is NTN-specific subclasses (`NtnGNodeB`, `NtnIp2Nic`, `NtnPhyGnb`) rather than conditionals in mature terrestrial paths. MAC is exactly where this work lands.

**Both now exist**: `NtnNrMacUe` (step 3) and `NtnNrMacGnb` (step 6), plus `NtnSchedulerGnbUl`. They are thin subclasses wired by `NtnNrNicUe.ned` and `NtnNrNic.ned`, which makes the "no effect on terrestrial scenarios" guarantee structural rather than a matter of care — no terrestrial NED type references any of them, and no fingerprint test instantiates one. The whole of steps 3 and 6 needed exactly one access-specifier change in a terrestrial header (`sendLowerPackets()` made virtual) and no member promotions at all.

Guardrails:

- Every change must be a no-op when the RTD term is zero. Prefer additive offsets defaulting to zero over redefining existing NED parameters.
- Per the branch convention, abort if a MAC computes a zero RTD term while its PHY has non-zero propagation delay configured.

## Part 7 — Staging

1. ✓ **Delay only (done).** See "Step 1 as built" in Part 1 for what landed and the measured failure signature: GEO drops from 58.5/56.4 kB to 9.3/2.1 kB while LEO is untouched at an 8.3 ms RTD.
2. ✓ **RTD service and NED parameters (done).** See "Step 2 as built" in Part 4: the min-elevation bound reproduces TR 38.821's reference delays to the digit, every timer is derived from it, `ntnOrbitProfile` is deleted, and all four smoke configs are byte-identical to the values the orbit profiles hard-coded. Originally built on `Binder`; the geometry has since moved to `common/NtnCommon.{h,cc}` and the bounds to `NtnGNodeB.ned`, with no change to any derived value.
3. ✓ **RAC and BSR offsets (done).** See "RAC and BSR as built" in Part 2: GEO preamble count 44 → 2, delivery 4950 → 8100 B/s DL and 1050 → 3150 B/s UL, LEO unaffected.
4. ✓ **HARQ (done).** Process count raised to 32 on both ends, and downlink feedback disabled while uplink feedback stays on — see "HARQ as built" below. GEO downlink 8100 → 25350 B/s, 85% of offered load.
5. ✓ **Class 1 timer values (done, out of order).** RLC AM/UM and RRC `t301` — see "Class 1 as built" in Part 2. Taken early because it needed no C++ and no dependency on step 2; step 2 then replaced its per-orbit constants with geometry-derived values and added the `tPollRetransmit > rtd` assertion that was deferred to it.
6. ✓ **Grant/k2 restructuring (done).** See "Step 6 as built" in Part 5: the grant carries the time from which it is valid, the UE holds it until then, and the gNodeB books resource blocks for the slot the transmission will be heard in. No future-slot allocation map was needed. GEO uplink 89 910 → 101 920 B/s with application delay 1.770 → 1.028 s over 30 s and four UEs, because the single-slot grant store had been silently discarding grants.

## Part 8 — Diagnostics and Verification

**Step 1 is undebuggable without diagnostics that survive release builds.** Per the logging convention in `ntn-implementation.md`, `EV_DEBUG` and `EV_TRACE` are compiled out under `NDEBUG`. The following must be at `EV_INFO` or they do not exist where it matters:

- measured end-to-end RTT per UE;
- HARQ process occupancy and stall counts;
- RAC attempts and outcomes;
- BSR retransmission counts;
- grant-to-transmission latency.

Without these, a GEO smoke run that drops from 58 kB to 0 kB gives no way to distinguish a HARQ stall from a RAC storm from an RLC timeout.

**As built (step 6).** Recorded statistics rather than log lines, because what matters about most of them is their distribution over a run; the configuration values that must not be silently wrong are reported at `EV_INFO` on change, following `NtnNrMacUe::refreshNtnCounters()`.

| Statistic | Module | What it settles |
|---|---|---|
| `ntnHarqTxOccupancy`, `ntnHarqTxStall` | `NtnNrMacUe` | Whether the uplink is process-limited. Sampled before the slot inserts its own PDU, which is exactly when `firstAvailable()` would fail |
| `ntnHarqRxOccupancy` | `NtnNrMacGnb` | Whether uplink data is arriving at all. **Not** a stop-and-wait measure: a receive process is held for `harqFbEvaluationTimer` (the k1 budget), which does not scale with delay |
| `ntnGrantHoldTime`, `ntnPendingGrants` | `NtnNrMacUe` | How long grants wait and how many are outstanding |
| *(no statistic)* | `NtnNrMacUe` | More than one held grant coming due at once aborts the run: consecutive grants activate one carrier-slot apart and the UE ticks at least that often, so two at once means they were issued out of order |
| `ntnGrantActivationLead` | `NtnNrMacGnb` | The offset actually applied, in slots of the granted carrier |
| `ntnRtxGrantsSuppressed` | `NtnSchedulerGnbUl` | Retransmission grants not issued because one was already in flight |

The occupancy statistics settled a question immediately: with one UE the transmit pool peaks at 13 of 32 processes and never stalls, so the single-UE GEO uplink is grant-limited, not process-limited. It reaches 32 of 32 only once several UEs contend. Two of these are still absent — RAC attempts/outcomes and BSR retransmission counts have no counters, and grant-to-transmission latency is measurable from `grantIssueTime` but is not yet emitted.

Verification beyond the geometry checks already listed in item 7:

- one-way delay for GEO at nadir and at cell edge, and for the LEO TLE at a known epoch, against the TR 38.821 table above;
- a "delay added, timers untouched" run recorded as the documented failure baseline;
- delivered bytes for GEO and LEO after each staged fix;
- **the analytical check that actually validates the timer work**: with feedback-enabled HARQ, delivered throughput should equal `N_proc · TBS / RTT`. If the simulation does not reproduce that number, the HARQ pipelining is wrong regardless of what the delivery counters say.

Note also that the current 2 s smoke runs are no longer adequate: at a 540 ms RTT that is barely four round trips. GEO needs tens of seconds of simulated time, and LEO must still start inside a pass.

**Addressed by step 6**, which added three 30 s configurations to `simulations/nr/ntn_smoke/omnetpp.ini` without touching `GeoSat`: `GeoSatMultiUe` (4 UEs), `GeoSatMultiUeLoad` (8 UEs, past what the cell carries) and `GeoSatSpreadUe` (4 UEs over 450 km, so their one-way delays differ by whole slots). They are what showed the HARQ pool binding at all, and `GeoSatSpreadUe` is where per-UE grant timing is visible.

**A pre-existing defect they exposed, unrelated to the grant work.** `GeoSatMultiUeLoad` at the default seed leaves one UE locked out of uplink for the entire run: it fires a preamble about every RAR window, never obtains a grant, and its downlink meanwhile runs at the full offered rate. At another seed nobody starves but the per-UE uplink share still spreads about fourfold. This is the preamble-collision gap of item 4 in `ntn-implementation.md` — the gNodeB collision window is a fixed one-slot bucket with no round-trip term — and it needs the timing-advance model to close. It is invisible with one UE. **Compare runs of that configuration only at equal seeds.**
