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

## Piece 6a: four small fixes, the path-loss draws and the Jakes fading at a transmission's start (12 commits + 6 test commits)

Commits: "Binder: fix: asking for the X2 address of a base station that is no
X2 peer is an error" to "unit tests: test: a link's value does not depend on
when or how often it is evaluated". The first half of piece 6 of the plan:
four fixes that the fingerprint suite does not reach, the UMa environment
height drawn once per link with its LOS state, the random term of a UE's
building penetration loss drawn once per UE, and the Jakes fading of a link
taken at the start of the transmission evaluated on it.

- **The fingerprint suite passes at the piece's tip.** The test commits leave
  the sources as they were.
- **One configuration moves, and as intended.** Replicated A/B against dev, 15
  configurations (the channelmodel rows UMa-38901, TallUe-38901,
  TallUe-36873, Penetration-36873, PenetrationLow-38901, PenetrationHigh-38901
  and Penetration-MultiCarrier-38901, nr standalone VoIP-DL, background
  traffic, standalone multicell, numerology DL and UL, dual connectivity
  multicell and split bearer, nr D2D with dynamic LOS), 30 seeds per side: 3152
  statistics tested, significant ones only in channelmodel
  PenetrationHigh-38901 (18 within the configuration, 10 over all). The
  first-transmission DL H-ARQ error rate goes from 0.50 to 0.33, the MAC
  packet loss from 0.106 to 0.035, the VoIP frame delay from 0.09 to 0.36 s.
  On dev the random term of the high-loss penetration loss (sigma_P = 6.5 dB)
  was drawn at every evaluation, so a CQI and the reception after it saw two
  independent terms, and about half of the first transmissions failed: those
  where the reception's term came out worse than the CQI's. Drawn once per
  UE, the CQI and the reception agree. The delay grows in most runs, not in a
  few (the median of the 30 run means goes from 0.04 to 0.26 s), with fewer
  MAC PDUs sent (963 to 668 on average); why was not analyzed further. With
  the low-loss model (sigma_P = 4.4 dB) nothing is significant.
- **The cause is the intended commit.** Within the piece only "the random
  term of a UE's building penetration loss is drawn once, the UE's own" moves
  the PenetrationHigh-38901 fingerprint row, so the changes were not bisected
  further. The UMa environment height (not drawn where its outcome is
  certain, then drawn with the link's LOS state) and the Jakes fading at the
  transmission's start move no distribution.
- **The null control is clean:** 3385 statistics, none significant, in no
  configuration.
- **The A/B found a crash on dev:** one run of dev's side, nr/test_numerology
  MultiCell-CBR-UL at numerology 2, seed 29, aborts at 4.025 s: a UE between
  two cells of a handover (serving cell 0) makes a MAC PDU and asks for the
  MAC of cell 0. The run was left out of the comparison (29 runs on that
  side). The piece does not cause it; it is to be fixed on its own.
- **The fixes have their tests first,** each failing at the commit before the
  fix: BinderX2PeerAddress (a lookup that misses throws and leaves the peer
  table as it was), MulticastDestIds (the counter the inspector shows holds the
  next id, 32768 first) and BackgroundUeAllocationBands (the real scheduler,
  given a band-limit vector holding only band 3, records the blocks on band 3).
  The LteMaxCi fix has no test: the check it corrects had no effect, and only a
  real UE with the id 4097 reaches the difference (the 2049th NR UE gets it, an
  id clash to be fixed on its own). The four messages were reworded to the
  Fault/Fix/Test form.
- **Test gaps were closed, each checked by mutation:** the UMa environment
  height's distribution, graded against TR 38.901 from draws, and the path
  loss at a given h_E (PathLossEnvironmentHeight), and that a link's path loss
  is computed at the h_E kept for it, at either end
  (ChannelModelEnvironmentHeight); an indoor UE's penetration-loss deviate on
  each of its links, at either end (ChannelModelPenetrationDeviate); and a
  received frame faded at the start of its reception, downlink and D2D
  (ChannelModelFrameStartTime). ChannelModelLinkEvaluation also checks that a
  later start time changes the fading only, and says that its geometry does not
  show h_E.
- **The new tests pass at every later commit,** 51 of them, up to the branch
  tip, in versions that follow the interface changes; each fix test fails at
  the commit before its fix only, as intended. The full unit and physics suites
  at the piece's tip show no failure beyond the recorded expected failures,
  each failing on its checks.
- **Recorded, to be fixed after the merge** (model-deviations.md): items 26
  (the UMa h_E's probability of 1 m: TR 36.873 leaves out the distance, TR
  38.901 draws at 18 m; two new expected failures) and 27 (a link's LOS state
  and h_E per cell and carrier, not per site and UE), the Doppler speed read at
  the evaluation rather than at the transmission's start (item 24), and the
  Binder's uplink map written past its end on a second carrier (item 18).

## Piece 6b: INET's event shape, the evaluation's arguments and the what-if (15 commits + 6 test commits)

Commits: "CellularRadioMedium: refactor: radios are kept, and broadcasts
delivered, in the order they were added" to "StochasticChannelModel: fix:
another cell interferes whatever its own channel model is". The second half
of piece 6 of the plan: a transmission's end and every reception at an event
of the radio's own, beacons as packets with a size, every radio's position
read as a transmission starts, the radio and the link's conditions as
arguments of an evaluation, the CQI's what-if on the medium, and another
cell's interference evaluated with the receiver's model.

- **The fingerprint suite passes at the piece's tip.** The test commits leave
  the sources as they were; the two fixes, the throw on an unknown
  self-message and the corrected comments move no row.
- **The event-shape commits move only the event count.** The transmission end
  event and the reception timer move tplx and tilx in every row with radio
  transmissions; the beacon's own event, packet and size the 48 rows whose
  base stations send beacons. No result changes.
- **No distribution moves beyond a hair.** Replicated A/B against dev, 17
  configurations (channelmodel UMa-38901, UMi-36873 and
  PenetrationHigh-38901, lte tutorial SingleCell DL and UL, lte handover
  VoIP DL and UL, nr numerology single and multicell DL and UL, nr multi
  carrier, standalone multicell, dual connectivity multicell, nr D2D mode
  switching and dynamic LOS, nr TDD), 30 seeds per side: 3751 statistics
  tested, 14 significant, in four configurations whose runs hardly vary
  between seeds (lte test_handover VoIP-DL and VoIP-UL, nr
  dualConnectivity_multicell DoubleConnection-CBR-DL, nr d2d
  SinglePair-modeSwitching-UDP): the mean measured DL SINR by at most 0.002
  dB, and the D2D UEs' distance statistic by 0.3 m (115.24 to 115.57 m, the
  distance weighted by the reads of the position, item 25). The UEs there
  move, and their positions are now read at more instants and the CQI a slot
  later; the changes were not bisected further.
