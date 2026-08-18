# Propagation Delay and NTN Timers

How the `ntn` branch models propagation delay and the protocol timers that have to survive
it. Expands item 2 of `ntn-implementation.md` and the timer half of item 4.

Everything below is implemented. Open work is in the last section.

## Propagation delay

Each radio hop is delayed by its own ECEF slant range divided by the speed of light,
computed from the actual radio endpoints (`radioTransmitterId`/receiver coordinates in
`UserControlInfo`) rather than the logical source and destination, which stay at the gNodeB
and UE by design. `NtnPropagationDelay` holds the logic; both `NtnPhyBase` and `NtnPhyUe`
own an instance.

A transparent payload is an analog repeater, so the frame duration is charged **once** end
to end rather than once per hop. Without that, a two-hop bent pipe silently behaves as a
store-and-forward relay.

Both behaviours are flags on `NtnPhyBase.ned`/`NtnPhyUe.ned` (`useGeometricPropagationDelay`,
`transparentPayloadRelay`, both default true), so the pre-delay baseline is recoverable from
an ini file.

Measured: GEO 126.64 ms per hop, 506.57 ms four-hop round trip; LEO (350 km smoke TLE)
2.08 ms per hop, 8.33 ms round trip.

## Round-trip delay

Two free functions in `common/NtnCommon.{h,cc}`, both reading positions through
`ChannelAccess::getRadioPosition()` so that delay, path loss and timer dimensioning share one
geometry:

- **`ntnCellRoundTripDelay(binder, gnbId)`** — worst case anywhere in the cell, bounded by
  the cell's own `ntnMinElevation` (10 deg by default, per TR 38.821 clause 6.1.1) rather
  than by where the satellite currently is. Depends only on satellite *altitude*, so it is
  constant for a circular orbit; derived once and cached on the Binder association, so both
  ends of a bearer read the same number. This is what every timer is dimensioned from.
- **`ntnRoundTripDelay(binder, gnbId, ueId)`** — instantaneous, from actual positions. Used
  for grant timing after a UE has been heard from.

Both return zero for a gNodeB with no NTN association, which is what leaves terrestrial
scenarios untouched.

The bound reproduces TR 38.821's reference figures: GEO 541.5 ms, LEO-600 25.8 ms. So the
timer values are the report's own derivation computed rather than transcribed.

## Which timers scale with delay, and which must not

The useful distinction is not "counter versus `cMessage`" but **local processing budget
versus waiting for a peer**. Only the second scales.

| Class | Examples | Treatment |
|---|---|---|
| `cMessage` timers already in seconds | RLC `t_Reassembly`, `t_PollRetransmit`, `t_StatusProhibit`; RRC `t301` | New **values**, derived from the round trip |
| Wall-clock vs slot duration | `LteHarqProcessRx::isEvaluated()` (3GPP k1 decode budget) | **Unchanged.** Must not scale with propagation delay |
| Per-TTI decrement counters | RAC `raRespTimer_`, BSR `bsrRtxTimer_`, RAC backoff | New values, converted to slot counts |
| Per-TTI counters that are *not* response waits | periodic-grant `expirationCounter_`, `periodCounter_` | **Unchanged.** They count the UE's own cadence |

## Derived timer values

Every NTN timer is derived at runtime from the cell round-trip delay and rounded to a value
TS 38.331 can actually signal (`ntn38331Ceil`/`ntn38331Floor` over a typed enumeration).
There is no orbit label to set or get wrong. A negative NED default means "derive me"; an
explicit positive assignment wins untouched.

| Parameter | Derivation | GEO | LEO (smoke TLE) |
|---|---|---|---|
| `t_Reassembly` | RTD x HARQ transmissions, rounded up | 2200 ms | 75 ms |
| `t_PollRetransmit` | smallest legal value > RTD | 800 ms | 20 ms |
| `t_StatusProhibit` | < `t_PollRetransmit` - RTD, rounded down | 250 ms | 2 ms |
| `ntnRaResponseWindowOffset` | = RTD | 541.5 ms | 17.6 ms |
| `ntnRetxBsrTimer` | smallest legal `retxBSR-Timer` > RTD | 640 ms | 20 ms |
| `ntnRacBackoffMax` | largest TS 38.321 backoff indicator <= 2 x RTD | 960 ms | 30 ms |

Not derived, deliberately: `t301` is pinned to 2000 ms (largest legal T301; in this model it
is a resume delay after RLF, not a protocol deadline), and `ntnRaResponseWindow` to 40 ms,
because it tracks twice the maximum *differential* delay across the beam footprint, which
needs the coverage model of item 5 rather than a path-length bound.

