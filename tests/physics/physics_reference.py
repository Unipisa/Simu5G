#! /usr/bin/env python3
#
# Grades the result files of a physics test against closed-form expectations.
#
# The expected value of a check is computed here, from the scenario's own
# parameters, and never read from a previous run of the model. That is the
# whole point of this directory: a fingerprint tells you the trajectory
# changed, this tells you whether the physics is still right, and it keeps
# saying so after a fingerprint has been re-recorded.
#
# Result files are read through omnetpp.scave.results, the same library the
# IDE and opp_scavetool use, rather than by parsing .sca text here. It needs
# omnetpp/python on PYTHONPATH, which sourcing the OMNeT++ setenv script does.
#

import glob
import importlib.util
import math
import os
import re
import sys

try:
    from omnetpp.scave import results as scave
except ImportError as e:
    print(f"FAILED cannot import omnetpp.scave.results ({e}) -- "
          f"source the OMNeT++ setenv script", file=sys.stderr)
    sys.exit(2)

# Tolerance policy, stated once for every statistical check in this directory
# rather than per test.
#
# A check whose observed value is a fraction of N independent Bernoulli trials
# is graded against a K-sigma band around the closed-form probability, where
# sigma is the binomial standard error sqrt(q(1-q)/N). K = 4 puts the
# false-failure rate of a correct model at about 1 in 16000 per check, which
# is small enough that the suite does not cry wolf, while still catching every
# structural error seen so far -- those move the result by a factor, not by a
# few percent.
#
# The band is only as tight as N allows: at q = 0.0625 and N = 3400 it is
# +-27% of q, at q = 0.49 it is +-7%. A case's own sigma figure is printed
# with its verdict so the reader can see how discriminating it actually was.
SIGMA_K = 4.0


def binomial_verdict(observed, expected, n):
    """K-sigma binomial band. Returns (ok, deviation_in_sigmas, halfwidth)."""
    if n <= 0:
        return False, float('nan'), float('nan')
    sigma = math.sqrt(expected * (1.0 - expected) / n)
    if sigma == 0.0:
        return observed == expected, 0.0, 0.0
    return (abs(observed - expected) <= SIGMA_K * sigma,
            (observed - expected) / sigma,
            SIGMA_K * sigma)


class Grader:
    def __init__(self):
        self.graded = 0
        self.failures = 0

    def check(self, ref, params, observed, expected, n):
        ok, sigmas, halfwidth = binomial_verdict(observed, expected, n)
        self.graded += 1
        self.failures += 0 if ok else 1
        print(f"{'OK     ' if ok else 'FAILED '} {ref:<22} {params:<28} "
              f"observed={observed:.6f} expected={expected:.6f} "
              f"n={int(n)} dev={sigmas:+.2f}sigma tol=+-{halfwidth:.6f}")

    def check_true(self, ref, params, ok, detail=""):
        """A property that is either held or not, with no numeric distance to
        report -- an ordering, a sign, an invariance."""
        self.graded += 1
        self.failures += 0 if ok else 1
        print(f"{'OK     ' if ok else 'FAILED '} {ref:<22} {params:<28} {detail}")

    def check_absolute(self, ref, params, observed, expected, tolerance, unit=""):
        ok = abs(observed - expected) <= tolerance
        self.graded += 1
        self.failures += 0 if ok else 1
        print(f"{'OK     ' if ok else 'FAILED '} {ref:<22} {params:<28} "
              f"observed={observed:.6g}{unit} expected={expected:.6g}{unit} "
              f"tol=+-{tolerance:.6g}{unit}")