- **The null control has one hit, which is chance:** 4977 statistics, 0
  significant over all; within nr/test_multiCarrier MultiCarrier-CBR-DL one
  correlated family (ue[0]'s packet counts, 9 statistics) is lower in seeds
  30-59 than in seeds 0-29, and within nr/test_tdd TestTdd one statistic. Both
  sides are dev's build; a third set of dev's seeds (60-89) lies between the
  two (p = 0.18 and 0.018 for the family).
- **Two defects were found in the piece, and fixed after their tests:** a
  frame still on the air when its radio is deleted was left behind (from "a
  reception ends on a reception timer of the radio's own": the timer only
  pointed to the frame; every run reported "undisposed object" for the frames
  in flight at its end); RadioFramesDisposedAtEnd (a module test) fails from
  that commit until "a frame still on the air when its radio is deleted is
  deleted with it". Another cell whose RAT has a non-stochastic model on the
  carrier did not interfere at a UE of the other RAT, although the receiver's
  model evaluates the link; ChannelModelCrossRatInterference fails until
  "another cell interferes whatever its own channel model is". Neither state
  is in a fingerprint row.
- **Test gaps were closed, each checked by mutation:** a radio removed from
  the medium takes only its own entry (RadioMediumRemoveRadio); a
  transmission's start reads every other radio's position once, in the order
  the radios were added, and from "a phantom transmission reads no radio's
  position" on a phantom transmission none (RadioMediumPositionReads); a CQI's
  what-if takes the fading and the positions of its own instant
  (ChannelModelCqiTime; which instant the base station's PHY chooses is not
  reachable without a PHY); the medium's what-if is evaluated with the
  receiver's RAT model, and a receiver off the carrier is rejected
  (RadioMediumWhatIfModel).
- **Messages:** the radio and the link conditions as arguments are marked as
  refactors; the CQI's later instant, the D2D CQI's mixed positions it
  removes, the what-if's two changed edge states and the interference link's
  state drawn by the receiver's model are stated; the unit-test counts that
  predated the rebase are dropped.
- **The new tests pass at every later commit,** up to the branch tip, in
  versions that follow the interface changes; the two fix tests fail only
  between the faulty commit and their fix. The full unit and physics suites
  at the piece's tip show no failure beyond the recorded expected failures.
- **Recorded, to be fixed after the merge** (model-deviations.md): items 28
  (the CQI has no measurement delay: TS 38.214 5.2.2.5 measures nCSI_ref >=
  4*2^mu slots before the report), 29 (the indoor state ignored by TR 36.814
  and by TR 36.873 outside UMa) and 30 (a moving base station's interference
  taken from where it is now).

## Piece 7a: phantom radios, and background cells on the medium (9 commits + 8 test commits)

Commits: "ExtCell, BackgroundCell, BackgroundUe: external and background cells
and background UEs have phantom radios" to "BackgroundCell:
BackgroundCellChannelModel deleted". The first part of piece 7 of the plan:
whatever transmits without a PHY gets a radio of its own, shaped like INET's
NoiseSource, its transmissions are sent by that radio, their links have the
radios' budget and a LOS state of their own, and background UEs' and cells' CQIs
are what-ifs on the medium, so BackgroundCellChannelModel goes.

- **The fingerprint suite passes at the piece's tip.** The test commits, the
  external cell's fix and the amendments (a failed registration is not removed,
  a phantom transmission on none of a reception's bands is not evaluated, the
  reception paths assert that the radio has a receiver) move no row.
