# Simu5G channel and PHY model: recorded deviations

Deviations of the channel and PHY model from the 3GPP specifications, and defects
of the model, found during the acceptance of the radio medium branch
(radio-medium-acceptance.md). Checked against ETSI TR 138 901 V16.1.0, TR 36.814
V9.2.0, TR 36.873 V12.7.0, TR 36.843 V12.0.0 and TR 37.885 V15.3.0.

They move fingerprints, so they were recorded rather than fixed during the
acceptance, and are addressed after the whole branch has been merged. Until then
the unit tests record a deviation, where one pins it, as a passing check labelled
"PINNED DEFECT:" and a separate test with %expected-failure that asserts the
specification.

"Later in the branch" names the commits of the radio medium branch that touch the
same mechanism, so that nothing is fixed twice.

## Deviations from the specification

1. **The shadowing correlation over distance is wrong.**
   - What the model does: StochasticChannelModel::computeShadowing() updates a moving UE's shadowing as `a*old + sqrt(1-a^2)*N(0,sigma)` with `a = exp(-0.5*d/dcorr)`. It only updates once the UE has moved more than dcorr, and holds the value until then.
   - What the spec says: TR 38.901 7.4.4, eq. (7.4-5): R(dx) = exp(-dx/d_cor), with d_cor from Table 7.5-6, "correlation distance in the horizontal plane". TR 36.873 7.2.4 and TR 36.814 B.1.2.1.1 say the same (Gudmundson, ITU-R P.1816). No clause has a 0.5 factor or a hold rule.
   - Effect: the 0.5 doubles the correlation length, and the hold makes R = 1 below d_cor and the process stepwise.
   - Fix shape: `a = exp(-d/dcorr)`. Then decide between updating at every evaluation with that a, or interpolating ("may be interpolated within the coherence length", 7.6.3.2).
   - Later in the branch: "a link's shadowing is redrawn by how far its ends have moved, not by time and speed" keeps the factor "as before", and "a link's shadowing is kept once per link, shared by every channel model evaluating it" makes the shadowing per link. Neither fixes this.
   - Pinned by tests/unit/ChannelModelShadowing.
2. **The thermal noise does not depend on the bandwidth, and the transmit power is not split over the RBs.**
   - What the model does: thermalNoise is a fixed parameter, -104.5 dBm by NED default, which is kTB for 50 RBs at 15 kHz (9 MHz). Every band's SINR gets the node's full transmit power.
   - Effect: with numBands = 6 the physical floor is -113.7 dBm (15 kHz SCS) or -110.7 dBm (30 kHz), so the SINR is 9.2 or 6.2 dB too low. And the uplink SINR does not depend on how many RBs a UE is given.
   - Fix shape: derive N from numBands, the numerology and the RB width; take a decision on the per-RB power (PSD) convention.
   - The physics oracle copies the convention, so it cannot see it.
3. **Rayleigh fading is not fading.**
   - What the model does: rayleighFading() is a lookup in the one PhyPisaData table shared by the whole simulation, indexed by band only. Every node on a band gets the same value, the value never changes over time, and no random number is drawn.
   - Used by: channelmodel Rayleigh-38901, and any config with fadingType = RAYLEIGH.
   - Pinned by tests/unit/ChannelModelJakesFading, as "tripwires".