def scalar_table(filter_expression, itervars):
    """One row per run, one column per scalar name, indexed by the iteration
    variables the test sweeps.

    convert_to_base_unit is off deliberately. It would rescale the values by
    the statistic's declared unit, and it applies that to the ':count' field
    too -- so a statistic declared in bytes reports a packet count multiplied
    by eight. What a check wants is the number the run actually recorded.
    """
    df = scave.get_scalars(filter_expression, include_itervars=True,
                           convert_to_base_unit=False)
    if df.empty:
        raise LookupError(f"no scalars matched: {filter_expression}")
    for var in itervars:
        if var not in df.columns:
            raise LookupError(f"results carry no iteration variable '{var}'")
        # Iteration variables arrive as text. Numeric ones are converted so that
        # a sweep sorts and prints as numbers; the rest -- a boolean flag, a
        # scenario name -- are left as they were written in the ini.
        try:
            df[var] = df[var].astype(float)
        except ValueError:
            pass
    # One recorded value per cell. The table is keyed by name, not by module,
    # so the same scalar matched on two modules -- or in two repetitions of a
    # run -- would otherwise be averaged into a number no run recorded. A nan is
    # not a value: a statistic declared on a module that never emitted it
    # records nan, and is left out.
    counts = df.dropna(subset=['value']).groupby(list(itervars) + ['name']).size()
    if (counts > 1).any():
        raise LookupError("scalars matched more than once, on several modules or in "
                          f"several repetitions: {counts[counts > 1].to_dict()}")
    table = df.pivot_table(index=list(itervars), columns='name', values='value',
                           aggfunc='first')
    return table


def grade_harq_residual_loss(grader):
    """IdealChannelModel with harqReduction = 1 makes the HARQ transmission
    attempts of a MAC PDU i.i.d. Bernoulli(perDl), so a PDU is discarded only
    when all maxHarqRtx+1 of them fail. See IdealChannelModel.ned:32."""

    # The module filters also do the work of skipping the statistics that were
    # declared on a module which never emitted them: on an NR node the LTE leg
    # records macPacketLossDl as nan, and '*.nrMac' leaves those rows out.
    table = scalar_table(
        '(module =~ "*.nrMac" AND (name =~ "macPacketLossDl:*"'
        '                       OR name =~ "harqErrorRateDl:*"))'
        ' OR (module =~ "*.app[0]" AND name =~ "cbr*Bytes:count")',
        itervars=('perDl', 'maxHarqRtx'))

    for (per, rtx), row in table.iterrows():
        rtx = int(rtx)
        params = f"perDl={per} maxHarqRtx={rtx}"

        # The residual loss rate RLC sees: macPacketLossDl is emitted once per
        # completed HARQ process, 1 when it ended in a discard and 0 when it
        # ended in an ACK (LteHarqUnitTx::pduFeedback).
        processes = row['macPacketLossDl:count']
        grader.check('harq-residual', params, row['macPacketLossDl:mean'],
                     per ** (rtx + 1), processes)

        # Tripwire, not a claim about the model: the per-attempt error rate has
        # to be perDl for the check above to mean anything at all. If
        # harqReduction stops being neutral, or the error model starts looking
        # at the transmission number, this fails first and says so -- rather
        # than the residual check failing with no indication of why.
        grader.check('harq-attempt-rate', params, row['harqErrorRateDl:mean'],
                     per, row['harqErrorRateDl:count'])

        # RLC UM does not retransmit, so every MAC PDU the HARQ entity gave up
        # on is an SDU the application never sees, and with one SDU per PDU the
        # two counts must be equal. This is what makes the check above an
        # assertion about the whole PHY -> MAC -> HARQ -> RLC chain rather than
        # about one statistic: if a discarded PDU still reached the application,
        # or a delivered one did not, the residual figure above would be
        # measuring something other than what RLC actually loses.
        #
        # The tolerance is a few SDUs, not zero, because an SDU still in flight
        # when the run ends is indistinguishable from a lost one. The drain
        # window makes that unlikely rather than impossible.
        never_arrived = row['cbrGeneratedBytes:count'] - row['cbrReceivedBytes:count']
        grader.check_absolute('um-loss-transparency', params, never_arrived,
                              row['macPacketLossDl:sum'], 5.0, " SDUs")