- **One change is larger than its commit said.** The interference switches are
  now the medium's channel model's, off by default, where
  BackgroundCellChannelModel's were on, so a background cell's UEs see no real
  cell's interference unless they are set. nr/tutorial BgTraffic, fading and
  shadowing off, mean bgMeasuredSinrDl: the old model 8.48 dB, and 16.90 dB with
  its switch off; the new code 16.92 dB, and 8.50 dB with
  `**.downlinkInterference = true`. The rise is all of it: TR 38.901's and TR
  36.814's UMa NLOS formulas differ by 0.02 dB at that distance. The tutorials
  (nr/tutorial, tutorials/nr) get the switches together with the fingerprint-row
  work right after the branch, as their rows have to be re-recorded.
- **No real UE's distribution moves.** Replicated A/B against dev, 10
  configurations (nr/bgTraffic CBR-DL, CBR-UL, CBR-DL_UL and
  CBR-DL-NoBgCellInterference; nr/tutorial BgTraffic, Numerology and Multiple-UEs;
  lte/multicell Interference-AllOn and Interference-NoExtCell; nr/standalone
  VoIP-DL), 30 seeds per side: 1418 statistics tested, 23 significant within their
  configurations, none over all, and none of a real UE. In the tutorials the
  background UEs' SINR rises by about 10 dB and their CQI by 2 to 3, the change
  above. In nr/bgTraffic CBR-DL_UL one cell's served blocks fall (UL 5.22 to 4.68,
  DL 3.46 to 3.38). The lte/multicell configurations and the VoIP control are
  identical run by run.
- **The null control is clean:** 3216 statistics, none significant.
- **Background UEs lost their shadowing, which is recorded (item 31).** The spread
  of the mean SINR across a cell's background UEs narrows, with the means unchanged,
  in nr/bgTraffic: DL 10.2 to 8.6 dB (mean 47.06 to 47.10), UL 9.7 to 8.5 dB (mean
  29.84 to 29.87).
- **A capability is lost.** A background cell's scenario, heights, fading and
  thermal noise are the medium's channel model's, per carrier and RAT; a cell can
  no longer have its own. This is part of item 32 of model-deviations.md, a modeling
  error to be fixed after the merge, not a design choice.
- **The fix has its test first:** an external cell's new mobility inherited the
  ini's constraint area, so a cell outside it aborted at startup;
  ExternalCellOutsideMobilityArea fails until "an external cell may stand outside
  the area the nodes move in".
