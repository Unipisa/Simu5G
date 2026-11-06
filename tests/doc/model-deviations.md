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
   - Later in the branch: 5166c1198 ("shadowing is redrawn by how far its ends have moved") keeps the factor "as before", and c65da458c makes the shadowing per link. Neither fixes this.
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
   - Later in the branch: fe74f9e4b ("external and background cells' interference takes the model's own path loss") replaces this. Re-check after the merge whether anything remains, e.g. shadowing and LOS for these links (stage B gives phantom links their own LOS state in 8766d912d).

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
   - What the model does: each CellularRadio draws its own distance from the wall (uniform 0-25 m) and, from 3e2cc6bb4 on, its own penetration-loss deviate. An NR UE always has two radios (NrNicUe extends the LTE NIC: `radio` and `nrRadio`, the LTE one unused in standalone), and `**.insideBuilding = true` marks both, so every indoor NR UE draws twice; a dual-connectivity UE uses both.
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

## Fixed later in the branch, re-check only

- **The O2I loss's random term redrawn at every evaluation:** drawn once per UE from 3e2cc6bb4 ("the random term of a UE's building penetration loss is drawn once, the UE's own"); see item 14 for the per-radio residue.

- **Shadowing per direction (LinkReciprocity):** made per link by c65da458c.
- **Jakes fading evaluated at now - 1 ms:** fixed by c36541a99.
- **The Doppler speed from a two-sample estimate:** fixed by 9f1ff8874.

## Not a deviation

- **ModuleOrderPermutation** asserts per-realisation invariance under relabelling of UEs. That is a common-random-numbers (RNG design) property, not physics. Keep it as a reproducibility goal, not as a model defect.
