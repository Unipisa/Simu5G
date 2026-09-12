# What's New in Simu5G

## v1.6.0 (2026-07-31)

This release adds a standards-compliant NR RLC to Simu5G. RLC Unacknowledged
Mode and Acknowledged Mode per TS 38.322 contributed by Esteban Egea Lopez have
been integrated into the mainline and are now the default on NR bearers. The RLC
entity modules were restructured into shared bases with LTE and NR concrete
implementations. The previously incomplete LTE RLC AM was reimplemented per TS
36.322 on the same architecture. Radio link failure detection with RRC
re-establishment was added. This release, like all releases since v1.3.1,
was developed by Andras Varga and the OMNeT++ core team.

Tested with INET-4.5.4 and OMNeT++ 6.3, compatible with INET-4.6.0 and OMNeT++
6.1 through 6.4.

### NR RLC (TS 38.322)

Simu5G's RLC layer so far implemented only the LTE wire format (TS 36.322: FI
framing with concatenation, one sequence number per PDU), and NR bearers used
it as well. This release adds a faithful NR RLC:

- **Unacknowledged Mode**: `NrRlcUmTxEntity`/`NrRlcUmRxEntity` perform SI +
  byte-offset (SO) segmentation without concatenation -- one SDU or SDU
  segment per PDU, one sequence number per SDU, `NrRlcUmDataPdu` on the wire.
  Reassembly is byte-coverage based (`RlcUmReceptionBuffer`) over an SDU-SN
  window with `t-Reassembly`. The SN field length is selectable (6 or 12 bits).

- **Acknowledged Mode**: `NrRlcAmTxEntity`/`NrRlcAmRxEntity` perform SO
  segmentation with re-segmentation on retransmission (via
  `RlcRetransmissionBuffer`), `pollByte`/`pollPDU`-driven status polling
  with `t-PollRetransmit`, and reassembly with `t-Reassembly` and
  `t-StatusProhibit`, using the `NrRlcAmDataPdu`/`NrRlcAmStatusPdu` formats
  over a 12- or 18-bit sequence number window.

- **NR bearers use the NR RLC by default**: `BearerManagement` gained the
  `nrRlcUmEntityModuleType` and `nrRlcAmEntityModuleType` parameters (default:
  the new `NrRlcUmEntity`/`NrRlcAmEntity` compound modules), and selects them
  for every bearer that has an NR node at either end; LTE bearers keep the
  `lteRlc*` ones. RLC framing is a function of the RAT rather than a free
  choice, so there is no LTE/NR mix; TM, being transparent, is identical for
  both RATs and has no NR variant.

This changes results in every NR simulation: the NR wire format has different
per-PDU header sizes and a different segmentation/reassembly discipline than
LTE FI framing, so packet timing, delay and throughput shift. (The MAC and
scheduler groundwork for it -- one PDU per SDU or segment, several RLC PDUs
multiplexed into one grant, exact octet-aligned header sizing -- shipped in
v1.5.1 and is only now actually exercised.) A configuration that needs the
previous behavior can point `nrRlcUmEntityModuleType` and
`nrRlcAmEntityModuleType` back at the `LteRlcUmEntity`/`LteRlcAmEntity`
compounds.

The NR RLC UM and AM implementations were contributed by Esteban Egea Lopez
(Universidad Politécnica de Cartagena). The code was originally published as
the "Simu5G-1.3.1 RLC-AM" special release and rebased onto several Simu5G
versions since; adapting it to the current RLC architecture was done by Attila
Török (OpenSim Ltd).

### RLC entity modules restructured

The RLC entity module and class names were made consistent with their
surroundings (`RlcMux`, `RlcTxEntityBase`, ...), the AM "Queue" names were
normalized to "Entity", and each mode's two variants were factored into a
shared base with LTE and NR concrete subclasses (`RlcUmTxEntityBase` with
`LteRlcUmTxEntity`/`NrRlcUmTxEntity`, and likewise for the other three). The
common shell -- MAC plumbing, D2D mode-switch machinery, UL burst-throughput
accounting -- lives in the base; only buffering, PDU build, reassembly, window
and timer logic is mode-specific. The renames:

      UmTxEntity  ->  LteRlcUmTxEntity      TmTxEntity  ->  RlcTmTxEntity
      UmRxEntity  ->  LteRlcUmRxEntity      TmRxEntity  ->  RlcTmRxEntity
      AmTxQueue   ->  LteRlcAmTxEntity
      AmRxQueue   ->  LteRlcAmRxEntity

Configurations that name these NED types explicitly need to be updated. The
`NrRlcUmEntity` and `NrRlcAmEntity` compounds are subclasses of
`RlcUmEntityBase` and `RlcAmEntityBase` that bind their two sides to the NR
concrete entities with `tx.typename`/`rx.typename`.

### LTE RLC AM reimplemented per TS 36.322

Simu5G's original LTE RLC AM was derived from UMTS RLC (TS 25.322), it was
incomplete, and no simulation configuration used it. What it implemented was not
TS 36.322 compliant: the wire format was per-SDU fragmentation with a sequence
number per fragment (no concatenation, no FI/LI, no poll bit), retransmission
was driven by per-PDU timeouts that resent without any NACK, a PDU exhausting
its retransmissions was silently discarded with no radio link failure
indication, and status reporting was periodic rather than event-driven.

It has been reimplemented from scratch on the architecture of the NR AM
entity, whose TS 38.322 ARQ skeleton TS 36.322 shares; only the framing is
LTE-specific:

- One AMD PDU per MAC grant, built by concatenating queued SDUs and SDU
  fragments (FI framing, on the same PDU model the LTE UM entity uses). The
  built PDU, retained in the 512-entry (10-bit SN) transmission window, is the
  unit of ARQ.
- NACK-driven retransmission with the `ACK_SN` + NACK-list STATUS PDU (the
  same `StatusPduData` structure the NR AM uses, including SOstart/SOend byte
  ranges), re-segmenting a retained PDU into AMD PDU segments when the grant
  is smaller than the PDU.
- `pollPDU`/`pollByte`/`t-PollRetransmit` polling, `t-Reordering` and
  `t-StatusProhibit` at the receiver, and radio link failure at
  `maxRtxThreshold` retransmissions, wired to the same
  `BearerManagement` teardown and RRC re-establishment as the NR AM.

Since no configuration could use the old LTE AM, this does not affect existing
simulation results.

### Selecting RLC AM

Acknowledged Mode is now usable on both RATs, but nothing selects it by default:
every bearer stays in the mode it had before, so existing simulations are
unaffected. Two mechanisms choose the mode of a bearer, depending on whether
SDAP is in the stack.

Without SDAP, `Ip2Nic` classifies each packet into a traffic class by packet name
(`VoIP*` -> conversational, `gaming*` -> interactive, `VoDPacket*` -> streaming,
anything else -> background) and maps the class to an RLC mode with its
`conversationalRlc`, `streamingRlc`, `interactiveRlc` and `backgroundRlc`
parameters. They accept `"TM"`, `"UM"` and `"AM"`, and all four default to
`"UM"` (which is the pre-v1.6.0 behavior, kept for backward compatibility).

With SDAP in the stack (`hasSdap = true` on the NR NIC), `Ip2Nic` skips traffic
classification entirely and the mode becomes a property of the DRB: every entry
of `NrSdap.drbConfig` takes an optional `rlcType` field, again one of `"AM"`,
`"UM"` and `"TM"`, and again defaulting to `"UM"`. For example:

      *.gnb.cellularNic.hasSdap = true
      *.gnb.cellularNic.sdap.drbConfig = [
          {"drb": 0, "ue": 2049, "qfiList": [1, 2], "rlcType": "UM"},
          {"drb": 1, "ue": 2049, "qfiList": [3, 4], "rlcType": "AM"}]