Values are expressed in **time** and converted to slot counts against the actual slot
duration, since the inherited counters are slot counts. At numerology 0 slots and
milliseconds coincide, which is exactly why a NED-only version would look right in the smoke
scenarios and silently halve every timeout at numerology 1.

Conversion happens at the start of every slot rather than during initialisation, for two
reasons that agree: the geometry does not exist until `inet::INITSTAGE_SINGLE_MOBILITY`,
which runs after every Simu5G init stage; and a real UE latches these durations when a
procedure starts.

**Note on random access.** Simu5G models no Scheduling Request at all -- no PUCCH, no SR
resource, no `sr-ProhibitTimer` -- so a backlogged UE with no grant fires a RACH preamble
instead. `raResponseWindow` therefore carries three 3GPP roles at once (RAR window start
offset, RAR window, and `sr-ProhibitTimer`), and its total necessarily exceeds any legal
TS 38.331 `ra-ResponseWindow` value. That is a property of this model, not a spec-legal
configuration.

## HARQ

TR 38.821 clause 6.4.2 offers two mechanisms, and the branch uses both:

- **32 processes** (the Rel-17 ceiling). Keeping the pipe full needs one process per slot of
  round trip -- about 26 for LEO-600, but 541 for GEO -- so the process count solves LEO and
  cannot solve GEO.
- **Downlink feedback disabled, uplink feedback enabled.** TR 38.821 clause 7.2.1.4 treats
  these as separate decisions and measurement agrees: disabling downlink feedback raises GEO
  downlink about 37%, while additionally disabling uplink feedback costs about 80% of
  uplink, because an uplink MAC PDU also carries the buffer status report. Downlink
  reliability therefore rests on RLC ARQ; uplink stays stop-and-wait, which is why the
  process count still matters.

Both ends must agree: `LteHarqBufferRx` is sized from the receiver's `harqProcesses` while
the transmitter picks an ACID from its own.

## Uplink grant timing

Simu5G has no K1/K2/K_offset in the spec sense -- there is no explicit DL-to-UL slot relation
to shift -- so what is modelled is the physical effect K_offset and timing advance jointly
achieve, not the signalling procedure.

The gNodeB books resource blocks for the slot in which the granted transmission will be
**heard**, not the slot that scheduled it, and stamps each grant with the absolute time from
which it is valid (`grantActivationTime` on `UserControlInfo`). The UE holds the grant until
then instead of using it on receipt.

Per carrier, in that carrier's slots (numerology is a Binder-registered carrier property, so
gNodeB and UE agree on slot duration without signalling it):

    D    = ceil(R_cell / slot) + k2      lookahead
    r    = n + D                          reception slot booked now
    a(u) = r - ceil((R_ue/2) / slot)      activation time sent to UE u

Two consequences worth knowing:

- **No future-slot allocation map is needed.** With one offset per carrier, each reception
  slot is booked by exactly one scheduling round, so the existing single-slot allocator
  already is that slot's map.
- **A late grant is impossible by construction**, since `R_cell >= R_ue` and the rounding
  goes the right way. Hold time is about 35 ms at GEO, 9 ms at LEO. It is checked anyway,
  and aborts the run.

A UE gets the cell-wide bound until the gNodeB has heard from it, then its own delay --
mirroring TS 38.331's split between the cell-specific K_offset broadcast in SIB19 and the
per-UE refinement signalled by MAC CE after access. Using a per-UE value before attach would
model a network that knew a UE's range before it connected.

Because a receive process stays corrupted until its retransmission physically arrives -- a
whole round trip, not the ~2 slots the inherited scheduler assumes -- `NtnSchedulerGnbUl`
suppresses further retransmission grants for a process that already has one in flight.

## Interference

Not modelled in either direction on a satellite link, and **enabling it aborts the run**
rather than producing a plausible wrong number. Both mechanisms assume a transmission from
one or two slots ago is what is arriving now: true terrestrially, wrong by hundreds of slots
over a satellite link. Uplink reads `Binder::getUlTransmissionMap()` (a two-slot window);
downlink compares cells' resource-block bookings via `LteMacEnb::getDlBandStatus()`.
`ntnAllowUplinkInterference`/`ntnAllowDownlinkInterference` override the guards.

## Where the code lives

- `common/NtnCommon.{h,cc}` -- round-trip delay geometry, TS 38.331 value rounding
- `stack/phy/NtnPropagationDelay.{h,cc}` -- per-hop delay
- `stack/rlc/` -- `NtnNrRlcAmEntity`, `NtnNrRlcUmEntity` and their TX/RX profiles
- `stack/mac/NtnNrMacUe` -- RAC/BSR counters, grant hold
- `stack/mac/NtnNrMacGnb` -- grant offset and activation times
- `stack/mac/scheduler/NtnSchedulerGnbUl` -- retransmission suppression

