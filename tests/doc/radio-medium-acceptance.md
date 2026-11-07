# Acceptance of the radio medium branch

The radio medium branch rebuilds Simu5G's physical layer on the pattern of INET's
radio medium: a medium that keeps the radios and the per-link channel state, a
radio module tree per PHY (antenna, transmitter, receiver), and the link budget,
the reception decision and the SINR statistics on the radios. It is about 160
commits. It was accepted into the development branch piece by piece; this file
records, for each piece, what was checked and what was found. The methods are
described in ../README and ../abtest/README.

Model deviations found along the way are listed in model-deviations.md. Unless a
piece says otherwise, they were recorded rather than fixed, to be addressed
after the whole branch has been merged.

Tools used beyond the test suites of this folder:

- **Reception traces (RxTrace).** A per-run trace of every reception decision
  and every channel evaluation that feeds one: transmitter, receiver, carrier,
  the SINR of every band, the interferers, the error rate, the random draw and
  the RNG's position, the outcome. Off by default; written with
  `--simu5g-rxtrace-file='${resultdir}/rxtrace.txt'`, and fingerprint-inert when
  on. It was kept on its own branch during the acceptance and applied to the
  compared commits in throwaway worktrees.
- **Mutation runs.** A new check is run against a build in which the mechanism
  it guards is deliberately broken, and must fail.

## Piece 1: the D2D mode-switch fix and an RNG stream shift (3 commits)

Commits: "D2dUeMacHelper: test: a mode switch drops the H-ARQ processes towards
both receivers, once" (c45aed913) to "PhyEnb: drop randomChannelIndex_, an unread
member whose only effect was an RNG draw" (a00dc67d2).

- **The test fails before the fix and passes after it.** D2dModeSwitchHarqInterrupt
  fails 6 of 10 checks before "D2dUeMacHelper: fix: a mode switch left UL H-ARQ
  processes waiting for feedback forever", each a defect of the old code, and
  passes 10 of 10 after it.