Either way, both ends of a bearer must be configured with the same mode: each
node builds its own RLC entity from its own configuration, so a mismatch leaves
an AM entity facing a UM one. With `Ip2Nic`, this can be ensured by using `**.`
wildcards; with SDAP, the UE's `drbConfig` entry for a DRB and the gNB's entry
for the same DRB have to agree on `rlcType`.

Which entity type then implements the mode follows from the RAT, as described
above: an AM bearer with an NR node at either end runs the `NrRlcAmEntity`
compound, an LTE one `LteRlcAmEntity`. TM is available on both, and is the same
entity for both.

### RLC validation scenarios

The new `simulations/nr/rlc` and `simulations/lte/rlc` directories hold
protocol-validation scenarios for the two RLC implementations: a single UE
over `LteDummyChannelModel` -- which replaces propagation modelling with a
configurable per-direction packet error rate, so with independent HARQ
attempts the residual loss RLC sees is exactly `perDl^(maxHarqRtx+1)` -- with
deterministic CBR traffic and the loss process on its own RNG. The scenarios
sweep the error rate (`AM-Lossy`, with `UM-Lossy` as the no-ARQ contrast),
force segmentation and re-segmentation on retransmission (`AM-Segmentation`),
concatenation on LTE (`AM-Concatenation`), a transmission-window stall that
must recover (`AM-WindowStall`), and a scripted mid-run coverage loss that
must end in a radio link failure (`AM-RLF`) or in RRC re-establishment with
the flow resuming (`AM-RLF-Reestablish`). Three scenarios cover the common
usage patterns beyond a lossy downlink: `AM-Lossy-UL` (both RATs) runs the
flow uplink, through the UE MAC's strict grant accounting; `TCP-AM` carries a
TCP transfer over the lossy bearer, its acknowledgement stream putting data
through the reverse direction of the same bearer; and
`lte/test_handover VoIP-AM-Handover` runs bidirectional VoIP over AM with the
UEs moving through handovers.

Measured on both RATs: every AM configuration delivers every offered SDU at
every loss rate in the sweep, uplink and downlink -- the AM guarantee --
while UM loses the predicted residual fraction, and the per-attempt HARQ
error rate matches the configured error rate throughout. TCP makes steady
progress over a downlink losing half its transmission attempts, and the
handover scenario completes with zero application-level frame loss and no
entities left behind at the old cell.

Defects found in the NR AM implementation found using these scenarios
were fixed.

### Radio link failure and RRC re-establishment

The RLC AM transmitters declare a radio link failure when a PDU exceeds
`maxRtxThreshold` retransmissions (TS 38.322 5.3.2 / TS 36.322 5.2.1). This
is now wired to a full teardown of the link:

- `BearerManagement::scheduleRadioLinkFailure()` defers the teardown to a safe
  execution context (so that entity modules are never deleted from inside
  packet processing), then releases the link at both ends -- reaching the
  peer's `BearerManagement` through the `Binder` -- deleting the bearer's MAC
  (`deleteQueuesRadioLinkFailure()`, which also drops the node's in-flight HARQ
  feedback), RLC and PDCP state.

- `Ip2Nic` gained `releaseUe()`/`resumeUe()`, and drops a released peer's DL
  and UL packets for as long as its context is released, modeling the RRC UE
  Context Release. Without this, the application kept pushing packets at
  torn-down entities, which crashed; handover does not have this problem only
  because it redirects the traffic to a new cell.

- RRC re-establishment (TS 38.331 5.3.7) is modeled by its timers, the way
  handover signaling already is: `BearerManagement.t311` (cell selection) and
  `t301` (request to complete). When `t301` expires, the peer is un-released
  and its bearer re-establishes on demand. The default `t311 = 0s` disables
  re-establishment, that is, a radio link failure releases the UE to idle.

This is inert in simulations that do not use RLC AM, as only the AM entities
detect radio link failures.

### RLC statistics recorded on the bearer entities

The per-bearer RLC statistics -- `rlcDelay*`, `rlcThroughput*`, `rlcPduDelay*`,
`rlcPduThroughput*`, `rlcPacketLoss*` and their D2D variants -- are now recorded
on the RLC entity module of the bearer that produced them, instead of on an
`RlcMux`. **Configurations and analysis files that refer to these results by
module path need to be updated**, for example from

      SingleCell.ue[0].cellularNic.nrRlcMux.rlcDelayDl:mean

to the bearer entity that measured it, such as

      SingleCell.ue[0].cellularNic.nrRlc-um-1-1.rx.rlcDelayDl:mean

The old arrangement dates from when RLC was a single module per network
interface, with the per-connection entities being plain C++ objects inside it:
there was no per-bearer module to record on, so a receiving entity reached the
*other* node's mux through the `Binder` and emitted the sample there -- an
uplink measurement taken at the gNB was recorded as a result of the UE. Since
v1.5.0 the entities are modules in their own right, one per peer and radio
bearer, so each sample is now recorded where it is produced. Results for one UE
across its bearers are obtained by aggregating over its entity modules in the
analysis tool.

The cell-level statistics (`rlcCellThroughput*`, `rlcCellPacketLoss*`) were
**removed** rather than moved. The cell throughput was computed from a C++
`static` byte counter -- one counter for the entire simulation, not one per
cell -- so in any scenario with more than one cell, every serving node reported
approximately the network-wide total as its own cell throughput. (In
`lte/multicell`, both eNBs report the global figure; the true per-cell values
are about half of what was recorded.) The statistic was correct only in
single-cell scenarios, where it equals the sum of the per-bearer
`rlcThroughput*` results, which is how it can be obtained now.

The MAC layer's `macCellThroughput*` statistics (including the D2D variant,
which shared the same counter and thus mixed D2D and cellular bytes) had the
identical defect and were removed for the same reason; the per-UE
`macThroughput*` results remain. `macCellPacketLoss*`, which is computed
per-cell correctly, is kept.

Two side effects are worth noting. `rlcPacketLoss*` was emitted onto a module
that did not declare it, so it was never recorded at all; it now is. And
per-bearer results that used to be merged into one mux are visible separately
per bearer, which is what makes the two legs of a Dual Connectivity split
bearer individually measurable.

### PDCP mux renamed and refactored

`UpperMux` was renamed to `PdcpMux`: the old name said where the module sits
relative to the PDCP entities rather than which layer it belongs to, and did not
match its RLC counterpart `RlcMux` (the submodule was already named `pdcpMux`).
Like `RlcMux` in v1.5.2, it now maps DRBs to `toTxEntity` gate indices instead
of TX entity pointers, so dispatch is plain multiplexing.

Both muxes became replaceable submodules, with the new `IPdcpMux` and `IRlcMux`
module interfaces. Replaceability is partial: `BearerManagement` creates and
wires the per-bearer gates and registers the routing tables through the C++
classes, so an implementation has to subclass `PdcpMux` or `RlcMux`; what the
interface buys is type selection from NED and ini.

The NR-leg flag moved off `PdcpMux`, which never read it, onto its only reader,
`Ip2Nic`: **`cellularNic.pdcpMux.isNR` is now `cellularNic.ip2nic.isNr`**,
spelled like the same flag on `LteMacUe`, `LtePhyUe` and `HandoverController`.
Configurations that set it need to be updated. `BaseStationStatsCollector` also
lost its `pdcpModule` parameter, which was unread and pointed at a module the
v1.5.0 PDCP flattening deleted.

### Other

- **RLC statistics on NR bearers**: the NR RLC entities did not emit the
  per-bearer delay and throughput statistics that their LTE counterparts do, so
  those results were empty in NR simulations from the moment the NR RLC became
  the default on NR bearers. They are emitted now. `NrRlcAmRxEntity` also emits
  `rxWindowOccupation`, which was declared but never emitted; the NR UM
  transmitter's `requestedPDUSize`/`sentPDUSize` statistics were renamed to
  `requestedPduSize`/`sentPduSize`, and it gained the
  `receivedPacketFromUpperLayer`/`sentPacketToLowerLayer` counters.

