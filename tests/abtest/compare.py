#!/usr/bin/env python3
#
# Compares two sides of a replicated A/B run (see README), statistic by
# statistic: a two-sided Mann-Whitney U test per scalar, with the
# Benjamini-Hochberg correction for the number of tests in each configuration.
#
# usage: compare.py <outdir> <sideA> <sideB>
#    e.g. compare.py out A B    (the change)
#         compare.py out A A2   (the null control: same code, other seeds)
#
# A scalar is skipped when fewer than 20 runs of a side recorded a number for
# it, and counted as identical when both sides recorded the same multiset of
# values.
#
import sys, os, glob, shlex, math, collections
import numpy as np
from scipy import stats
root, A, B = sys.argv[1:4]
FIELDS = {"count", "mean", "sum", "max", "min"}

def parse(path):
    vals = {}
    cur = None
    with open(path) as f:
        for line in f:
            if line.startswith("scalar "):
                p = shlex.split(line)
                vals[(p[1], p[2], "")] = float(p[3])
                cur = None
            elif line.startswith("statistic "):
                p = shlex.split(line)
                cur = (p[1], p[2])
            elif line.startswith("field ") and cur:
                p = shlex.split(line)
                if p[1] in FIELDS:
                    vals[(cur[0], cur[1], p[1])] = float(p[2])
            elif line.startswith(("attr", "itervar", "config", "run ", "version")) or not line.strip():
                pass
            else:
                cur = None
    return vals

def load(side, cfgdir):
    runs = []
    for sca in sorted(glob.glob(f"{root}/{side}/{cfgdir}/r*/run.sca")):
        runs.append(parse(sca))
    return runs

def bh(ps, q=0.05):
    m = len(ps)
    order = sorted(range(m), key=lambda i: ps[i])
    k = 0
    for rank, i in enumerate(order, 1):
        if ps[i] <= q * rank / m:
            k = rank
    return set(order[:k])

allp = []
report = []
for cfgdir in sorted(os.listdir(f"{root}/{A}")):
    ra, rb = load(A, cfgdir), load(B, cfgdir)
    keys = set()
    for r in ra + rb:
        keys |= set(r)
    tested, identical, skipped = [], 0, 0
    for k in sorted(keys):
        xa = [r[k] for r in ra if k in r and not math.isnan(r[k])]
        xb = [r[k] for r in rb if k in r and not math.isnan(r[k])]
        if len(xa) < 20 or len(xb) < 20:
            skipped += 1; continue
        if sorted(xa) == sorted(xb):
            identical += 1; continue
        if np.std(xa) == 0 and np.std(xb) == 0:
            if xa[0] == xb[0]:
                identical += 1; continue
            tested.append((k, 0.0, xa, xb)); continue
        p = stats.mannwhitneyu(xa, xb, alternative="two-sided").pvalue
        tested.append((k, p, xa, xb))
    ps = [t[1] for t in tested]
    sig = bh(ps) if ps else set()
    nlt05 = sum(1 for p in ps if p < 0.05)
    report.append((cfgdir, len(ra), len(rb), len(tested), identical, skipped, nlt05, len(sig)))
    allp += ps
    print(f"\n=== {cfgdir}: runs {len(ra)}/{len(rb)}, tested {len(tested)}, identical {identical}, skipped {skipped}, "
          f"p<0.05 uncorrected {nlt05} ({100*nlt05/max(1,len(ps)):.1f}%), BH-significant {len(sig)}")
    for i in sorted(sig, key=lambda i: ps[i])[:15]:
        k, p, xa, xb = tested[i]
        ma, mb = np.mean(xa), np.mean(xb)
        sd = math.sqrt((np.var(xa, ddof=1) + np.var(xb, ddof=1)) / 2) or float("nan")
        print(f"   SIG p={p:.2e} d={(mb-ma)/sd:+.2f} {k[0]} {k[1]} {k[2]}: {ma:.6g} -> {mb:.6g}")
print("\n=== overall")
ps = np.array(allp)
print(f"tests {len(ps)}, p<0.05 {np.mean(ps<0.05)*100:.1f}%, p<0.01 {np.mean(ps<0.01)*100:.1f}%, BH-significant overall {len(bh(list(ps)))}")
print("p-value deciles:", np.histogram(ps, bins=10, range=(0,1))[0].tolist())