- **Test gaps were closed, each checked by mutation:** the fields of a phantom
  transmission and the antenna wiring, an id clash failing cleanly
  (PhantomRadioTransmissions); the link budget, DL and UL, with every term set apart
  (ChannelModelPhantomLinkBudget: putting the model's keys back passed every suite);
  a phantom link's 2D and 3D distances on either side of a breakpoint, its UE end, its
  h_E, and no evaluation of a transmission on none of the bands
  (ChannelModelPhantomGeometry, deferred from piece 5a); a background UE's CQI and
  load against the closed form, so that its allocations reach the medium with the
  station's antenna (BackgroundUeCqi, BackgroundUeLoad, the rest of that deferred
  item; dropping the DL what-if's cell id passed every suite and all 254 rows); a
  background cell's CQI, the interference switches both ways, and a narrower cell
  (BackgroundCellCqi); the two rejected configurations
  (BackgroundCellRejected_WiderThanCarrier, BackgroundCellRejected_NoModelOnCarrier).
  Not covered: a background UE's retransmission's registration, and the penetration
  deviate on a phantom link.
- **Messages:** the flipped switches and the false path-loss explanation are
  corrected, the old speed that was always 0, the deleted keys' successors and the
  states that differ are stated, and the unit-test counts that predated the rebase
  are dropped. The WHATSNEW entry, which repeats the explanation, is corrected with
  piece 7c.
- **The new tests pass at every later commit,** up to the branch tip, in versions
  that follow the interface changes; the external cell's test fails only at its own
  commit, before the fix. The full suites at the piece's tip show no failure beyond
  the recorded expected failures.
- **Recorded, to be fixed after the merge:** item 31 of model-deviations.md, and
  the defects and cleanups outside the branch's scope, in open-defects.md (new with
  this piece, from the findings of pieces 1 to 6b).

## Piece 7b: D2D in the core model, and the receiver's error model (6 commits + 5 test commits)

Commits: "CellularReceiver: a D2D receiver records its own rcvdSinrD2D" to
"IdealChannelModel: the configured error rate is the receiver's
ConfiguredRateErrorModel", with its test commit after it. D2D links are evaluated by the
core channel model, the capture effect takes a received frame's power from the
medium, and the receiver's error model is replaceable, the configured error
rate being one.

- **The fingerprint suite passes at the piece's tip.** Only "a D2D receiver
  records its own rcvdSinrD2D" moves rows: 38, sz only, the statistic's module
  path. The moved D2D bodies match the originals.
- **Nothing moves.** Replicated A/B against dev, 9 configurations (lte/d2d and nr/d2d
  SinglePair-UDP-D2D, nr/d2d MultiplePairs-UDP-D2D-wReuse,
  SinglePair-modeSwitching-UDP and ChannelModel-DynamicLos, lte/d2d_multicast
  checkMulticastRange, lte/rlc and nr/rlc AM-Lossy, nr/standalone VoIP-DL), 30 seeds
  per side: every value common to both sides is identical, run by run. The null
  control: 1460 statistics, none significant.
- **An existing ideal-channel configuration changes meaning, and does not fail.**
  A lossy ideal channel now takes two settings. With a rate but without
  `**.receiver.errorModel.typename = "ConfiguredRateErrorModel"`, the run loses
  nothing (lte/rlc AM-Lossy at perDl = 0.3, first-transmission DL H-ARQ error rate:
  0.32 with it, as on dev; 0 without it), and OMNeT++ 6.5 reports the rate keys as
  having had no effect. With no rate keys at all, the loss goes from 10% to 0, from
  the NED defaults (not run). No error was added: a lossless default is what an
  ideal channel should mean. A rate is also one per receiver now, not per carrier
  and RAT, and the wildcard reaches every radio of a node.
- **Small amendments, none moving a row:** the ideal model's decision has the
  stochastic model's checks of the CQI and the attempt and takes its SINR from
  getSINR(); the receiver asserts that a rate is between 0 and 1.
- **A recorded item was false:** item 13, on the key of a D2D multicast reception.
  D2dUePhy::frameAccepted() sets a group member's destination id to its own before
  the SINR is computed. lte/d2d_multicast checkMulticastRange, 3 s, a temporary
  counter: 7401 receptions with the receiver's id, none with the transmitter's. So
  the recorded deviations are claims too, to be checked before they are fixed.
- **Test gaps were closed, each checked by mutation:** the per-carrier D2D statistic
  (ReceiverD2dStatistics); the D2D interference walk against the closed form, and
  the rule's parts through the D2D CQI, as a same-cell uplink never overlaps a D2D
  reception (D2dInterferenceDelta, a characterisation test); a received frame's
  power (RadioMediumReceptionRsrp); what the stochastic model gives the error model
  (StochasticErrorModelSeam); the configured rate's formula
  (ConfiguredRateErrorModel); the ideal channel with either error model
  (IdealChannelErrorModels).