- **LteDummyChannelModel made usable**: the class had no NED type (so it could
  not be instantiated) and hardcoded error rates. It now has one, with `per` /
  `perDl` / `perUl` / `perD2D` and `harqReduction` parameters -- the
  per-direction rates volatile, so a coverage loss can be scripted as a
  function of time -- turning it into a controlled loss source for protocol
  validation: with `harqReduction = 1` the residual loss RLC sees is exactly
  `per^(maxHarqRtx+1)`. It also reports SINR/RSRP on every band; the
  single-element vector it used to return broke the AMC.

- **MEC RNI**: `PacketFlowObserver` now also tracks NR SO PDUs, which carry no
  per-PDU RLC sequence number, by keying the per-SDU tracking on the PDCP
  sequence number instead. The reported delay is exact for the common
  unsegmented case; an SDU segmented across several MAC PDUs is accounted as
  delivered on the acknowledgement of its first segment.

- **D2D**: D2D bearers run on the NR RLC as well; draining of the mode-switch
  holding buffer now takes place in the owning entity's context.

- **Module references**: the RLC-to-RRC and RRC-to-Ip2Nic lookups became NED
  module-path parameters (`RlcMux.bearerManagementModule`,
  `BearerManagement.ip2nicModule`), continuing the `ModuleRefByPar` conversion.

- **Simulations**: `nr/standalone` gained the `VoIP-DL-AM`, `VoIP-DL-AM-Lossy`,
  `VoIP-UL-AM`, `VoIP-DL-UM-NR` and `VoIP-UL-UM-NR` configurations, and
  `lte/demo` the `VoIP-AM` configuration, exercising the AM and the NR RLC
  paths.

- **Fingerprint tests**: the five new configurations above were added to the
  suite, together with the RLC validation scenarios of `simulations/nr/rlc`
  and `simulations/lte/rlc` and the `VoIP-AM-Handover` configuration of
  `lte/test_handover` (157 configurations in total), and the rows were
  re-recorded for the NR RLC default and the statistics changes.

- **Documentation**: the RLC entity documentation comments were retargeted at
  the compound modules that actually bind them -- several still referred to
  per-side `rlcUm{Tx,Rx}EntityModuleType` parameters, which v1.5.1 replaced
  with selection on the per-bearer compound -- and the `RlcUmEntityBase` /
  `RlcAmEntityBase` comments now name both of their concrete subclasses.

- **Source housekeeping**: file headers were brought in line -- the contributed
  NR RLC sources now carry the standard Simu5G header naming their author
  instead of an LGPL blurb, files that had no header got one, and new files
  that had inherited the header of the file they were derived from now name
  their actual author. The redundant `@class` line was dropped from the C++
  class comments, and `IRlcAmEntities.ned` was split into `IRlcAmTxEntity.ned`
  and `IRlcAmRxEntity.ned`, one interface per file. The interfaces themselves,
  and all type names, are unchanged.


## v1.5.2 (2026-07-30)

This release corrects the names of the per-bearer PDCP and RLC entity modules
that v1.5.0 and v1.5.1 introduced, before more code comes to depend on them. It
changes names only: no behavior changes, and no simulation results change. It
also refactors RlcMux.

Tested with INET-4.5.4 and OMNeT++ 6.3, compatible with INET-4.6.0 and OMNeT++
6.1 through 6.4.

### PDCP and RLC entity module names made consistent

The PDCP entity modules and their module interfaces were renamed so that the
TX/RX role follows the layer name, as it does in RLC and in their own C++ base
classes (`PdcpTxEntityBase`, `PdcpRxEntityBase`):

      LteTxPdcpEntity  ->  LtePdcpTxEntity      ITxPdcpEntity  ->  IPdcpTxEntity
      LteRxPdcpEntity  ->  LtePdcpRxEntity      IRxPdcpEntity  ->  IPdcpRxEntity
      NrTxPdcpEntity   ->  NrPdcpTxEntity
      NrRxPdcpEntity   ->  NrPdcpRxEntity

The per-bearer compound modules were restructured so that an explicit LTE
concrete type stands beside the NR one, instead of the base type doubling as the
LTE type. `PdcpEntity`, `RlcUmEntity` and `RlcAmEntity` became `PdcpEntityBase`,
`RlcUmEntityBase` and `RlcAmEntityBase`, which bind no entity types and are
therefore not instantiable on their own; the instantiable types are their
subclasses, which bind their two sides with `tx.typename`/`rx.typename`:

      PdcpEntity   ->  PdcpEntityBase   + new LtePdcpEntity
      RlcUmEntity  ->  RlcUmEntityBase  + new LteRlcUmEntity
      RlcAmEntity  ->  RlcAmEntityBase  + new LteRlcAmEntity

`NrPdcpEntity` is now a subclass of `PdcpEntityBase`. A mixed entity (e.g. an
EN-DC master eNB, which runs an NR TX side with an LTE RX side) still overrides
just `tx.typename` or `rx.typename`.

`BearerManagement`'s parameters that select the RLC entity types were renamed so
that the LTE ones are marked as explicitly as their coming NR counterparts:
`rlcUmEntityModuleType` -> `lteRlcUmEntityModuleType` and
`rlcAmEntityModuleType` -> `lteRlcAmEntityModuleType`. Its
`pdcpEntityModuleType` parameter keeps its name -- it is a single per-node
selector, and it holds an NR type at every gNB -- and now defaults to
`LtePdcpEntity`. `RlcTmEntity` and `rlcTmEntityModuleType` are unchanged: TM is
transparent and identical for both RATs, so it has a single entity type.

Configurations that name any of these NED types explicitly need to be updated.

### RlcMux refactoring

`RlcMux` now maps DRBs to gate indices, not RX entity pointers. Its internal
table now holds the index of the `toRxEntity` gate serving each DRB instead of a
pointer to the RX entity, so PDU dispatch is plain multiplexing: `send()` on the
gate index, with no entity involved. It used to make a round trip -- look up the
entity, then ask it for its gate and walk back to our own gate.


## v1.5.1 (2026-07-28)

This release continues the architectural overhaul of Simu5G, focusing on the
PDCP and RLC layers: per-bearer protocol entities are now packaged into
compound modules, DRBs are established as bidirectional (duplex) bearers, and
the Dual Connectivity split-bearer data path was restructured.

Tested with INET-4.5.4 and OMNeT++ 6.3, compatible with INET-4.6.0 and OMNeT++
6.1 through 6.4.

### Per-bearer PDCP and RLC compound entity modules

v1.5.0 transformed the PDCP and RLC layers into dynamically created per-bearer
TX/RX entity modules. This release packages the two sides of each bearer into
one compound module per bearer, following the 3GPP model of one PDCP/RLC
entity per bearer (for RLC AM explicitly with a transmitting and a receiving
side):

- **RLC**: Each bearer's TX and RX entities now live in an `RlcTmEntity`,
  `RlcUmEntity` or `RlcAmEntity` compound module. In AM, the internal control
  paths are now explicit gate connections between the RX and TX submodules,
  replacing C++ registry-lookup calls between the two simple modules:
  `feedbackOut` -> `feedbackIn` carries the STATUS PDUs received from the peer
  into the TX side's ARQ, and `statusOut` -> `statusIn` hands the locally
  generated status reports to the TX side for transmission.

- **PDCP**: Each bearer's TX and RX entities now live in a `PdcpEntity`
  compound module. Variants are subclasses overriding the entity typenames
  (`NrPdcpEntity`); a mixed entity (e.g. an EN-DC master eNB, which runs an NR
  TX side with an LTE RX side) overrides just `tx.typename` or `rx.typename`.