4. **The UE-to-UE (D2D) path loss uses base-station conventions.**
   - What the model does, as the D2dLinkBudget oracle reproduces it: the 38.901 UMi breakpoint is computed with the gNB height (hBS = 10 m, so d'BP = 120 m), and cableLoss (2 dB) is subtracted on a link with no base station.
   - What the specs say: TR 36.843 A.2.1.2 gives h_BS = h_MS = 1.5 m (effective 0.8 m), and TR 37.885 Table 6.2.1-1 reuses the V2V path loss for P2P (urban LOS: 38.77 + 16.7 log10 d + 18.2 log10 fc).
   - At 50 m and 2 GHz: the model gives 74.10 dB, TR 37.885 72.62 dB, and 38.901 UMi with both heights at 1.5 m 90.73 dB.
   - D2D is legacy (keep alive, no features). Decide whether this is worth fixing at all.
5. **The scenario geometry is outside the formulas' validity.** The physics tests (and possibly examples) run URBAN_MICROCELL with nodebHeight = 25 m. TR 38.901 Table 7.4.1-1 UMi-SC gives "hBS = 10 m", and TR 36.814 Table B.1.2.1-1 UMi gives "hBS = 10 m". This is a configuration issue, not code; check the shipped examples.
6. **The external- and background-cell interference was not evaluated with the model's own path loss.**
   - What the model did: computeExtCellPathLoss always applied TR 36.814 with the 3D distance as d, NLOS by configuration, and antennGainEnB.
   - Later in the branch: "external and background cells' interference takes the model's own path loss" replaces this. Re-check after the merge whether anything remains, e.g. shadowing and LOS for these links (stage B gives phantom links their own LOS state in "a phantom transmission's link has its own LOS state").
   - From piece 5a on, these links take the evaluating model's own study. What remains at the end of piece 5a: no shadowing on these links, and the formulas take the nodebHeight and ueHeight parameters while the distances come from the real positions.

## Data and arithmetic defects

7. **The BLER curve ends in a cliff.** GetBLER_TU returns 0 at and above each curve's last tabulated point, and never reads BLER_15_CQI_TU[row][15]. For CQI 11-15 that value is an error floor (e.g. 0.02), so the lookup goes from 0.06 at 20 dB to 0 at 21 dB for CQI 11. Pinned by tests/unit/BlerCqiRoundTrip as the "current contract".
8. **The BLER curves are not monotonic.** BLER_15_CQI_TU rises with SINR at CQI 1 (-1.5 to -0.5 dB), CQI 4 (4 to 5 dB) and CQI 5 (-3 to -2 dB). Recorded in tests/unit/BlerCurveMonotonicity as an expected failure, which counts 2 offenders at 1 dB sampling. Fixing it is a modelling decision about the curves.
9. **The CQI and the error model read the SINR differently.** getCqi rounds the SINR (floor(snr+0.5)), while isReceptionSuccessful truncates it toward zero (int). So they disagree by up to 1 dB about which curve point applies, and -14.9 dB is judged as -14 dB, inside the curves. Pinned by BlerCqiRoundTrip and ChannelModelReception.
10. **The azimuth is wrong whenever the heights differ.** computeAngle() takes asin(dy/d) with d the 3D distance. A UE due north of a 25 m mast at 50 m comes out at 64.8 degrees instead of 90. Fix: atan2 on the horizontal components. Recorded in tests/unit/ChannelModelAngleHeights as an expected failure. In the suite only channelmodel/Anisotropic-36873 would move.
11. **The SINR statistic leaves out the worst receptions.** A reception lost to CQI 0, or to a band below the curves, returns before rcvdSinr is emitted, so the statistic and the recorded results never see those receptions. Fix shape still to be agreed.

12. **Part of the fading table is never filled (latent).** PhyPisaData's constructor sizes channel_ to 10000 entries and fills only the first 1000. getChannel(i) wraps i modulo 10000, so any index from 1000 to 9999 returns 0, which is -inf dB. Both callers index by band only (StochasticChannelModel::rayleighFading and BackgroundCellChannelModel), so this is reached only with 1000 or more bands. Found while mutation-checking the Rayleigh test.

## Found in piece 4 (2026-11-06): fix after the full merge

Recorded, not fixed in the branch; addressed after the whole radio branch is
merged, together with the items above. Each fix moves
fingerprints. In the branch they get test hardening only (pins and expected failures).

13. **A D2D multicast reception keys its interferers on the wrong link.**
   - What the model does: a D2D_MULTI flow sets its destination id to the transmitter's own id (Ip2Nic), and the receiver's SINR evaluation takes the destination id as its own id (StochasticChannelModel::getReceptionSinr). Each interferer X is evaluated on LinkKey(X, transmitter), with the geometry of X to the local receiver; with the capture factor "Distance" the desired signal is evaluated on LinkKey(transmitter, transmitter).
   - Effect: since "a link's LOS state is kept once per link" (radio stage B), that entry is one medium entry, shared by every multicast receiver and by the real X-to-transmitter link: one LOS state, shadowing sample and set of Jakes paths for physically different links. On dev the wrong key lived in each receiver's own model.
   - Reachable in simulations/lte/d2d_multicast (d2dInterference defaults to true). Moves those rows.
   - Fix shape: the receiving radio's own id for D2D_MULTI.
14. **A UE with two radios has two indoor positions.**
   - What the model does: each CellularRadio draws its own distance from the wall (uniform 0-25 m) and, from "the random term of a UE's building penetration loss is drawn once, the UE's own" on, its own penetration-loss deviate. An NR UE always has two radios (NrNicUe extends the LTE NIC: `radio` and `nrRadio`, the LTE one unused in standalone), and `**.insideBuilding = true` marks both, so every indoor NR UE draws twice; a dual-connectivity UE uses both.
   - What the spec says: TR 38.901 7.4.3.1 generates d2D-in and the O2I loss's random term per UT ("UT-specifically"), the same for all of the UT's links.
   - Effect: the indoor loss is 0.5 dB/m, so the two legs of one UE can differ by up to 12.5 dB for no physical reason; biases anything comparing a DC UE's LTE and NR legs indoors. On dev it was worse: one draw per channel model, i.e. per radio and per carrier.
   - Moves the four channelmodel Penetration rows (one draw fewer per UE on RNG 0).
   - Fix shape: one draw per UE, shared by its radios.
15. **A D2D link's indoor loss depends on its direction.**
   - What the model does: the penetration loss is the "UE end's", `link.dir == UL ? txRadio : rxRadio` (StochasticChannelModel::getAttenuation), so a D2D link takes the receiver's indoor state: A indoors, B outdoors gives no wall loss on A->B and the full wall loss on B->A. Two indoor UEs get the full outdoor-to-indoor loss.
   - What the spec says: a link's path loss is reciprocal; TR 36.843 A.2.1.2 uses the indoor model between two indoor UEs.
   - Fix shape: one indoor end means one wall; both indoor needs an indoor-to-indoor model. D2D is legacy: decide whether it is worth more than the reciprocal rule.
16. **The medium's per-link maps are never erased.**
   - What the model does: the LOS, shadowing and Jakes maps keep one entry per (UE, cell) pair and carrier, and per D2D pair, for the whole run (CellularRadioMedium.h: "Entries are never removed: node ids are not reused"). On dev the base-station side grew the same way; the UE-side state died with the UE's channel model.
   - Effect: memory grows with the number of UEs ever created (cars, Veins); Jakes entries are bands x paths x 16 B. A user-assigned macNodeId reused after a departure would inherit stale state.
   - Fix shape: erase a radio's links when it unregisters from the medium (fingerprint-inert: no row removes UEs).
17. **The indoor distance is one uniform draw.** TR 38.901 7.4.3.1 takes d2D-in as the minimum of two independent uniforms (0-25 m for UMa/UMi, 0-10 m for RMa); the model draws one uniform(0, 25 m) for every scenario. Pre-existing.

## Found in piece 5a (2026-11-06): fix after the full merge

Recorded, not fixed in the branch, like the items of piece 4; the tests of
piece 5a pin item 19.

18. **A background UE on a cell's second carrier occupies the wrong bands.**
   - What the model does: a background UE's allocation is keyed as the scheduler allocates, by the cell-wide band index (CellInfo lists every band of the cell for each carrier, -2 outside it), and the background traffic manager registers it with the medium unchanged. A radio's frame is registered with carrier-local band indices (LteSchedulerEnb::readRbOccupation() subtracts the carrier's first band). On dev the DL interference read the cell's band status, also cell-wide, with the victim carrier's band index.
   - Effect: with carriers of 6 and 10 bands, a background UE on cell band 6+j is taken as occupying band 6+j of the second carrier: the wrong band for j < 4, none for j >= 4. No row has background UEs on a cell's second carrier.
   - With uplink interference on, Binder::storeUlTransmissionMap() writes the same cell-wide band into a vector sized by the carrier's bands: for j >= 4 past its end, which is undefined behavior. The map is deleted later in the branch.
   - Fix shape: subtract the carrier's first band when the allocation is registered.
19. **A transmission between two slot boundaries prunes the slot completed last.**
   - What the model does: when a transmission is added, the medium drops every transmission of its carrier that ended before the new one's start minus its duration. A beacon takes no air time, so one sent between two boundaries drops the slot completed last, and a CQI computed in the rest of that slot sees none of its interference.
   - More generally, the medium takes a slot's transmissions by time, which is right only if every data transmission starts on the carrier's slot grid; nothing asserts it. A beacon off the grid also overlaps two of an external cell's 1 ms ticks, and its RSSI counts that cell twice.
   - Reached only when the beacon interval is not a multiple of the slot; the shipped intervals are. Latent.
   - Pinned by tests/unit/RadioMediumTransmissionRegistry; tests/unit/RadioMediumTransmissionRegistryOffGrid asserts what the medium should keep, as an expected failure.
   - Fix shape: keep what overlaps the slot before the current slot boundary, whatever the new transmission's duration; assert that data transmissions start on the grid.
20. **A background UE's uplink allocation is on the medium only if its own cell evaluates uplink interference.**
   - What the model does: LteSchedulerEnb registers a background UE's UL allocation with the medium only when its base station's channel model has uplinkInterference on, while every real UE's uplink frame is on the medium.
   - Effect: a neighbour cell with uplinkInterference on, or a D2D receiver, sees a cell's real UEs but not its background UEs when that cell's switch is off. The condition mirrored the Binder's uplink transmission map, which is deleted later in the branch.
   - Fix shape: register every allocation; the receiving model's switch decides what it counts.
21. **The handover beacon's RSSI counts the background cells' interference, but not the real cells'.**
   - What the model does: from "external and background cells interfere through the downlink and uplink interference computations" on, a beacon's reception, which has no bands, goes through the downlink interference computation with the other cells off and the external and background cells as their switches say.
   - Open: what the RSSI the handover decision compares is meant to include.

## Found in piece 5b (2026-11-07): fix after the full merge

Recorded, not fixed in the branch, like the items of pieces 4 and 5a.

22. **A car halted at a red light keeps its Doppler shift.**
   - What the model does: TrafficLightMobility leaves the car's velocity unchanged while it waits at a red light, and from "the Doppler speed is the mobile end's, from its mobility" on, the Doppler speed is the mobility's velocity. A waiting car fades as if it were driving at its cruising speed. The old two-sample estimate gave 0 there.
   - Reached in nr/videostreaming_dataset_generator (UrbanNetwork).
   - Fix shape: the velocity is zero while the car waits.
23. **A car's trajectory depends on how often its position is read.**
   - What the model does: at each update, TrafficLightMobility either stops the car for the whole time since the last update (light red) or moves it for the whole of it (light green). From "positions are the mobility's, at the instant they are read" on, every read of the position is an update, and positions are read at arbitrary instants (frames sent and received, CQIs, other nodes' evaluations), so the time a car waits, and where it is, depend on the reads.
   - Reached in the same row.
   - Fix shape: compute when the car reaches the stop line and when the light changes, independently of the update instants.
24. **The Jakes phase is the speed times the absolute time, and only one end's speed counts.**
   - What the model does: StochasticChannelModel::jakesFading() sums the paths with phase 2*pi*(cos(aoa)*v*f/c*t - tau*f), with v the speed at the evaluation and t the absolute simulation time. The phase that has accumulated up to t is not the integral of the Doppler frequency, so every change of speed (waypoints, TraCI steps, GaussMarkovMobility) jumps the phase by an amount growing with t, which re-randomizes the fading, and a UE that stops returns to the same static value whatever its path. The speed is that of the link's mobile end, the UE, or the transmitter of a D2D link: a moving base station's and a D2D receiver's are ignored, and the two directions of a D2D pair share one realization at two different speeds. On dev the phase was the same; the speed was an estimate.
   - What the physics says: a path's phase advances by the integral of its Doppler frequency over time, and the Doppler frequency has a term for each moving end.
   - The speed is read when the link is evaluated, while from "a link's Jakes fading is that at the start of the transmission evaluated on it" on the fading is taken at the transmission's start: when the speed changes in between (a reception decoded at its slot end after a waypoint, a TraCI step), a reception's fading depends on when it is evaluated.
   - Fix shape: keep the accumulated phase per link and path, advanced by the Doppler frequency over the time since the last evaluation, up to the transmission's start; take the velocities of both ends.
25. **The distance statistic is weighted by the reads of the position.**
   - What the model does: a UE's PHY emits its distance (and serving cell) statistics on the mobility's mobilityStateChanged signal. From "positions are the mobility's, at the instant they are read" on, the mobility emits that signal whenever anyone reads the position, so the mean of the distance is an average over the reads, not over time, and an unrelated module that reads the position more often changes it.
   - Fix shape: record the statistic time-weighted (timeavg), or emit it on a timer.

## Found in piece 6a (2026-11-07): fix after the full merge

Recorded, not fixed in the branch, like the items of the earlier pieces.

26. **The UMa environment height's probability of 1 m: TR 36.873 leaves out the distance, TR 38.901 draws at 18 m.**
   - What the model does: Tr36873PathLossModel::drawEnvironmentHeight() takes C(d2D, hUT) as ((hUT-13)/10)^1.5, without the distance term g(d2D), so the probability of h_E = 1 m is the same at every distance: 0.52 at hUT = 22.5 m. The same file's UMa LOS probability has the distance term. Tr38901PathLossModel takes g(d2D) = 0 only for d2D < 18 m, so at exactly 18 m it draws, and about 5 links in 1000 get an h_E other than 1 m.
   - What the specification says: TR 36.873 Table 7.2-1 NOTE 4 draws h_E = 1 m with probability 1/(1+C(d2D, hUT)), with the C of Table 7.2-2, which has the distance term: 0.98 at 30 m, 0.63 at 100 m and 0.29 at 200 m for hUT = 22.5 m, and 1 at 18 m or less. TR 38.901 Table 7.4.1-1 NOTE 1 has g(d2D) = 0 for d2D <= 18 m.
   - Reached in channelmodel TallUe-36873 (the only row with UEs above 13 m under TR 36.873); the 18 m boundary in no row.
   - Recorded in tests/unit/Tr36873EnvironmentHeightDistance and Tr38901EnvironmentHeightBoundary as expected failures; PathLossEnvironmentHeight pins the TR 36.873 distribution.
   - Fix shape: the C of the LOS probability in the TR 36.873 draw; d2D <= 18 m in the TR 38.901 one.
27. **A link's LOS state and h_E are drawn per cell and per carrier, not per site and UE.**
   - What the model does: the medium keeps a link's LOS sample, and with it h_E, per (cell, UE) pair and per carrier (CellularRadioMedium::getLosMap() per carrier frequency). A UE served on two carriers of one cell has two independent LOS states and h_E values, and the cells of one site (sectors) draw independently.
   - What the specification says: TR 38.901 7.6.5, "Propagation conditions generated in Step 2 are the same for all the frequencies"; Table 7.6.3.4-1 makes the LOS/NLOS state site-specific, and NOTE 1 of Table 7.4.1-1 determines h_E "for every link between BS sites and UTs".
   - Reached in the rows that serve a UE on two carriers of one cell (e.g. nr/test_multiCarrier), and in any network with co-sited cells.
   - Fix shape: key the LOS sample by site and UE, shared by every carrier.

## Found in piece 6b (2026-11-08): fix after the full merge

Recorded, not fixed in the branch, like the items of the earlier pieces.

28. **The CQI describes the channel of the slot it is first used for: it has no measurement delay.**
   - What the model does: the base station computes a UE's DL CQI, and a D2D pair's CQI, from the UE's report as a what-if on the medium at the instant the report's reception ends (from "a CQI's what-if is evaluated where the radios are now" on), so it takes the positions and the Jakes fading of the DL slot that starts then. Before that commit it took the report's start, one slot earlier. The UL CQI of the same report is the report's own reception, faded at its start, so the two halves of one report are a slot apart.
   - What the specification says: TS 38.214 V19.5.0 5.2.2.5: "In the time domain, the CSI reference resource for a CSI reporting in uplink slot n' [...] is defined by a single downlink slot n-nCSI_ref-Koffset[...]", "where for periodic and semi-persistent CSI reporting - if a single CSI-RS/SSB resource is configured for channel measurement nCSI_ref is the smallest value greater than or equal to 4*2^muDL, such that it corresponds to a valid downlink slot". TS 36.213 V19.5.0 7.2.3 defines the LTE CSI reference resource the same way, as "a single downlink subframe/slot/subslot [...] n-nCQI_ref".
   - Effect: where the channel changes within a few slots (moving UEs, Jakes fading), the CQI is more accurate than a real UE's, and link adaptation fares better than it would.
   - Reached in every row with moving UEs and CQI reporting.
   - Fix shape: evaluate the CQI at its CSI reference resource -- the positions and the fading nCSI_ref slots before the report -- for the DL, UL and D2D CQI alike.
29. **A UE inside a building has no penetration loss under TR 36.814, nor under TR 36.873 except in UMa.**
   - What the model does: only Tr36873PathLossModel's UMa and Tr38901PathLossModel add a building penetration loss. Tr36814PathLossModel, in every scenario, and Tr36873PathLossModel's UMi, RMa and InH (and SMa, which is TR 36.814's formula) take the link's conditions and do not use them, so `insideBuilding = true` on a UE silently changes nothing.
   - What the specification says: TR 36.873 7.2.3: "3D-UMi O2I PL = PLb + PLtw + PLin", "PLb = PL3D-UMi (d3D-out + d3D-in)", "PLtw = 20", "PLin = 0.5d2D-in", "0m < d2D-in < 25m". TR 36.814 Table B.1.2.1-1, UMi outdoor-to-indoor: "PLtw = 20, other values remain the same", "0 m < din< 25 m". The other scenarios of the two studies have no outdoor-to-indoor model.
   - Reached in no row: the Penetration rows use TR 38.901 and TR 36.873 UMa.
   - Fix shape: the O2I loss in TR 36.873 UMi and TR 36.814 UMi; an indoor UE rejected where the study has no O2I model.
30. **A moving base station interferes from where it is when the interference is evaluated, not from where its transmission started.**
   - What the model does: the DL interference walk evaluates another cell's link through StochasticChannelModel::cellularLink(), which reads the interfering cell's current position; the angular attenuation of the same walk and the uplink interference take the transmission's start position.
   - What the medium's model says: from "every radio's position is read as a transmission starts" on, a transmission's arrival is computed from where it started, as on INET's medium.
   - Reached in no row: no base station moves.
   - Fix shape: build the interfering cell's link from the transmission's start position.

## Fixed later in the branch, re-check only

- **The O2I loss's random term redrawn at every evaluation:** accepted with piece 6a. Drawn once per UE from "the random term of a UE's building penetration loss is drawn once, the UE's own"; see item 14 for the per-radio residue.

- **Shadowing per direction (LinkReciprocity):** made per link by "a link's shadowing is kept once per link, shared by every channel model evaluating it".
- **Jakes fading evaluated at now - 1 ms:** accepted with piece 6a, fixed by "a link's Jakes fading is that at the start of the transmission evaluated on it"; see item 24 for the speed read at the evaluation. The background cells' channel model keeps now - 1 ms until it is deleted later in the branch.
- **The Doppler speed from a two-sample estimate:** accepted with piece 5b. The estimate becomes the real speed with "positions are the mobility's, at the instant they are read", and the speed is the mobility's from "the Doppler speed is the mobile end's, from its mobility"; see items 22 and 24 for what remains.

## Not a deviation

- **ModuleOrderPermutation** asserts per-realisation invariance under relabelling of UEs. That is a common-random-numbers (RNG design) property, not physics. Keep it as a reproducibility goal, not as a model defect.