- **The defect reproduces at system level.** lte/d2d SinglePair-modeSwitching-TCP,
  seed sets 0-29: 14 of 30 runs abort before the fix ("new process selected for tx
  is not completely empty"); all 30 run to the time limit after it.
- **The RNG shift changes no distribution.** Removing one unused draw moves 165 of
  222 fingerprint rows. Replicated A/B, 11 configurations, 30 seeds per side:
  2945 statistics tested, none significant; null control 2980 tested, none
  significant.
- **An unmoved row is no evidence.** The 57 rows the shift does not move are a
  seed-0 coincidence: lte/tutorial SingleCell-DL, identical at seed 0, differs in
  181-212 of 968 scalars at seeds 1-6.

## Piece 2: radio tests and small fixes (45 commits)

Commits: up to "physics tests: test: the D2D link budget" (93bb5577e). Mostly the
unit and physics tests the later pieces rely on, the IRadioEndpoint refactoring,
and config fixes.

- **The tests were reviewed against the specifications.** An independent review
  compared tests/physics/physics_reference.py with TR 38.901 V16.1.0, TR 36.814
  V9.2.0, TR 36.873, TR 36.843 and TR 37.885: the closed forms are verbatim.
- **The tests can fail.** Six mutants (CQI truncation, SINR rounding, the H-ARQ
  reduction applied once too often, a halved Doppler shift, the spec's shadowing
  coefficient, 3 dB more noise) are each killed by the test meant to catch them.
- **The red-first tests are red exactly as their messages say,** and each fix
  flips exactly the checks it names (unit suite run at every commit).
- **Test hardening followed** (radio-test-hardening-on-dev, 9 test-only commits):
  known deviations recorded as a passing pin plus an expected failure against the
  spec (shadowing correlation, BLER table ends, BLER monotonicity, Rayleigh
  fading), interferers at a power of their own in the interference tests, and the
  physics README's account of what each expected value rests on.

## Piece 3: the radio medium and the radio module tree (25 commits)

Commits: "CellularRadioMedium: refactor: the networks gain an inert radioMedium"
to "LteMacEnb: fix: a RAC request from a UE the cell no longer serves gets no
response" (fa2c9f382).

- **The refactorings that move the event structure change no reception.** "frames
  pass through the radio" moves tplx in 252 rows; "frames arrive at the start of
  their transmission" moves tplx and ~tNl in 252 rows. Reception traces of all 253
  rows are identical to their parents' (for the first, once the trace's reception
  start field, which the hook derives from the frame's duration, is set aside).
- **The five fixes move exactly what their messages say,** shown by trace diffs.
  The uplink map rotating at each carrier's own slot boundaries: in
  nr/test_numerology MultiCell-CBR-UL, other-cell interference on the slower
  carrier's uplink went from 12 of 22868 receptions to 2870 of 23922.
- **Each fix got a test placed before it,** red there and green from the fix on:
  UlTransmissionMapNumerologies, UlTransmissionMapIdleGap (both deleted with the
  map they test), BackgroundUeIds, EnbRacResolution, HandoverCandidateX2.
- **The statistics baselines change in location only.** 196 baselines updated;
  1,906,932 values identical and none changed. The SINR statistics moved from the
  channel models to the receivers; multi-carrier radios also record them per
  carrier.
- **Unit suite at 71 commits, physics suite at the commits with resolved
  conflicts and at the tip:** no failure beyond the recorded expected failures.
- **Recorded, not fixed:** a latent dangling gate pointer in the transmission
  request, per-statistic ini options not reaching the per-carrier SINR
  statistics, and `refactor:` markers on commits that change the NED interface.

## Piece 4: per-link channel state (14 commits + 4 test commits)

Commits: "CellularRadioMedium: refactor: the medium keeps the channel state, one
record per channel model" to "channel models: a building's penetration loss is the
UE end's". The LOS state, the shadowing and the Jakes fading paths are kept once
per link by the medium, and redrawn by how far the link's ends have moved.

- **The fingerprint suite passes at the piece's tip.**
- **The per-link state changes the results where it should, and only there.**
  Replicated A/B against piece 3's tip, 13 configurations, 30 seeds per side; the
  null control is clean (3476 statistics, none significant). Significant: the
  downlink H-ARQ error rate of the first transmission, and with it delays and
  playout loss, in channelmodel UMa-38901 (0.20 to 0.05), RMa-38901 (0.16 to 0.04)
  and Penetration-36873 (0.21 to 0.07).
- **The cause is one commit, and it is the intended one.** The three
  configurations were run with 30 seeds at each of the five commits in between.
  The only significant step, in all three, is "a link's LOS state is kept once per
  link" (UMa 0.20 to 0.05, p = 1e-7; RMa 0.16 to 0.05, p = 1e-10; Penetration 0.21
  to 0.12, p = 5e-3); no later step moves any of them significantly. On the
  development branch the base station computed a DL CQI with its own LOS draw while
  the UE decoded through an independent one, so the MCS was often chosen for the
  wrong LOS state. The SINR the CQI is computed from (measuredSinrDl) and the mean
  CQI are unchanged at every step: the channel's distribution did not move, the
  CQI and the decoding now agree.
- **Test gaps were closed, each checked by mutation:** the farther-moved end
  decides when both ends of a link move (shadowing and LOS; the sum or the norm of
  the displacements passed before), the uplink through getRSRP() is faded by the
  link's Jakes paths (the previous check was vacuous), and the per-link state is
  per carrier (ChannelModelPerCarrierState).
- **Deferred:** tests of the indoor logic, to the piece where its final design
  lands.
- **Recorded, to be fixed after the merge** (model-deviations.md, items 13-17): a
  D2D multicast reception keys its interferers on the wrong link; a UE with two
  radios has two indoor positions; a D2D link's indoor loss depends on its
  direction; the per-link maps are never erased; the indoor distance is one
  uniform draw.
- **Transitional states, fixed later in the branch:** the low/high building-loss
  choice can be uninitialized until "PhyBase: refactor: whether a node is inside a
  building is the PHY's parameter"; the D2D conflict graph's path loss depends on
  evaluation order until "the conflict graph's path loss is evaluated outdoors".

## Piece 5a: interference from the medium's transmissions (16 commits + 3 test commits)