- **PdcpRelayEntity**: At a Dual Connectivity secondary node, which only
  tunnels already-processed PDUs between the master (over X2) and its own RLC,
  the two per-bearer bypass modules were packaged into a `PdcpRelayEntity`
  compound module, which stands in place of the bearer's PDCP entity;
  `BypassTxPdcpEntity`/`BypassRxPdcpEntity` were renamed to
  `PdcpDownlinkRelay`/`PdcpUplinkRelay`.

- New module interfaces (`ITxPdcpEntity`, `IRxPdcpEntity`, `IRlcTxEntity`,
  `IRlcRxEntity`, `IRlcAmTxEntity`, `IRlcAmRxEntity`) make the entity
  implementations replaceable: every compound binds its two sides with the
  standard `tx.typename`/`rx.typename` submodule typename assignment, which a
  configuration or a subclass of the compound can override. The ten per-side
  entity-type parameters of `BearerManagement` were consolidated into five
  per-compound ones (`pdcpEntityModuleType`, `pdcpRelayEntityModuleType`,
  `rlcTm/Um/AmEntityModuleType`).

### DRBs established as duplex (bidirectional) bearers

Per TS 38.331, a DRB is bidirectional; Simu5G so far established each
direction as an independent unidirectional bearer with its own
locally-assigned DRB id. RLC AM fundamentally needs the reverse path of the
same bearer for its STATUS PDUs, which the old model could only provide via
on-demand reverse entities created from inside packet processing. Now:

- `Binder::establishDataConnection()` (renamed from
  `establishUnidirectionalDataConnection()`) creates both directions of a
  unicast bearer at once; multicast bearers remain unidirectional.

- DRB ids are allocated by the `Binder` with a counter per node pair, so the
  two ends of a bearer see the same DRB id, and DRB ids are peer-scoped
  (per-UE identities, as in the spec) rather than node-unique.

- Reverse application traffic resolves to the reverse leg of the existing
  bearer instead of allocating a second bearer.

This may change results in simulations where request and response flows
between the same node pair previously used two separate bearers: they now
share one duplex bearer, which changes logical channel ids and can change
scheduling order under contention.

### Dual Connectivity split-bearer data path restructured

A DC split bearer is one PDCP entity -- one sequence number space -- whose
PDUs are steered per-packet across two RLC legs. The `PdcpEntity` compound now
reflects this: its lower boundary is a `legOut[]`/`legIn[]` gate vector. A
plain bearer has one leg, and the TX/RX entities connect straight to it; a
split bearer routes the TX side through a `DcPdcpLegSplitter` (per-PDU leg
dispatch, per-leg DC id mapping and statistics) and merges both legs through a
`PdcpLegJoiner` into the single RX entity, whose one reordering window
restores sequence order across the legs. The per-packet leg steering policy
itself remains in `TechnologyDecision`; the splitter only executes it.

### MAC prepared for NR RLC framing

The MAC and the schedulers can now accommodate an RLC that emits one SDU or
segment per PDU without concatenation (the NR model of TS 38.322), in addition
to LTE's single concatenated PDU per grant: for such flows, the schedulers
plan one PDU per SDU/segment to fill the grant, the MAC issues one SDU request
per planned PDU and multiplexes them into the MAC PDU, and exact per-PDU RLC
header sizes are computed (octet-aligned, per SN length and segment state).
This is inert by default (`soFraming=false` keeps the LTE path) and is
groundwork for an upcoming standards-compliant NR RLC implementation.

### Beacon emission control at the eNB/gNB

- Beacon broadcasting was decoupled from `enableHandover`: the new
  `enableBeacons` parameter (default: `enableHandover`) controls it, so that
  radio link monitoring can later work without handover enabled.
  `enableHandover=true` now requires beacons to actually flow.

- A non-positive `beaconInterval` is now an initialization error instead of
  silently disabling beacons; beacons are switched off with
  `enableBeacons=false`.

### Module architecture improvements

- **isNr as parameter**: `LteMacUe`, `LtePhyUe` and `LteDlFeedbackGenerator`
  no longer determine whether they are the NR leg of the UE by string-matching
  their own module name ("nrMac", "nrPhy", "nrDlFbGen"); they now have a
  `bool isNr` parameter, set by `NrNicUe`.

- **Parametrized module references**: Hardcoded `getSubmodule()` walks inside
  the NIC were replaced with NED module-path parameters (11 new parameters
  across `BearerManagement`, `HandoverController`, `DcMux`, `LteMacEnb` and
  `TechnologyDecision`), and foreign-node lookups now go through `Binder`
  helper methods.

- **IHandoverPacketHolder**: The `hoManagerOut` gate was added to the module
  interface, so that custom holder implementations can be substituted
  (contributed by Mohamed Seliem).

### Bug fixes

- **Crash on interleaved Dual Connectivity leg handovers**: Fixed a
  long-standing crash (also present in v1.4.3..v1.4.5) triggered when the LTE
  leg of an NR UE hands over while its NR leg is detached: per-UE state
  provisioned at the old master's secondary gNB was left orphaned, and
  re-establishment collided with the leftovers when the UE later returned.
  Handover cleanup now also covers the old serving node's secondary. In
  addition, PDCP entity teardown at NR UEs is now keyed by peer node, so an
  NR-leg detach no longer deletes the LTE leg's entities as well.

- **MAC**: `macSduRequest()` no longer underflows when the scheduler allocates
  a grant smaller than the MAC header, which surfaced as a misleading
  "configured queueSize too low" error with many DRBs under `QOS_PF`
  contention (contributed by Mohamed Seliem).

- **NrPhyUe**: D2D DATA frames arriving while the UE is detached during
  handover are now dropped, as `LtePhyUeD2D` already did, instead of crashing
  on already-deleted HARQ buffers.

- **HandoverController**: Removed a redundant second detach/attach of the D2D
  direction on the AMC during NR UE handover.

- **LtePhyEnb**: Corrected copy-pasted class names in `requestFeedback()`
  error messages.

### Other

- **Fingerprint tests**: The simulation-time intervals of configurations
  involving events like handover or D2D mode switching were extended so that
  the fingerprint window actually covers those events, and fingerprints were
  re-recorded for the architectural changes above.


## v1.5.0 (2026-07-13)

This release continues the architectural overhaul of Simu5G. Major themes
include consolidating Control Plane functions under the RRC module, adding QoS
support via DRBs and the SDAP protocol, restructuring Ip2Nic and other modules
for cleaner architecture, and improving type safety throughout the codebase.

Tested with INET-4.5.4 and OMNeT++ 6.3, compatible with INET-4.6.0 and OMNeT++
6.1 through 6.4.

### More explicit Control Plane modeling

Continuing the direction set in v1.4.3, code fragments that implement pieces of
the 3GPP Control Plane have been identified throughout the codebase and collected
under the `Rrc` module. `Rrc` is now a compound module with the following
submodules:

- **BearerManagement**: The former simple `Rrc` module, renamed and extended.
  It now owns the lifecycle (creation, deletion, lookup) of all PDCP and RLC
  entities. Previously, entity management was scattered across the monolithic
  PDCP module and the `LteRlcUm`/`LteRlcAm` modules.

- **Registration**: Node registration and deregistration logic, previously
  embedded in `Ip2Nic`, was moved here.

- **HandoverController**: Handover decision and execution logic was extracted
  from `LtePhyUe` into this new module. This is architecturally more correct,
  as handover is an RRC function, not PHY. The internal "handover packet"
  misnomer was corrected to "beacon" (`HANDOVERPKT` -> `BEACONPKT`,
  `broadcastMessageInterval` -> `beaconInterval`). Several parameters were
  exposed as NED parameters (`hysteresisFactor`, `handoverDetachmentTime`,
  `isNr`).

- **D2DModeController**: D2D mode selection was moved here from the former
  `stack/d2dModeSelection/` directory, and D2D peer tracking from
  `LteRlcUmD2D`.

### QoS support: SDAP, DRBs and per-bearer PDCP/RLC entities