- **Messages:** the states that differ are named (a D2D CQI under the core type
  aborted, the ideal model's capture aborted); the error model's interface is marked
  as a refactor; the unit-test counts that predated the rebase are dropped.
- **The new tests pass at every later commit,** up to the branch tip, in versions
  that follow the interface changes. The full suites at the piece's tip show no
  failure beyond the recorded expected failures.
- **Recorded, to be fixed after the merge:** item 32 of model-deviations.md, and new
  entries in open-defects.md (AmcPilotD2D's band 0, the pilotMode enum's typo, a band
  on a remote unit keeping its RSRP, the decision's place, the two encodings of a
  certain loss).

## Piece 7c: the deletions, the budget and the indoor state on the radios (13 commits + 7 test commits)

Commits: "Binder: the uplink transmission map deleted" to "NICs, BackgroundScheduler:
the carrier counts and the medium reference that nothing reads are gone". The last part
of piece 7 of the plan, and of the branch: what the medium replaced goes, the link budget
and the indoor state are the radios', and the entry in WHATSNEW describes the result.

- **The fingerprint suite passes at the branch's tip, and no commit of this piece moves a
  row.** 254/254 in 832 s on a quiet machine. The commit that gave the medium six empty
  submodules named after INET's (245 rows, tilx only) was dropped: they existed only for a
  later stage that is not scheduled, and their names invited keys that silently do nothing.
- **Nothing moves.** Replicated A/B against dev, 10 configurations (nr/bgTraffic CBR-DL_UL,
  nr/tutorial BgTraffic, lte/multicell Interference-AllOn, lte/test_handover VoIP-DL,
  nr/test_numerology MultiCell-CBR-DL at numerology 2, nr/test_multiCarrier
  MultiCarrier-CBR-DL, nr/d2d MultiplePairs-UDP-D2D-wReuse, lte/rlc AM-Lossy, channelmodel
  PenetrationHigh-38901, nr/standalone VoIP-DL), 30 seeds per side: every value common to
  both sides is identical, run by run. The null control: 3147 statistics, none significant
  over all; within nr/test_multiCarrier the family of ue[0]'s packet counts in seeds 30-59
  again, the chance found in piece 6b (the seeds are the same).
- **A defect was found in the piece, and fixed after its test:** a UE that has left the
  simulation aborted another node's reception or CQI, as the medium keeps its transmissions
  for up to two slots and the walks looked up its radio for the antenna gain. A transmission
  now records that gain. RadioMediumDepartedInterferer fails until "a UE that has left does
  not abort another node's evaluation". On a release build this was a regression against
  dev; no row removes UEs.
- **The deleted bridges found something, so their loss is stated.** The downlink band-status
  check was catching issue #310. Nothing detects it on dev until that fix arrives with an
  invariant of the scheduler and the medium (open-defects.md). The uplink map's bridge has
  two replacements that need no copy: a real UE's uplink power and a radio's bands are
  asserted in the walks.
- **WHATSNEW was wrong in places, and is corrected:** the background cell's rise in SINR comes
  from the interference switches, not from TR 38.901; the ideal channel does not keep "the same
  parameters and defaults"; the indoor draws are per radio. The table gains the keys it missed,
  of which a stale componentCarrierModule or numCarriers puts a cell on the wrong carrier
  without a message, and a section says what can no longer be configured.
- **The NED residue is gone:** numNrCarriers and a UE's numCarriers, which nothing read,
  though 19 ini and test files set them; BackgroundScheduler's radioMediumModule. A base
  station's numCarriers stays: it sizes bgTrafficGenerator[], which the MAC indexes by
  carrier, so a rename to a generator count would have hidden that it must equal the
  carriers (a smaller value crashes: open-defects.md).
- **The tests deferred from pieces 4, 5a and 5b are in,** each checked by mutation:
  CellularRadio's own draws (CellularRadioIndoorDraw); one indoor distance per radio on all
  its carriers and in both directions, through a real PHY (IndoorOneRadioTwoCarriers); the
  indoor distance of an uplink and the high-loss choice at the base station
  (ChannelModelPenetrationDeviate); an indoor UE in the interference walks
  (ChannelModelIndoorInterference: dropping the UE end passed every suite and row); pins with
  expected failures for items 14 and 15. Also a frame no radio sent
  (ChannelModelFrameWithoutStartPosition), and the D2D walk's cable loss end
  (D2dInterferenceDelta).
- **Messages:** deletions without behavior are marked as cleanups, the states that differ are
  named, the capabilities lost are stated, and the unit-test counts are dropped.
- **The new tests pass at every later commit,** each commit built in debug; the fix test fails
  only at its own commit. Full suites at the tip: unit 64 + 9 expected failures, physics 17 +
  1, module 17 + 2, all recorded.
- **What the branch leaves open:** model-deviations.md, items 1 to 32 but for 13, and
  open-defects.md, to be fixed after the merge, starting with the modeling errors of item 32
  and the receive chain.