Terrestrial behaviour is preserved structurally rather than by care: no terrestrial NED type
references any of these modules, so no fingerprint scenario can instantiate one. Where the
work did have to touch shared code it was kept additive and inert by default -- the grant
timing needed one `virtual` keyword, two zero-defaulted message fields and one `@enum` value.
The HARQ work is the exception: feedback disabling reaches into the shared HARQ buffer and
process classes, and is inert only because the flags guarding it default to terrestrial
behaviour.

## Current state

30 s, four UEs, GEO: uplink 105.5 kB/s and downlink 94.2 kB/s against roughly 120 kB/s
offered each way. Downlink application delay stays at 0.26 s, essentially the one-way
propagation delay, so the downlink does not queue; the uplink residual is the buffer-status
report and grant round trip.

Two-second figures elsewhere in the repository understate uplink badly -- that limit is four
GEO round trips and mostly startup, and most of what was sent is still in flight when the run
ends. Use the 30 s multi-UE configurations for anything steady-state.

## Remaining to fix or model

**Verification**

- **No automated NTN tests of any kind.** `tests/fingerprint/simulations.csv` has zero NTN
  rows; every result here was verified by hand-comparing recorded scalars against a
  stashed-baseline rebuild. Nothing guards against a regression.
- **LEO timer values are unexercised.** They are derived correctly and the path is
  demonstrably live, but at an 8.3 ms round trip under the smoke load no RAC or BSR counter
  ever binds. Needs a saturating-load scenario across a full pass.
- **Carrier-aware grant timing is untested.** Every NTN scenario is single-carrier at
  numerology 0, so the per-carrier slot arithmetic has never run with carriers of differing
  numerology.

**Diagnostics** (Part 8 of the original plan; `EV_DEBUG` is compiled out under `NDEBUG`, so
anything needed to explain a run must be `EV_INFO` or a statistic)

- Grant-to-transmission latency: `grantIssueTime` is stamped on every grant and **never
  read**. The measurement is plumbed but not emitted.
- No counters for RAC attempts and outcomes, or for BSR retransmissions.

**Modelling gaps**

- **Timing advance.** The branch assumes ideal TA (grids aligned in absolute time). The
  consequence is a gNodeB preamble-collision window that is a fixed one-slot bucket with no
  round-trip term, so under differential delay UEs that transmit in the same slot need not
  collide while UEs slots apart may collide spuriously. This is no longer hypothetical: in
  `GeoSatMultiUeLoad` at the default seed it locks one UE out of uplink for an entire 30 s
  run while its downlink runs at full rate. Needs item 5's coverage model.
- **The wired gNodeB-gateway fronthaul has no channel** (`gnb.ntn <--> ntnGateway.gnbLink`,
  zero delay and infinite datarate, in both smoke networks). It belongs in a
  `DatarateChannel`, not in `NtnRelay::relayDelay`, which must stay payload processing delay.
  It is also the one leg the round-trip delay service deliberately does not account for.
- **PDCP has no `discardTimer`** modelled at all, and `NrPdcpRxEntity.timeout` only runs on
  dual-connectivity split bearers, so it is dead code on a single-leg NTN bearer.
- **CQI aging.** Feedback is not timestamped, so the AMC cannot know its age. At ~240 ms
  one-way the gNodeB schedules on roughly 480 ms stale CSI: tolerable for GEO, not for LEO
  where elevation changes fast. Outer-loop link adaptation is required.
- **Pre-attach Msg3 can arrive early.** A UE not yet heard from can only be given the
  cell-wide bound, so its Msg3 arrives slightly before the booked slot. Bounded by the beam
  footprint; the clean fix reserves Msg3 blocks across a span, which is the one thing here
  that would need a multi-slot allocation map. Needs item 5.

**Small known defects**

- `NtnSchedulerGnbUl` guards `schedulePerAcidRtx()` but not the identical
  `schedulePerAcidRtxD2D()`. Inert today (no NTN scenario uses D2D).
- `NtnSchedulerGnbUl`/`NtnNrMacGnb` keep no cleanup for UEs that leave the simulation,
  unlike the surrounding `rtxschedule()` loop.
- `NrMacUe.cc:80` decrements an `unsigned int` and tests `< 0`, so a periodic grant can never
  expire. Dead today because the gNodeB never sets `periodic`; live the moment anyone uses
  activation times for configured grants.
- `Binder::initAndResetUlTransmissionInfo()` compares against a hardcoded `2 * TTI` (the LTE
  constant, not `ttiPeriod_`) -- a pre-existing numerology bug.
- `maxRacTryouts_` bounds nothing: the exhaustion branch resets the counter and zeroes the
  backoff, so a UE retries immediately and never stops after its satellite sets.