QoS (Quality of Service) support was added through Data Radio Bearers (DRBs)
and the SDAP (Service Data Adaptation Protocol) layer, which is part of the 5G
NR protocol stack. The code is based on a contribution by Mohamed Seliem
(University College Cork); see releases v1.4.1-sdap and v1.4.1-sdap-2 for
details. In this release, the code was substantially reworked and integrated
into the main codebase.

In accordance with the 3GPP architecture, the PDCP and RLC layers were
transformed so that they purely consist of per-DRB entities, created and
configured by `BearerManagement` (RRC). Each DRB has dedicated PDCP TX/RX and
RLC TX/RX entity modules, wired directly to each other via per-bearer gate
connections.

Details:

- **SDAP protocol layer**: An SDAP implementation was added, providing
  QFI-to-DRB routing with a JSON-configured `DrbTable`. The SDAP layer is
  optional in NR NICs (enabled via `hasSdap=true`).

- **QFI propagation via GTP-U**: QFI is set by the application via DSCP,
  picked up by `TrafficFlowFilter`/UPF, carried in the GTP-U protocol header
  (mirroring the 3GPP PDU Session Container extension header), and extracted
  by the gNB for SDAP routing.

- **QoS-aware proportional fairness scheduler**: A `QoSAwareScheduler` was
  added to MAC, supporting QFI-based scheduling with configurable weight
  constants. Enable with `LteMacEnb.schedulingDisciplineDl/Ul = "QOS_PF"`.

- **DRB configuration in JSON**: DRB configuration is split between SDAP
  (`sdap.drbConfig` for QFI-to-DRB routing) and MAC (`mac.drbQosConfig` for
  QoS scheduler parameters), both in JSON format.

- **Non-IP PDU session support**: SDAP was generalized for non-IP PDU session
  types, with `PduSessionType` enum and `upperProtocol` in DRB configuration.

- **PDCP refactored into per-bearer entities**: The former monolithic PDCP
  module (which had six subclass variants for LTE/NR × UE/eNB/D2D) was
  replaced with per-bearer `PdcpTxEntity` and `PdcpRxEntity` modules, plus
  `PdcpMux` for upper-layer routing and `DcMux` for Dual Connectivity X2
  forwarding. Bypass entities handle the DC secondary leg. Entities communicate
  via OMNeT++ gates, not C++ method calls.

- **RLC refactored into per-bearer entities**: The former `LteRlc` compound
  module (containing `LteRlcUm`/`LteRlcUmD2D`, `LteRlcAm`, `LteRlcTm`) was
  replaced with per-bearer TX/RX entity modules for all three RLC modes (UM,
  AM, TM), plus `RlcMux` for MAC↔entity routing.

- **PDCP↔RLC directly wired**: PDCP and RLC entities are connected directly
  via per-bearer gates. All submodules now reside directly at NIC level -- the
  former `PdcpLayer` and `LteRlc` compound modules no longer exist.

- **Example simulations**: `simulations/nr/standalone_drb/` with
  multi-UE, multi-QFI configurations.

### Ip2Nic decomposed, further module architecture improvements

The `Ip2Nic` module, which had accumulated various unrelated responsibilities
over time, was decomposed. Several code fragments were factored out into
separate modules:

- **`analyzePacket()` moved to Ip2Nic from PDCP**: Packet classification
  (filling `FlowControlInfo` tags) was moved to where it logically belongs --
  at the IP-to-NIC boundary. The `IpFlowInd` tag was eliminated. RLC type NED
  parameters (`conversationalRlc`, etc.) also moved from PDCP to `Ip2Nic`.

- **HandoverPacketHolderUe/Enb**: Handover packet buffering was factored out
  of `Ip2Nic` into separate modules. X2 tunneled packets are now received via
  gates instead of C++ method calls.

- **TechnologyDecision**: Dual Connectivity technology selection logic was
  extracted into a separate, configurable module that uses NED expressions.

Further module architecture improvements:

- **MAC turned into compound module**: MAC is now a compound module with `AMC`
  and DL/UL `Scheduler` as proper `cSimpleModule` submodules (previously
  created via `new` in C++). They perform their own staged initialization.

- **UPF and PgwStandard** now derive from INET's `ApplicationLayerNodeBase`.

- **PacketFlowObserver refactored to use OMNeT++ signals**: Direct C++ calls
  from PDCP, RLC, and MAC into `PacketFlowObserver` were replaced with
  OMNeT++ signals, fully decoupling the observer from protocol modules.

- Replaced method-call-based packet passing with gate connections in several
  places: `LteHandoverManager`, `DualConnectivityManager`, `Ip2Nic` (X2 path).

### Type safety improvements

- **Strong typedefs**: `SIMU5G_STRONG_TYPEDEF` macro applied to `MacNodeId`,
  `DrbId`, `LogicalCid`, and `Qfi`, preventing accidental mixing of ID types.

- **Direction enum**: `LteControlInfo.direction` changed from `unsigned short`
  to a proper `Direction` enum.

- **C++ types extracted**: Types previously defined in `LteCommon.msg` were
  moved into a dedicated `LteTypes.h` header.

- **ROHC header**: PDCP header compression now uses a proper ROHC header
  representation instead of simply truncating the IP header.

- `FlowControlInfo`: `lcid` field renamed to `drbId`.

### Naming and layout cleanup

- Gate renames throughout the NIC for clarity and consistency:
  `MAC_to_RLC`/`RLC_to_MAC` -> `upperLayerIn`/`upperLayerOut` and
  `macIn`/`macOut`; `MAC_to_PHY`/`PHY_to_MAC` -> `phyOut`/`phyIn`;
  `filterGate` -> `dnPppg`. Several `inout` gates split into separate `input`
  + `output` gates.

- Submodule renames: `pdcpUpperMux` -> `pdcpMux`, `rlcLowerMux` -> `rlcMux`,
  `pppIf` -> `dpPpp` (in UPF/PGW).

- Module renames: `DualConnectivityManager` -> `DcX2Forwarder`,
  `LteHandoverManager` -> `HandoverX2Forwarder`.

- Improved NED layout of NIC internals for better visualization in Qtenv:
  data-path modules arranged vertically, control-plane modules on the left
  edge, dynamically created PDCP/RLC entities positioned between muxes.

### Bug fixes

- `LteSchedulerEnb`: Fixed multi-UE starvation in multi-DRB scheduling.

### Other

- Added `tilx` fingerprints (resistant to module renames) to the fingerprint
  test suite. Fingerprint test coverage for MEC simulations improved.

- `SplitBearersTable` turned into `std::ordered_map`.


## v1.4.5 (2026-07-09)

This release is a collection of bug fixes to the physical-layer error model
(CQI and BLER computation), the MAC layer, and the uplink scheduler. Several of
these fixes change simulation results for the affected configurations.

Tested with INET-4.5.4 and OMNeT++ 6.3, compatible with INET-4.6.0 and OMNeT++
6.1 through 6.4.

### PHY error model fixes

- **BLER table indexing**: `GetBLER_TU()`/`GetBLER_AWGN()` indexed the CQI/BLER
  tables one row too low, making throughput results slightly optimistic.

- **CQI boundary condition**: An SNR exactly at the minimum (`minSnr`, -14 dB)
  wrongly yielded the maximum CQI 15 instead of CQI 0, scheduling a noise-floor
  UE at the highest MCS.

- **CQI 0 handling**: `LteRealisticChannelModel::error()` no longer treats CQI 0
  (a valid "channel unusable" value, e.g. just after handover) as a fatal error;
  the packet is simply dropped.

### MAC and scheduler fixes

- **HARQ process count**: The `harqProcesses` NED parameter (whose NR default is
  5) was ignored by the C++ code, which used a hardcoded value of 8. The code now
  honors the parameter, so NR uses 5 HARQ processes as v1.4.4 already intended.
  Contributed by Esteban Egea Lopez (Universidad Politécnica de Cartagena).