Commits: "StochasticChannelModel: fix: DL data interference takes the
interfering cell's occupancy on the victim's carrier, from the radio medium"
to "channel models: external and background cells' interference takes the
model's own path loss". Piece 5 of the plan was split in two; this half moves
every interference computation onto the transmissions registered with the
medium, makes a CQI measure the slot completed last, and puts the external and
background cells on the medium.

- **The fingerprint suite passes at the piece's tip.**
- **The interference changes move no distribution the A/B can see.** Replicated
  A/B against piece 4's tip, 16 configurations (the lte/multicell interference
  family, lte/x2, nr/test_numerology with two numerologies, nr/standalone_multicell,
  nr/bgTraffic, the nr tutorial's background traffic, dual connectivity with
  several cells, multi-carrier, D2D reuse, indoor and tall UEs), 30 seeds per
  side: 7631 statistics tested, none significant. The null control flagged one
  counter in one configuration (nr/test_multiCarrier MultiCarrier-CBR-DL, the
  UE's received MAC PDUs, 8 statistics counting the same thing); a third seed
  range (60-89) compares clean with both, so it was a low sample of a
  heavy-tailed counter, not a dependence on more than the seed.
- **Five refactorings were not identical in every state** and were re-marked or
  amended before acceptance: two are latent fixes (the DL occupancy of a cell
  with several carriers; uplink frames whose sender did not record them in the
  Binder's map), two change behavior in states no row reaches (background UEs
  of a cell with several carriers; external and background cells whose ticks
  are not the carrier's slots), and one introduced a latent defect -- a
  background UE's DL allocation radiated omnidirectionally by a sectorial base
  station -- whose fix, from piece 7, was folded into it.
- **Test gaps were closed, each checked by mutation:** the medium's
  transmissions of a slot and what it keeps (RadioMediumTransmissionRegistry),
  the slot a CQI measures (ChannelModelCqiSlot, on 0.25 ms slots), and the
  position a reception is evaluated at (ChannelModelReceptionPosition).
- **The new tests pass at 42 commits:** at their own commits, and at every
  later commit that changes the interfaces they use, up to the branch tip. The
  full unit and physics suites, at the piece's tip and at the commit the fix
  was folded into, show no failure beyond the recorded expected failures, each
  failing on its checks.
- **Deferred to piece 7,** where the phantom radios give these links their
  final shape: a test of the background UEs' registration through the
  scheduler, and of the external-cell path loss's geometry and indoor rule.
- **Recorded, to be fixed after the merge** (model-deviations.md, items
  18-21): a background UE on a cell's second carrier occupies the wrong bands;
  a transmission between slot boundaries prunes the slot completed last
  (pinned, with an expected failure); a background UE's uplink allocation is
  on the medium only if its cell evaluates uplink interference; the handover
  beacon's RSSI counts the background cells but not the real ones.
- **The A/B tool was fixed** (tests/abtest/run.sh failed every run with a
  relative output directory, as in its README's example).

## Piece 5b: positions, Doppler speeds and the medium's channel models (9 commits + 4 test commits)

Commits: "PhyBase, background UEs: positions are the mobility's, at the instant
they are read" to "D2D: fix: a one-to-many D2D frame interferes at the D2D
power it is sent at". The second half of piece 5 of the plan: a node's position
is its mobility's at the instant it is read, the Doppler speed is the mobile
end's, from its mobility, the radio medium owns the channel models (one per
carrier and RAT), and a one-to-many D2D frame interferes at its own power.

- **The fingerprint suite passes at the piece's tip.**
- **The statistics of the rows with moving UEs and Jakes fading change, and
  they change as the old Doppler estimate predicts.** Replicated A/B against
  piece 5a's tip, 15 configurations (the channelmodel rows UMa-38901,
  SMa-36814, Penetration-36873 and TallUe-38901, the LTE tutorial, demo VoIP,
  handover VoIP-DL, D2D mode switching, D2D multihop, nr D2D with dynamic LOS,
  multi-carrier, numerology, standalone multicell, dual connectivity, Ethernet
  handover), 30 seeds per side: 2792 statistics tested, 120 significant, in 10
  configurations. The largest: the first-transmission DL H-ARQ error rate in
  SMa-36814 (0.20 to 0.37, with the mean received DL SINR 11.2 to 8.1 dB) and
  in lte/demo VoIP (0.025 to 0.004, with the mean opinion score 4.18 to 4.36);
  the UL error rate in nr/ethernet Ethernet-Handover (0.006 to 0.048). On the
  development branch the channel models estimated the Doppler speed from two
  positions quantized to the mobility's update interval: zero between two
  updates, so a moving UE's fading was frozen at its static value, and many
  times the real speed right after an update. With the real speed, the fading
  of a fast UE varies between the CQI and the decoding (more errors), and
  that of a slow UE no longer jumps after each update (fewer errors).
- **The cause is the intended commit.** The 10 configurations with
  significant changes were run with 30 seeds at two inner commits. All but a
  few of the changes come with "positions are the mobility's, at the instant
  they are read", measured together with the refactoring after it (for
  example SMa-36814 0.20 to 0.37, VoIP 0.025 to 0.005). "the Doppler speed is
  the mobile end's, from its mobility" moves only the means of the distance
  statistic and the measured SINR of a few UEs, in their fourth or fifth
  significant digit (lte/test_handover VoIP-DL, DoubleConnection): reading
  the speed makes the mobility report its position, and the PHY emits these
  statistics at each report (item 25). The rest of the piece moves nothing.
