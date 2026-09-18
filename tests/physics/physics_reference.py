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
import math
import os
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
        df[var] = df[var].astype(float)
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


GRADERS = {
    'HarqResidualLoss': grade_harq_residual_loss,
}


def main():
    if len(sys.argv) != 3 or sys.argv[1] != '--grade':
        print("usage: physics_reference.py --grade <result-directory>", file=sys.stderr)
        return 2

    files = sorted(glob.glob(os.path.join(sys.argv[2], "*.sca")))
    if not files:
        print(f"FAILED no .sca files in {sys.argv[2]}")
        print("graded=0 failures=1")
        return 1
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
        print(f"FAILED no grader defined for configuration {e}")
        grader.failures += 1
    except LookupError as e:
        print(f"FAILED {e}")
        grader.failures += 1

    print(f"graded={grader.graded} failures={grader.failures}")
    return 1 if grader.failures else 0


if __name__ == '__main__':
    sys.exit(main())