- **RAC grant sizing**: A UE completing RACH on a poor uplink channel could get a
  grant too small to even carry a Buffer Status Report, leaving it unable to
  report its buffer and re-RACHing forever. Grants are now sized to at least 56 B.

### Other fixes

- **PacketFlowObserverEnb**: An unknown grant ID on an uplink MAC PDU (normal
  during handover) now logs a warning instead of throwing a fatal error.

- **PacketFlowObserver**: BSR-only MAC PDUs (no RLC SDUs) are now tolerated
  instead of triggering a fatal error.

- **AmcPilotAuto**: Using it with a D2D direction now fails with a clear error
  message; scenarios with D2D should set `amcMode="D2D"`.


## v1.4.4 (2026-05-24)

This release contains bug fixes and improvements, including more realistic MAC
layer modeling and several MEC fixes.

Tested with INET-4.5.4 and OMNeT++ 6.3, compatible with INET-4.6.0 and OMNeT++
6.1 through 6.4.

### MAC improvements

- **RACH preamble collision modeling**: UEs now pick a random preamble index
  when sending Random Access Channel (RAC) requests. The eNB detects collisions
  when multiple UEs choose the same preamble in a TTI, causing all colliding
  requests to fail. Failed UEs exercise the existing backoff/retry path. A new
  NED parameter `numPreambles` (default 64) controls the preamble pool size.
  While this is an abstraction of the real multi-step RACH procedure, it
  faithfully captures preamble contention (the primary source of access
  failures) with minimal additional model complexity.

- **NR MAC timer defaults adjusted**: `raResponseWindow` changed from 3 to 20,
  `retxBsrTimer` from 40 to 320. The original values were the LTE defaults, too
  aggressive for NR's finer timing granularity. Also adjusted `maxRacAttempts`
  (10) and `racBackoffMax` (20) for both LTE and NR.

- **Default HARQ processes for NR changed to 5** (from 8), reflecting the
  asynchronous nature of NR HARQ.

- **RAC/BSR timer parameters are now configurable from NED** (previously
  hardcoded).

### Bug fixes

- **Ip2Nic**: Fixed issue #302 -- packets arriving at the old gNB just after a
  handover are now forwarded to the new gNB over X2, instead of causing an error.

- **Ip2Nic**: Fixed fallback to NR node ID when a UE has no LTE ID, which is
  necessary for handling NR-only UEs.

- **MAC**: Fixed ASSERT failure on D2D mode switching (`SinglePair-modeSwitching`
  scenario). When switching from DM to IM mode, the MAC connection structure is
  now preserved with empty buffers instead of being destroyed, so that switching
  back to DM mode works correctly.

- **GtpUserX2**: Fixed `GtpUserMsg` chunk length to 8B, consistent with `GtpUser`.

- **MecOrchestrator**: Fixed `contextIdCounter` never being incremented, causing
  every MEC app to overwrite the previous map entry at key 0.

- **MecOrchestrator**: Fixed missing return after a failure path that caused an
  end-iterator dereference.

- **MecAppBase**: Fixed undisposed `HttpMessageStatus` objects on destruction.

- **MecResponseApp, MecRTVideoStreamingReceiver**: Fixed missing `localUePort`
  parameter that was inadvertently removed during earlier refactoring.

- **RniService**: Fixed incomplete CamelCase renaming that caused the service to
  not be found in the registry.

- **MEC**: Fixed uninitialized variables that caused non-deterministic fingerprint
  failures in debug builds.

### Other changes

- **Binder**: `getNextHop()` renamed to `getServingNodeOrSelf()` for clarity.

- **UDP error handling**: Refactored several application modules (including
  UeWarningAlertApp, UeRnisTestApp, UeRequestApp, and others) to use
  `UdpSocket::ICallback`, fixing errors when receiving ICMP "destination
  unreachable" indications. MEC apps now also properly close their UDP sockets.

- **Copyright headers adjusted**: Replaced generic "Authors" lines with precise
  copyright lines and added SimuLTE copyright attribution to files derived from
  SimuLTE.

- **INET 4.6 compatibility**: Added `checksumMode` parameter to emulation
  examples alongside the existing `crcMode` for backward compatibility.

- **TrafficLightController**: Made backward compatible with OMNeT++ 6.1.


## v1.4.3 (2026-02-18)

This release represents a major milestone in the complete overhaul of the Simu5G
codebase to make it architecturally more compliant with the 3GPP
specifications, modernize the code, and adopt the best practices of the INET
Framework on which it is based. The goal is to pave the way for a clean
implementation of new protocol features such as TSN support.

Tested with INET-4.5.4 and OMNeT++ 6.3, updated for INET-4.6.0 compatibility.

Key achievements in this release:

- **More explicit Control Plane modeling**: Simu5G is advertised as a User Plane
  simulator, but since it was also used to model dynamic scenarios such as
  handovers, it always contained elements of the Control Plane distributed across
  various modules. The new direction is to make these elements more explicit and
  centralized, such as creating dedicated RRC (Radio Resource Control) and
  Session Management Function (SMF) implementations. It is an explicit non-goal
  to simulate Control Plane messaging -- its functionality will be implemented
  with C++ method calls across modules. Thus, Simu5G remains a User Plane
  simulator, but with the possibility to more faithfully model dynamic
  scenarios with heavy Control Plane involvement. While this goal is not fully
  realized in this release, many changes point into that direction.

- **Control info refactoring**: Cleaned up `UserControlInfo` and
  `FlowControlInfo` by removing 5+ unused fields and splitting out smaller,
  focused tags. For example, IPv4 addresses, only used between `Ip2Nic` and PDCP,
  have been factored out into an `IpFlowInd` tag. This improves modularity,
  reduces coupling between protocol layers, and makes the code easier to
  maintain and extend.

- **Added vital missing fields to PDCP and MAC headers**: Protocol layers now
  use proper header fields instead of "tunnelling" information via
  `UserControlInfo` and `FlowControlInfo` packet tags that would not exist in a
  real implementation. For example, PDCP sequence numbers are now carried in
  PDCP headers, and LCIDs are stored in MAC PDU subheaders. This makes the
  simulation more realistic and packet contents more inspectable in Qtenv.

- **Explicit setup of logical connections instead of on-the-fly discovery**:
  This is a key architectural change, which also largely motivated the
  previous items. In previous iterations of Simu5G, data structures associated
  with logical connections / bearers were created in each protocol layer as they
  encountered packets that belonged to new connections. Moreover, part of the
  connection state was carried along by the packets in `FlowControlInfo` tags
  instead of stored inside the protocol. While this modeling approach still
  allowed for faithful simulation of the traffic while keeping the
  implementation simple, it has become a roadblock for implementing complex
  dynamic scenarios where connections come and go. In this iteration,
  centralized session and bearer management (SMF-like functionality) was added
  to the `Binder` module, and RRC modules were added to NICs to carry out local
  configuration. This brings the architecture closer to the 3GPP control/user
  plane separation, making it easier to implement features like handovers
  correctly. This is work in progress: SMF is still part of `Binder` and not a
  separate module, and connection setup is still triggered by the first packet
  of the connection hitting PDCP on the way out. However, moving the SMF code
  into its own module will be trivial, and the single `Binder` method call in
  PDCP can now be easily replaced with static configuration or with calls from a
  more detailed Control Plane implementation.

- **Removed incomplete MIMO support**: Removed MIMO-related code and parameters.
  The existing MIMO code was incomplete (e.g., PMI values were computed but
  never used). Removing it simplifies the codebase and model parameterization,
  and avoids confusion about capabilities. MIMO support will be added in a future
  release, with a different approach.

- **Initialization cleanup**: Reorganized module initialization into well-defined,
  Simu5G-specific init stages. This eliminates hidden cross-module dependencies,
  makes the initialization order explicit and verifiable, and prevents subtle bugs
  caused by modules accessing uninitialized data in other modules.

Further notable changes:

- In UE models, `masterId` and `nrMasterId` were renamed to `servingNodeId` and
  `nrServingNodeId`. The old names were confusing because "Master" has a
  specific meaning in Dual Connectivity (Master eNB vs Secondary gNB), unrelated
  to the UE's serving node.

- In UE models, the `macCellId`, `nrMacCellId` parameters were removed. In
  practice, the code already used the serving node ID as cell ID.

- `macNodeId` assignment was moved to NED, and now it is based on the new
  `simu5g_seq()` NED function that generates an integer sequence. This replaces
  the earlier approach where node IDs were assigned by `Ip2Nic` during
  initialization, and stored back into the module parameters for other modules to
  use.

- `LteRlcPduNewData` and `LteRlcSdu` packet chunks were converted to packet
  tags, as they represent internal metadata rather than actual protocol data.

- In the C++ code, merged the `ENODEB` and `GNODEB` node type enum values into a
  single `NODEB` value, with a separate `isNr` flag where needed. This change
  simplified a large number of "if" conditions throughout the codebase.

To port your existing Simu5G simulations to this version, apply the following
changes to the ini files:

- Change `masterId` to `servingNodeId` (and `nrMasterId` to `nrServingNodeId`),
  unless it refers to the Master/Secondary distinction in a Dual Connectivity
  setup.

- Remove `macCellId` and `nrMacCellId` parameter assignments for UE modules.

- Delete ini entries that set the following removed MIMO-related parameters:
  `numRus`, `ruRange`, `ruStartingAngle`, `ruTxPower`, `antennaCws`,
  `muMimo`, `pmiWeight`, `lambdaMinTh`, `lambdaMaxTh`, `lambdaRatioTh`,
  `feedbackGeneratorType`.

- For `initialTxMode`, the following values are no longer valid:
  `SINGLE_ANTENNA_PORT5`, `OL_SPATIAL_MULTIPLEXING`, `CL_SPATIAL_MULTIPLEXING`,
  `MULTI_USER`. Remove the parameter assigment to use the default.

There are many more changes that potentially affect existing simulations, and
projects extending, or built on top of, Simu5G. They cannot all be covered here
in detail - see the git history for details.


## v1.4.2 (2025-11-27)

This is primarily a bugfix release.

- Pdcp: Fixed Dual Connectivity bug where separate PDCP entities were
  incorrectly created for LTE and NR legs of a Split Bearer instead of using a
  single shared entity. This fix breaks RLC-UM packet loss statistics which
  (incorrectly) inferred packet loss from PDCP sequence numbers.

- RlcUm: Removed packet loss statistics that incorrectly relied on PDCP sequence
  numbers (PDCP sequences are not contiguous in Dual Connectivity setups)

- PacketFlowManager: Renamed to PacketFlowObserver, updated NED documentation.

- Statistics collection refined, e.g. remove recording "sum" and/or "mean" where
  it does not make sense; use new "rateavg" filter for computing average
  throughput.

- Binder: New utility functions: isGNodeB(), getUeNodeId().

- Apps: Added sequence numbers to VoIP and VoD packet names, to facilitate
  tracing with Qtenv.

- NED documentation: Added content to simu5g-index.ned including version number
  and WHATSNEW.


## v1.4.1-sdap-2 (2026-03-03)

This release improves on the simu5g-1.4.1-sdap release that added SDAP (Service
Data Adaptation Protocol) layer support to Simu5G. Contributed by Andras Varga
(OMNeT++ Core Team).

The most important changes:

- New simulations that exercise the code more: Multi-UE, multi-app, multi-QFI
  configurations were added into `omnetpp_drb.ini` under `nr/standalone`. Based
  on the Simu5G#294 bug report by Jonathan "Toaaster" Ebert.

- Fixed QFI propagation: QFI was originally added to packets by the application
  (`VoipSender`) as a packet tag (`QfiTag`). However, this tag did not make it to
  UPF, because it was already stripped by the local PPP interface on
  transmission. This mechanism was replaced by the VoipSender app setting DSCP
  on the packet, which UPF now interprets as QFI (simplified PDR matching). From
  then on, QFI is now carried through the GTP-U tunnel in the GTP header
  (mirroring the real 5G PDU Session Container extension header), instead of
  relying on QoS tags that were being stripped by PPP. The gNB extracts QFI from
  the GTP-U header to restore QoS tags for SDAP routing.

- DRB configuration changes: The DRB configuration is now split between SDAP and
  MAC layers, each only knowing as much as they need for their operation.
  QFI-to-DRB routing configuration went into `sdap.drbConfig`, while QoS
  parameters for the scheduler (GBR, delay budget, PER, priority) went into
  `mac.drbQosConfig`. Moreover, DRB configuration is now specified in JSON,
  replacing the previous text file-based configuration.