- **The null control is clean overall, and its flags in two configurations
  are chance.** No statistic is significant over all configurations; two
  configurations have flags of their own. In nr/test_multiCarrier the same counter as in piece 5a
  is flagged again (8 statistics counting the received MAC PDUs). In
  channelmodel UMa-38901, 117 of 134 statistics, all following the number of
  frames the base station sends (729 against 641 on average). A third seed
  range (60-89) compares clean with both (0 of 136 and 0 of 135 statistics;
  the same count is 693 there), so the two ranges were at the two ends of a
  wide spread, and BH, applied within one configuration, flagged every
  statistic that follows it. The A/B's 5 significant statistics in this
  configuration come, in the bisection, with the same commit as the others.
- **D2D multihop moves no distribution either.** It records no statistic in
  the 3 s of the A/B above, so it was compared again at 15 s, 30 seeds per
  side: 5577 statistics tested, none significant. The D2D power fix, which
  moves the four multihop fingerprint rows, is not visible in the
  distributions. The D2D SINR statistic is not compared: between "the radio
  medium owns the channel models" and "a D2D receiver records its own
  rcvdSinrD2D", later in the branch, the medium's channel model records it,
  for all receivers at once.
- **The building penetration loss had a fault, which was split out as its own
  fix:** at an outdoor base station, the high-loss choice of an indoor UE's
  links was read from an uninitialized member. No fingerprint row reaches
  this state (each row that puts UEs inside a building marks the base
  stations too), and a test of undefined behavior cannot be made red, so the
  fix has no test of its own; the test of the choice at the base station's end
  is deferred to piece 7, with the indoor tests.
- **Test gaps were closed, each checked by mutation:** a background UE's
  position between and after two mobility reports (BackgroundUePosition);
  whose speed the Doppler shift takes, for each of a link's three evaluations,
  with the base station's speed left out (ChannelModelDopplerSpeed); the
  medium's channel model of a carrier and a RAT (RadioMediumChannelModels);
  and the power a one-to-many D2D frame is recorded at
  (CellularTransmitterD2dPower, which goes first and fails until the fix).
- **The new tests pass at 41 commits:** at their own commits, and at every
  later commit that changes the interfaces they use, up to the branch tip;
  the D2D power test fails at its own commit only, as intended. The full unit
  and physics suites at the piece's tip show no failure beyond the recorded
  expected failures, each failing on its checks.
- **Recorded, to be fixed after the merge** (model-deviations.md, items
  22-25): a car halted at a red light keeps its Doppler shift; a car's
  trajectory depends on how often its position is read; the Jakes phase is
  the speed times the absolute time, and only one end's speed counts; the
  distance statistic is weighted by the reads of the position.