def pathloss_reference():
    """The transcription of the 3GPP formulas that grades the path-loss unit
    tests, imported rather than copied. A physics check that needs a path loss
    and the unit test that grades one are then held to the same statement of
    what the specification requires, and cannot drift apart."""
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                        os.pardir, 'unit', 'pathloss_reference.py')
    spec = importlib.util.spec_from_file_location('pathloss_reference', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def config_values(names):
    """The scenario's own configuration, so that a check states the model's
    arithmetic and not the scenario's numbers: edit the ini and the expected
    value follows. Units are stripped -- every quantity read here is in the
    unit the ini writes it in."""
    entries = scave.get_config_entries('*')
    out = {}
    for name in names:
        hits = entries[entries.name == name].value.unique()
        if len(hits) != 1:
            raise LookupError(f"expected exactly one value for config entry "
                              f"'{name}', found {list(hits)}")
        out[name] = float(re.match(r'-?[\d.]+', hits[0]).group(0))
    return out


def all_scalars_by_run(itervars, filter_expression='module =~ "*"'):
    """Every recorded scalar of every run, indexed by (module, name) with one
    column per combination of the swept variables.

    This is what an invariance check compares. Asserting over the whole result
    set rather than over one chosen statistic is the point: a transformation
    that ought to change nothing has to change nothing, and picking a statistic
    in advance would only test the part that was already suspected."""
    df = scave.get_scalars(filter_expression, include_itervars=True,
                           convert_to_base_unit=False)
    if df.empty:
        raise LookupError(f"no scalars matched: {filter_expression}")
    for var in itervars:
        if var not in df.columns:
            raise LookupError(f"results carry no iteration variable '{var}'")
        try:
            df[var] = df[var].astype(float)
        except ValueError:
            pass
    return df.pivot_table(index=['module', 'name'], columns=list(itervars),
                          values='value', aggfunc='first', dropna=False)


def differing_scalars(table, reference, other):
    """The rows where two runs disagree. Two recorded nans count as agreeing:
    a statistic that was declared and never emitted is not a difference."""
    a, b = table[reference], table[other]
    agree = (a == b) | (a.isna() & b.isna())
    return table[~agree]


def check_runs_identical(grader, table, reference, other, ref_name, label):
    """One verdict for "this transformation changed nothing", naming what it
    did change when it changed something."""
    diff = differing_scalars(table, reference, other)
    detail = f"{len(diff)} of {len(table)} recorded scalars differ from {ref_name}"
    if not diff.empty:
        worst = diff.head(3)
        detail += ": " + "; ".join(
            f"{module}.{name} {worst.loc[(module, name), reference]} vs "
            f"{worst.loc[(module, name), other]}"
            for module, name in worst.index)
    grader.check_true('invariance', label, diff.empty, detail)


def grade_geometry_invariance(grader):
    """Neither where the network sits nor which way it faces may reach a
    result. Both transformations map the coordinates onto exactly
    representable ones, so the demand is equality rather than closeness."""
    table = all_scalars_by_run(('dx', 'rot'))
    reference = (0.0, 0.0)
    if reference not in table.columns:
        raise LookupError("the untransformed run is missing from the results")
    check_runs_identical(grader, table, reference, (1000.0, 0.0),
                         'the untransformed run', 'translated by 1000 m')
    check_runs_identical(grader, table, reference, (0.0, 90.0),
                         'the untransformed run', 'rotated a quarter turn')


def check_relabelled_runs(grader, table, reference, other, relabel, ref_name, label,
                          ignore=()):
    """One verdict for "this permutation only permuted the labels".

    Unlike an invariance check the two runs are not expected to be equal
    element by element: the transformation renames the parts, so each scalar
    of the reference run is compared against the correspondingly renamed
    scalar of the other. A name with no counterpart is a failure and not a
    silent skip -- that is what would hide a relabelling rule that does not
    cover everything it should."""
    mismatched, compared, unmatched = [], 0, []
    for (module, name), value in table[reference].items():
        if any(pattern in module for pattern in ignore):
            continue
        key = (relabel(module), name)
        if key not in table[other].index:
            unmatched.append((module, name))
            continue
        compared += 1
        counterpart = table[other][key]
        if not (value == counterpart
                or (value != value and counterpart != counterpart)):
            mismatched.append((module, name, value, counterpart))

    detail = f"{len(mismatched)} of {compared} scalars differ from {ref_name} once relabelled"
    if unmatched:
        detail += f"; {len(unmatched)} had no counterpart, e.g. {unmatched[0][0]}"
    if mismatched:
        detail += ": " + "; ".join(f"{m.split('.', 1)[-1]}.{n} {v} vs {o}"
                                   for m, n, v, o in mismatched[:3])
    grader.check_true('permutation', label, not mismatched and not unmatched, detail)


def grade_carrier_symmetry(grader):
    """A carrier is identified by its frequency; which slot of the
    componentCarrier and channelModel vectors it occupies is not a physical
    fact. Exchanging the two indices must therefore exchange the two carriers'
    results and change nothing else."""
    table = all_scalars_by_run(('swap',))
    if 0.0 not in table.columns or 1.0 not in table.columns:
        raise LookupError("both carrier orderings are needed")

    pattern = re.compile(r'((?:nr)?[Cc]hannelModel|componentCarrier)\[([01])\]')
    relabel = lambda module: pattern.sub(
        lambda hit: f"{hit.group(1)}[{1 - int(hit.group(2))}]", module)

    check_relabelled_runs(
        grader, table, 0.0, 1.0, relabel, 'the unswapped run',
        'carrier indices exchanged',
        # The UE's LTE channel model has one carrier, so it has no index to be
        # exchanged with, and it is pinned to componentCarrier[0] by default --
        # which this configuration does swap the frequency of. It records
        # nothing in a standalone NR scenario, but it is excluded by name
        # rather than by being quietly unmatched.
        ignore=('.cellularNic.channelModel[',))


def grade_module_order_permutation(grader):
    """Swapping two UEs' positions must swap their results and change nothing
    else: which slot of a module vector a UE occupies is a property of how the
    network was written down, not of the radio environment."""
    # Scoped to what a UE index can address. The dynamically created RLC
    # entities are named after the MacNodeId, which follows module index, so
    # they move between parents when the UEs exchange cells -- comparing them
    # would report a naming artifact as a physics difference.
    table = all_scalars_by_run(('swap',),
                               'module =~ "*.ue[*].cellularNic.nrChannelModel[*]"'
                               ' OR module =~ "*.ue[*].cellularNic.nrPhy"'
                               ' OR module =~ "*.ue[*].cellularNic.nrMac"'
                               ' OR module =~ "*.ue[*].app[*]"')
    if 0.0 not in table.columns or 1.0 not in table.columns:
        raise LookupError("both the reference and the swapped run are needed")

    pattern = re.compile(r'ue\[([01])\]')
    relabel = lambda module: pattern.sub(
        lambda hit: f"ue[{1 - int(hit.group(1))}]", module)

    check_relabelled_runs(grader, table, 0.0, 1.0, relabel,
                          'the unswapped run', 'two UEs exchanged')


class Budget:
    """The downlink budget of a scenario in which nothing is random: a sum of
    configured constants and one path loss. Stated once, because the
    interference check needs exactly the same arithmetic for the interferer as
    the link-budget check needs for the wanted signal."""

    def __init__(self):
        self.plr = pathloss_reference()
        self.cfg = config_values([
            '**.eNodeBTxPower', '**.antennGainEnB', '**.antennaGainUe', '**.cableLoss',
            '**.thermalNoise', '**.ueNoiseFigure', '**.nodebHeight', '**.ueHeight',
            '*.carrierAggregation.componentCarrier[0].carrierFrequency',
        ])
        self.hBS = self.cfg['**.nodebHeight']
        self.hUT = self.cfg['**.ueHeight']
        self.fc = self.cfg['*.carrierAggregation.componentCarrier[0].carrierFrequency']
        self.noise = self.cfg['**.thermalNoise'] + self.cfg['**.ueNoiseFigure']

    def received_power(self, d2D):
        """dBm at the UE, for a base station d2D metres away."""
        d3D = math.sqrt(d2D ** 2 + (self.hBS - self.hUT) ** 2)
        return (self.cfg['**.eNodeBTxPower']
                - self.plr.t901_umi_los(d3D, d2D, self.fc, self.hBS, self.hUT)
                + self.cfg['**.antennGainEnB'] + self.cfg['**.antennaGainUe']
                - self.cfg['**.cableLoss'])

    def background_received_power(self, distance, tx_power):
        """dBm at the UE from an external or background cell.

        Same budget as a real base station's, but a different propagation
        model: computeExtCellPathLoss always applies the TR 36.814 formulas,
        whatever study the channel model itself is configured with, and passes
        the plain distance between the two nodes as both the 3D and the 2D
        one. NLOS, which is what enableExtCellLos = false selects; left true,
        the interferer would borrow the serving link's LOS state."""
        return (tx_power - self.plr.t814_umi_nlos(distance, self.fc)
                + self.cfg['**.antennGainEnB'] + self.cfg['**.antennaGainUe']
                - self.cfg['**.cableLoss'])

    def over_noise(self, signal_dbm, interference_dbm=None):
        """dB of a received power over the noise floor, or over noise plus one
        interferer."""
        denominator = 10 ** (self.noise / 10.0)
        if interference_dbm is not None:
            denominator += 10 ** (interference_dbm / 10.0)
        return signal_dbm - 10 * math.log10(denominator)

    def sinr(self, d_serving, d_interferer=None):
        """dB, for a base station d_serving away, over noise alone or over
        noise plus one more base station d_interferer away."""
        interference = None if d_interferer is None else self.received_power(d_interferer)
        return self.over_noise(self.received_power(d_serving), interference)


def grade_interference_delta(grader):
    """Two cells, one victim. The interfering cell is not busy in every TTI and
    the model only counts the bands it actually occupied, so the reported SINR
    spans two predictable endpoints rather than sitting at one value."""
    budget = Budget()
    pos = config_values(['*.gnb1.mobility.initialX', '*.gnb2.mobility.initialX',
                         '*.ue[0].mobility.initialX', '*.gnb1.mobility.initialZ',
                         '*.ue[0].mobility.initialZ'])
    d_serving = abs(pos['*.ue[0].mobility.initialX'] - pos['*.gnb1.mobility.initialX'])
    d_interferer = abs(pos['*.ue[0].mobility.initialX'] - pos['*.gnb2.mobility.initialX'])

    grader.check_absolute('geometry-is-consistent', 'formula vs coordinates',
                          pos['*.gnb1.mobility.initialZ'] - pos['*.ue[0].mobility.initialZ'],
                          budget.hBS - budget.hUT, 0.0, " m")

    quiet = budget.sinr(d_serving)
    loaded = budget.sinr(d_serving, d_interferer)

    table = scalar_table('module =~ "*.ue[0].*.nrChannelModel[*]" '
                         'AND name =~ "measuredSinrDl:*"', itervars=('dli',))
    for dli, row in table.iterrows():
        on = str(dli) == 'true'
        params = f"downlinkInterference={'true' if on else 'false'}"
        if not on:
            # Nothing varies, so the two extremes have to be the one budget
            # value. A spread here would mean something is still perturbing the
            # channel that the scenario believes it switched off.
            grader.check_absolute('no-interference-is-flat', params,
                                  row['measuredSinrDl:max'] - row['measuredSinrDl:min'],
                                  0.0, 1e-9, " dB")
            grader.check_absolute('quiet-sinr', params,
                                  row['measuredSinrDl:min'], quiet, 1e-9, " dB")
        else:
            grader.check_absolute('idle-interferer-costs-nothing', params,
                                  row['measuredSinrDl:max'], quiet, 1e-9, " dB")
            grader.check_absolute('loaded-interferer-sinr', params,
                                  row['measuredSinrDl:min'], loaded, 1e-9, " dB")
            # The headline: the drop between the two endpoints is the ratio of
            # the interference-plus-noise floor to the noise floor, and nothing
            # else. This is the quantity no fingerprint row varies today.
            grader.check_absolute('interference-delta', params,
                                  row['measuredSinrDl:max'] - row['measuredSinrDl:min'],
                                  quiet - loaded, 1e-9, " dB")


def grade_interference_delta_ul(grader):
    """The uplink mirror of grade_interference_delta: two cells, a victim UE in
    the first, an interfering UE in the second. The interferer is not
    scheduled in every TTI, so the uplink SINR the victim's base station
    computes spans two predictable endpoints."""
    budget = Budget()
    cfg = config_values(['**.ueTxPower', '**.bsNoiseFigure',
                         '*.gnb1.mobility.initialX', '*.ue[0].mobility.initialX',
                         '*.ue[1].mobility.initialX', '*.gnb1.mobility.initialZ',
                         '*.ue[0].mobility.initialZ', '*.ue[1].mobility.initialZ'])
    grader.check_absolute('geometry-is-consistent', 'formula vs coordinates',
                          cfg['*.gnb1.mobility.initialZ'] - cfg['*.ue[0].mobility.initialZ'],
                          budget.hBS - budget.hUT, 0.0, " m")
    grader.check_absolute('geometry-is-consistent', 'interferer at UE height',
                          cfg['*.ue[1].mobility.initialZ'], budget.hUT, 0.0, " m")

    def received_at_gnb1(ue_x):
        """dBm at the victim's base station from a UE ue_x along the x axis.
        The same budget as the downlink one, with the UE's transmit power."""
        d2D = abs(ue_x - cfg['*.gnb1.mobility.initialX'])
        return (budget.received_power(d2D)
                - budget.cfg['**.eNodeBTxPower'] + cfg['**.ueTxPower'])

    def over_bs_noise(signal_dbm, interference_dbm=None):
        noise = budget.cfg['**.thermalNoise'] + cfg['**.bsNoiseFigure']
        denominator = 10 ** (noise / 10.0)
        if interference_dbm is not None:
            denominator += 10 ** (interference_dbm / 10.0)
        return signal_dbm - 10 * math.log10(denominator)

    signal = received_at_gnb1(cfg['*.ue[0].mobility.initialX'])
    interferer = received_at_gnb1(cfg['*.ue[1].mobility.initialX'])
    quiet = over_bs_noise(signal)
    loaded = over_bs_noise(signal, interferer)

    # Two statistics, because the walk has two branches: the base station's
    # CQI computation (measuredSinrUl) reads the current TTI's transmission
    # map, a data reception (rcvdSinrUl) the previous TTI's.
    table = scalar_table('module =~ "*.ue[0].*.nrChannelModel[*]" '
                         'AND (name =~ "measuredSinrUl:*" OR name =~ "rcvdSinrUl:*")',
                         itervars=('uli',))
    for uli, row in table.iterrows():
        on = str(uli) == 'true'
        for stat in ('measuredSinrUl', 'rcvdSinrUl'):
            params = f"{stat} uplinkInterference={'true' if on else 'false'}"
            if not on:
                grader.check_absolute('no-interference-is-flat', params,
                                      row[f'{stat}:max'] - row[f'{stat}:min'],
                                      0.0, 1e-9, " dB")
                grader.check_absolute('quiet-sinr', params,
                                      row[f'{stat}:min'], quiet, 1e-9, " dB")
            else:
                grader.check_absolute('idle-interferer-costs-nothing', params,
                                      row[f'{stat}:max'], quiet, 1e-9, " dB")
                grader.check_absolute('loaded-interferer-sinr', params,
                                      row[f'{stat}:min'], loaded, 1e-9, " dB")
                grader.check_absolute('interference-delta-ul', params,
                                      row[f'{stat}:max'] - row[f'{stat}:min'],
                                      quiet - loaded, 1e-9, " dB")


def grade_link_budget(grader):
    """With fading, shadowing and all interference off, an omnidirectional
    antenna and a fixed LOS state, the reported SINR is a sum of constants and
    one path loss."""
    budget = Budget()
    pos = config_values(['*.gnb.mobility.initialZ', '*.ue[0].mobility.initialZ'])

    # Tripwire, not a claim about the model: the heights the formula uses and
    # the heights the coordinates place the nodes at are independent inputs. If
    # they disagree the checks below still run -- they just grade a geometry the
    # model never evaluated.
    grader.check_absolute('geometry-is-consistent', 'formula vs coordinates',
                          pos['*.gnb.mobility.initialZ'] - pos['*.ue[0].mobility.initialZ'],
                          budget.hBS - budget.hUT, 0.0, " m")

    table = scalar_table('module =~ "*.nrChannelModel[*]" AND name =~ "measuredSinrDl:*"',
                         itervars=('d',))
    previous = None
    for d2D, row in table.iterrows():
        params = f"d={d2D:g}m"
        expected = budget.sinr(d2D)
        observed = row['measuredSinrDl:mean']
        # Exact to floating point: with the randomness switched off there is
        # nothing left for a tolerance to absorb, and a loose one here would
        # hide precisely the term-sized errors the check exists to catch.
        grader.check_absolute('link-budget', params, observed, expected, 1e-9, " dB")

        if previous is not None:
            grader.check_true('budget-falls-with-distance', params, observed < previous,
                              f"{observed:.4f} dB < {previous:.4f} dB at the previous distance")
        previous = observed


def grade_background_cell_floor(grader):
    """One real cell, one background cell. The wanted signal and the
    interferer are both closed-form, from two different propagation studies."""
    budget = Budget()
    pos = config_values([
        '*.gnb.mobility.initialX', '*.gnb.mobility.initialZ',
        '*.ue[0].mobility.initialX', '*.ue[0].mobility.initialZ',
        '*.bgCell[0].mobility.initialX', '*.bgCell[0].mobility.initialZ',
        '*.bgCell[0].bgScheduler.txPower',
    ])
    grader.check_absolute('geometry-is-consistent', 'formula vs coordinates',
                          pos['*.gnb.mobility.initialZ'] - pos['*.ue[0].mobility.initialZ'],
                          budget.hBS - budget.hUT, 0.0, " m")

    d_serving = abs(pos['*.ue[0].mobility.initialX'] - pos['*.gnb.mobility.initialX'])
    # The background path takes the straight distance between the two nodes,
    # heights included, and uses it as both the 3D and the 2D distance.
    d_bg = math.hypot(pos['*.bgCell[0].mobility.initialX'] - pos['*.ue[0].mobility.initialX'],
                      pos['*.bgCell[0].mobility.initialZ'] - pos['*.ue[0].mobility.initialZ'])

    signal = budget.received_power(d_serving)
    interferer = budget.background_received_power(d_bg, pos['*.bgCell[0].bgScheduler.txPower'])
    quiet = budget.over_noise(signal)
    loaded = budget.over_noise(signal, interferer)

    table = scalar_table('module =~ "*.ue[0].*.nrChannelModel[*]" '
                         'AND name =~ "measuredSinrDl:*"', itervars=('bgi',))
    for bgi, row in table.iterrows():
        on = str(bgi) == 'true'
        params = f"bgCellInterference={'true' if on else 'false'}"
        if not on:
            grader.check_absolute('no-interference-is-flat', params,
                                  row['measuredSinrDl:max'] - row['measuredSinrDl:min'],
                                  0.0, 1e-9, " dB")
            grader.check_absolute('quiet-sinr', params,
                                  row['measuredSinrDl:min'], quiet, 1e-9, " dB")
        else:
            grader.check_absolute('idle-bgcell-costs-nothing', params,
                                  row['measuredSinrDl:max'], quiet, 1e-9, " dB")
            grader.check_absolute('loaded-bgcell-sinr', params,
                                  row['measuredSinrDl:min'], loaded, 1e-9, " dB")
            grader.check_absolute('background-cell-floor', params,
                                  row['measuredSinrDl:max'] - row['measuredSinrDl:min'],
                                  quiet - loaded, 1e-9, " dB")


def grade_link_reciprocity(grader):
    """The downlink and uplink budgets of one link differ only by constants,
    so their SINRs must differ by that constant whatever the channel does to
    the link -- as long as the channel does the same thing to both ends."""
    budget = Budget()
    powers = config_values(['**.eNodeBTxPower', '**.ueTxPower',
                            '**.ueNoiseFigure', '**.bsNoiseFigure'])
    # Antenna gains and cable loss appear in both directions and cancel; what
    # is left is the transmit powers and the noise figures.
    expected = ((powers['**.eNodeBTxPower'] - powers['**.ueTxPower'])
                - (powers['**.ueNoiseFigure'] - powers['**.bsNoiseFigure']))

    table = scalar_table('module =~ "*.ue[0].*.nrChannelModel[*]" '
                         'AND (name =~ "measuredSinrDl:mean" OR name =~ "measuredSinrUl:mean")',
                         itervars=('shad',))
    for shad, row in table.iterrows():
        on = str(shad) == 'true'
        params = f"shadowing={'true' if on else 'false'}"
        ref = 'reciprocal-with-shadowing' if on else 'reciprocal-budget'
        grader.check_absolute(ref, params,
                              row['measuredSinrDl:mean'] - row['measuredSinrUl:mean'],
                              expected, 1e-9, " dB")


GRADERS = {
    'HarqResidualLoss': grade_harq_residual_loss,
    'LinkBudget': grade_link_budget,
    'InterferenceDelta': grade_interference_delta,
    'InterferenceDeltaUl': grade_interference_delta_ul,
    'GeometryInvariance': grade_geometry_invariance,
    'ModuleOrderPermutation': grade_module_order_permutation,
    'BackgroundCellFloor': grade_background_cell_floor,
    'LinkReciprocity': grade_link_reciprocity,
    'CarrierSymmetry': grade_carrier_symmetry,
}


def main():
    if len(sys.argv) != 3 or sys.argv[1] != '--grade':
        print("usage: physics_reference.py --grade <result-directory>", file=sys.stderr)
        return 2

    files = sorted(glob.glob(os.path.join(sys.argv[2], "*.sca")))
    if not files:
        print(f"FAILED no .sca files in {sys.argv[2]}", file=sys.stderr)
        return 2
    scave.set_inputs(files)

    # Which set of checks to apply is the configuration's own name, taken from
    # the results rather than from the directory the script happens to run in.
    # The filter of get_runattrs selects runs, not attribute names, so the
    # attribute is picked out afterwards.
    runattrs = scave.get_runattrs('*')
    names = set(runattrs[runattrs.name == 'configname'].value)
    grader = Grader()
    try:
        if len(names) != 1:
            raise LookupError(f"expected one configuration in {sys.argv[2]}, found {sorted(names)}")
        GRADERS[names.pop()](grader)
    except KeyError as e:
        print(f"cannot grade: no grader defined for configuration {e}", file=sys.stderr)
        return 2
    except LookupError as e:
        print(f"cannot grade: {e}", file=sys.stderr)
        return 2

    print(f"graded={grader.graded} failures={grader.failures}")

    # Exit 0 even when checks failed. The verdict file is the result -- the
    # .test file asserts on it -- and opp_test treats a non-zero post-run
    # command as an ERROR, which would override %expected-failure and make it
    # impossible to record a known deviation. A non-zero exit is reserved for
    # being unable to grade at all, which is a different thing from grading
    # something and finding it wrong.
    return 0


if __name__ == '__main__':
    sys.exit(main())