- Fixed multi-UE DL starvation (fixes Simu5G#294): `MacDrbMultiplexer`
  incorrectly used LCID as the `nrRlc[]` array index, assuming LCID equals the
  DRB index. When multiple UEs shared the same DRB, only the first UE received
  data. Fixed by learning the LCID-to-gate mapping from RLC-to-MAC traffic.

- MEC fixes: There were several bug fixes in the MEC code, such as
  `MecOrchestrator` (contextId counter was never incremented), `MecOrchestrator`
  (missing return after failure path causing end-iterator dereference),
  `MecAppBase` (eliminate undisposed objects), fix uninitialized variables in
  various modules (fixing long-standing fingerprint failures of certain MEC
  simulations in debug mode).


## v1.4.1-sdap (2025-10-06)

Compatible with OMNeT++ 6.2.0 and INET 4.5.4.

This specialized branch release introduces SDAP protocol support, multiple DRBs
and advanced QoS capabilities to Simu5G for enhanced 5G network simulations.
Please note that future main releases may not include these features or may
incorporate them in a different form, as the primary development focus remains
on architectural refactoring and foundational improvements. The changes were
contributed by Mohamed Seliem (University College Cork), with improvements by
Andras Varga (OMNeT++ Core Team).

Reference paper: "QoS-Aware Proportional Fairness Scheduling for Multi-Flow 5G
UEs: A Smart Factory Perspective". Mohamed Seliem, Utz Roedig, Cormac Sreenan,
Dirk Pesch. IEEE MSWiM, 2025.

New Features:

- Added an SDAP protocol implementation with reflexive QoS capabilities (NrSdap
  and ReflectiveQosTable modules). Available using the NRUeSdap (UE) and
  gNodeBSdap (gNodeB) node types that contain the NRNicUeSdap and NRNicEnbSdap
  NIC types, respectively.

- DRB (Data Radio Bearer) support with multi-QFI/QoS handling for realistic 5G
  bearer management simulations. This feature is available using NRUeDrb (UE)
  and gNodeBDrb (gNodeB) node types that contain the NRNicUeDrb and NRNicEnbDrb
  NIC types, respectively. It can be configured using the numDrbs parameter.
  QFI-to-DRB mappings can be defined in a context file (see SDAP's
  qfiContextFile parameter) with 5QI parameters and QoS requirements.

- QoSAwareScheduler with QFI-based Proportional Fair scheduling using QfiContextManager.
  Enable QoS scheduling with LteMacEnb.schedulingDisciplineDl/Ul="QOS_PF".

- Better representation of compressed headers in PDCP. (Note that header compression is
  disabled by default; enable using PDCP's headerCompressedSize parameter.)

- New example simulations: simulations/nr/standalone/omnetpp_sdap.ini and omnetpp_drb.ini,
  each with Standalone, VoIP-DL, and VoIP-UL configurations demonstrating SDAP functionality
  and multi-DRB support with QoS-aware scheduling.


## v1.4.1 (2025-10-06)

This is a minor update that brings further refactoring of the C++ code for clarity,
improvements in the C++ interface of the Binder module, and some minor bug fixes.
These improvements were contributed by Andras Varga (OMNeT++ Core Team).

Notable changes:

- Binder: Partial rationalization of the C++ interface, via
  renaming/replacing/removing methods. See the git history for changes.

- Updated IP addresses in the IPv4 configuration files: use 10.x.x.x
  addresses for the Core Network, and 192.168.x.x addresses for external
  addresses

- Visual improvement: node IDs are now displayed over module icons

- In MEC, do not use module IDs for bgAppId, deviceAppId and other IDs, and do
  not encode module ID into module names. That practice made simulations brittle
  for regression testing via fingerprints.

- PDCP: Eliminated tweaking of srcId/destId in FlowControlInfo when sending
  downlink packets over the X2 link in a Dual-Connectivity setup.

- Various additional fixes and changes to improve code quality.


## v1.4.0 (2025-09-18)

Compatible with **OMNeT++ 6.2.0** and **INET 4.5.4**.

This release marks an important milestone in the ongoing transformation of
Simu5G. While not introducing behavioral changes, this intermediate release
focuses on restructuring the codebase to improve clarity, safety, and
maintainability. Major updates include a reorganized directory structure,
enforcing a consistent naming convention, making make packets more easily
inspectable, and refactoring of parts of the C++ code to pave the way for
changes in future versions. Although the release is not source-compatible with
previous versions, existing simulations will continue to work unchanged once
adjusted to follow the various rename operations. These improvements were
contributed by Andras Varga (OMNeT++ Core Team).

Renames:

- Sources are now under src/simu5g/ instead of just src/, so that C++ includes
  start with "simu5g/". This helps identifying Simu5G includes when Simu5G is
  used as a dependency of other projects.

- Some folders were moved inside the source tree to a more logical location. For
  example, the simu5g/nodes/mec/ subtree was promoted to simu5g/mec/.

- Several source folders were renamed to more closely follow the all-lowercase
  convention. For example, mec/UALCMP/ became mec/ualcmp/, and mec/MECPlatform
  became just mec/platform.

- Several classes were renamed to ensure that only the first letters of acronyms
  are uppercase. For example, MECHost became MecHost.

- Several parameters were renamed to enforce camelcase names. For example,
  bs_noise_figure became bsNoiseFigure, and fading_paths became numFadingPaths.
  If you have existing Simu5G simulations, review the ini files carefully and
  update the parameter assignments accordingly. (Caveat: Assignment lines that
  refer to the old names will be simply ignored by the simulation -- there is no
  error message for that!)

- The PdcpRrc modules were renamed to just Pdcp. Likewise, pdcpRrc submodules
  in NIC compound modules became pdcp.

Further refactoring:

- Several protocol header classes, while defined in msg files, contained heavy
  customization in C++ code, including the addition of new fields. Since the
  writing of those classes, the message compiler in OMNeT++ gained enough
  features so that most of the customizations were no longer needed, and the
  desired effect could be achieved in msg files only. This refactoring has the
  benefit of making packets more inspectable from Qtenv, and packet contents can
  now be serialized using parsimPack (useful for more thorough fingerprint
  tests).

- MacCid is a central data type that pairs an LCID with a nodeId to uniquely
  identify a logical channel. It used to be a packed integer, and now it was
  turned into a C++ class with separate fields for the node ID and LCID and with
  accessor methods, for increased type safety.

- carrierFrequency used to be a variable of the type double throughout the
  codebase. The type was changed to GHz (using INET's units.h) for increased
  type safety. This also helped identifying a bug in certain channel models
  (LteRealisticChannelModel, BackgroundCellChannelModel) where a double
  representing GHz instead of Hz was used in computing path loss, resulting
  in underestimated path loss values. [Correction from a later release: the
  "bug" was not one, and the change was reverted. The affected terms,
  20log10(40 pi d fc/3) in the RMa/SMa LOS path loss, take fc in GHz: they
  are the free-space term 20log10(4 pi d f/c) with the unit conversion
  folded into the constants, so evaluating them in Hz overstated the LOS
  path loss by 180 dB. The formulas were unreachable from the example
  simulations at the time, so no published result was affected.]

- Binder received several WATCHes for increased transparency in Qtenv, and
  an overhaul of a subset of its API and internal data structures.

- In the Pdcp modules, the unused EUTRAN_RRC_Sap port (and associated handling
  code) was removed.

- Refactoring of internals in several protocol modules, including MAC, PDCP and
  RLC implementations.

Build:

- Made the command line build consistent with the IDE build. src/Makefile is now
  generated/updated implicitly on every build, no need to type "make makefiles".

## v1.3.1 (2025-09-18)

This is a minor update for Simu5G-1.3.0. In addition to fixing regressions
in the previous release and making some cosmetic improvements, the main highlight
of this release is the revamp of the fingerprint test suite, which now provides
a more comprehensive safety net against future regressions. Changes in this
release were contributed by Andras Varga (OMNeT++ Core Team).

Changes:
- Example simulations: Marked abstract configs as such (abstract=true) in omnetpp.ini files
- FlowControlInfo's MacNodeId fields are now properly shown in Qtenv object inspectors
- Replaced EV_ERROR << lines with throwing cRuntimeError
- TrafficLightController: fixed startState NED parameter (also changed type from int to string)
- Fingerprints: CSV files merged into simulations.csv, added missing simulations,
  standardized on the set of fingerprints computed (tplx, ~tNl, sz), translated
  gen_runallexamples.py into Python and improved it

Fix regressions in v1.3.0:
- MECResponseApp: fixed wrong @class annotation
- BackgroundScheduler: fix "binder_ not initialized" error
- MecRequestForegroundApp, MecRequestBackgroundGeneratorApp: add back lost parameter defaults
- BackgroundCellTrafficManager: fix "Not implemented" thrown from getBackloggedUeBytesPerBlock()
- tutorials/nr/omnetpp.ini: fix missing unit for txPower parameter (dBm)

## v1.3.0 (2025-02-06)

- Compatible with **OMNeT++ 6.1.0** and **INET 4.5.4**
- New modules: MultiUEMECApp, MecRnisTestApp, UeRnisTestApp
- Added NED documentation for modules
- Increased reusability of modules via changes such as replacing hardcoded module
  paths in the C++ code with NED parameters (binderModule, macModule, etc.), and
  elimination of ancestorPar() calls by introducing local parameters instead. *
- Other NED adjustments, such as removal of unused NED parameters and splitting
  NED files to have one module per file. See doc/NED-changes.txt for details. *
- Extensive C++ modernization, and adaption of more OMNeT++ best practices. *
- Various bug fixes.
- Changes marked with an asterisk were contributed by Andras Varga (OMNeT++ Core
  Team).

## v1.2.3 (2025-01-10)

- Added support for **OMNeT++ 6.1.0** and **INET 4.5.4**.

## v1.2.2 (2023-04-19)

- Compatible with **OMNeT++ 6.0.1** and **INET 4.5**.
- Tested on Ubuntu 22.04 and macOS Ventura.

## v1.2.1 (2022-07-19)

- Compatible with **OMNeT++ 6.0** and **INET 4.4.0**.
- Tested on Ubuntu 20.04.
- Modifications to support the latest versions of OMNeT++ 6.0 and INET v4.4.0.
- Refactoring of simulation and emulation folders.
- Various bug fixes.

## v1.2.0 (2021-08-30)

- Compatible with **OMNeT++ 6.0 (pre10 and pre11)** and **INET 4.3.2**.
- Tested on Ubuntu 16.04, 18.04, 20.04, macOS Catalina, and Windows 7.
- Added modelling of **ETSI MEC** entities.
- Support for **real-time emulation capabilities** (Linux OS only).
- Modelling of background cells and background UEs for **larger scale** simulations and emulations.
- Several bug fixes.

## v1.1.0 (2021-04-16)

- Compatible with **OMNeT++ 5.6.2** and **INET 4.2.2**.
- Tested on Ubuntu 16.04, 18.04, 20.04, macOS Catalina, and Windows 7.
