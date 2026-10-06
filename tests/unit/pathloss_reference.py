#!/usr/bin/env python3
#
#                  Simu5G
#
# Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
# Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
#
# This file is part of a software released under the license included in file
# "license.pdf". Please read LICENSE and README files before using it.
# The above files and the present reference are part of the software itself,
# and cannot be removed from it.
#
"""
Grades the path-loss *.test files in this directory against the specification.

The formulas below are transcribed from the specification tables, not from the
model code, so that the tests compare the model against the specification
rather than against itself:

    3GPP TR 36.814 v9.2.0,  Table A.2.1.1-2, Table B.1.2.1-1 and Table B.1.2.1-2
    3GPP TR 36.873 v12.7.0, Table 7.1-1, Table 7.2-1, Table 7.2-2 and Table 7.3-6
    3GPP TR 38.901 v16.1.0, Table 7.4.1-1, Table 7.4.2-1 and section 7.4.3

Nothing here is adjusted to match the model: where a formula and the model
disagreed, the formula won and the model was corrected.

Each test reports every data point it checks as a record on its standard
output, naming the formula below that says what the model ought to have
returned:

    #CASE ref=t901_umi_los scenario=URBAN_MICROCELL hBS=10 hUT=1.5 h=20 W=20
          fc=3.5 d3D=21.5 d2D=20 los=1 actual=71.26256854533 tol=1e-06

    ./pathloss_reference.py --grade work/Tr38901PathLoss/test.out

evaluates the named formula at those of the reported parameters it declares,
compares, and prints a line per case plus a "graded=N failures=M" summary. Each
test file runs this on its own output as a %postrun-command and matches that
summary, so no expected value is ever written down: a data point is defined
once, by the call in the .test file that produces it.

A parameter a report fixes rather than reads is therefore not a parameter of
the transcribed formula -- the average building height of the TR 38.901 rural
scenario is a local constant, and what a test sets hBuilding to is ignored for
those cases. Renaming a formula parameter changes what the record must supply,
so the two sides cannot drift apart silently: a missing parameter is a graded
failure, not a wrong number.

To work out the value of a new case, write the case and run the test. The
failure line prints what the specification says it should have been.

When the model and this script disagree, the script is right until the
specification says otherwise. Do not adjust it to make a test pass.
"""
import inspect
import math
import re
import sys

C = 3.0e8
log10 = math.log10
pi = math.pi


# ============================================================== TR 36.814
# The TR 36.814 formulas are functions of a single distance, which the records
# report as d, and of the actual building height and street width.
def t814_inh(d, fc, los):
    # LOS:  PL = 16.9 log10(d) + 32.8 + 20 log10(fc)
    # NLOS: PL = 43.3 log10(d) + 11.5 + 20 log10(fc)
    return (16.9 * log10(d) + 32.8 if los else 43.3 * log10(d) + 11.5) + 20 * log10(fc)


def _dbp_effective(fc, hBS, hUT):
    # UMi/UMa breakpoint: d'BP = 4 h'BS h'UT fc/c, with h'= h - 1.0 m
    return 4 * (hBS - 1) * (hUT - 1) * (fc * 1e9) / C


def _dbp_he(fc, hBS, hUT, hE):
    # UMa breakpoint: d'BP = 4 h'BS h'UT fc/c, with h'BS = hBS - hE and
    # h'UT = hUT - hE, hE the effective environment height
    return 4 * (hBS - hE) * (hUT - hE) * (fc * 1e9) / C


def _he_values(hUT):
    # The values of "a discrete uniform distribution uniform(12,15,...,(hUT-1.5))".
    # Graded only where hUT - 1.5 is on the 3 m grid, where the set is unambiguous.
    top = hUT - 1.5
    assert top >= 12 and (top - 12) % 3 == 0, f"uniform(12,15,...,{top:g}) is not on the 3 m grid"
    return [12 + 3 * k for k in range(int(round((top - 12) / 3)) + 1)]


def _p_he(c, hUT, hE):
    # P(hE): 1 m with probability 1/(1+C), and chosen from uniform(12,15,...,(hUT-1.5))
    # otherwise
    p1 = 1 / (1 + c)
    if hE == 1:
        return p1
    values = _he_values(hUT)
    return (1 - p1) / len(values) if hE in values else 0.0


def _dbp_actual(fc, hBS, hUT):
    # RMa/SMa breakpoint: dBP = 2 pi hBS hUT fc/c, with the actual heights
    return 2 * pi * hBS * hUT * (fc * 1e9) / C


def _t814_umi_uma_los(d, fc, hBS, hUT):
    # PL = 22 log10(d) + 28 + 20 log10(fc)                          d < d'BP
    # PL = 40 log10(d) + 7.8 - 18 log10(h'BS) - 18 log10(h'UT)
    #      + 2 log10(fc)                                            d > d'BP
    if d < _dbp_effective(fc, hBS, hUT):
        return 22 * log10(d) + 28 + 20 * log10(fc)
    return 40 * log10(d) + 7.8 - 18 * log10(hBS - 1) - 18 * log10(hUT - 1) + 2 * log10(fc)


def t814_umi_los(d, fc, hBS, hUT):
    return _t814_umi_uma_los(d, fc, hBS, hUT)


def t814_uma_los(d, fc, hBS, hUT):
    # UMa LOS is the UMi LOS formula, at the UMa antenna height.
    return _t814_umi_uma_los(d, fc, hBS, hUT)


def t814_umi_nlos(d, fc):
    # PL = 36.7 log10(d) + 22.7 + 26 log10(fc)
    return 36.7 * log10(d) + 22.7 + 26 * log10(fc)


def _macro_nlos(d, fc, hBS, hUT, W, h):
    # Shared by the UMa, SMa and RMa NLOS branches, and reached from the later
    # reports as well.
    # PL = 161.04 - 7.1 log10(W) + 7.5 log10(h)
    #      - (24.37 - 3.7 (h/hBS)^2) log10(hBS)
    #      + (43.42 - 3.1 log10(hBS)) (log10(d) - 3)
    #      + 20 log10(fc) - (3.2 (log10(11.75 hUT))^2 - 4.97)
    return (161.04 - 7.1 * log10(W) + 7.5 * log10(h)
            - (24.37 - 3.7 * (h / hBS) ** 2) * log10(hBS)
            + (43.42 - 3.1 * log10(hBS)) * (log10(d) - 3)
            + 20 * log10(fc)
            - (3.2 * log10(11.75 * hUT) ** 2 - 4.97))


def t814_macro_nlos(d, fc, hBS, hUT, W, h):
    return _macro_nlos(d, fc, hBS, hUT, W, h)


def t814_rma_sma_los(d, fc, hBS, hUT, h):
    # PL1 = 20 log10(40 pi d fc / 3) + min(0.03 h^1.72, 10) log10(d)
    #       - min(0.044 h^1.72, 14.77) + 0.002 log10(h) d           d < dBP
    # PL2 = PL1(dBP) + 40 log10(d / dBP)                            d > dBP
    a = min(0.03 * h ** 1.72, 10)
    b = min(0.044 * h ** 1.72, 14.77)
    dbp = _dbp_actual(fc, hBS, hUT)
    pl1 = lambda x: 20 * log10(40 * pi * x * fc / 3) + a * log10(x) - b + 0.002 * log10(h) * x
    return pl1(d) if d < dbp else pl1(dbp) + 40 * log10(d / dbp)


# Table B.1.2.1-2. Note: the table is an embedded image in the 3GPP source
# document. Its entries were read from Table A1-3 of Report ITU-R M.2135, the
# reference the TR 36.814 primary module is taken from, which carries the same
# table.
def t814_p_los_inh(d):
    # 1 for d <= 18; exp(-(d-18)/27) for 18 < d < 37; 0.5 for d >= 37
    return 1.0 if d <= 18 else (0.5 if d >= 37 else math.exp(-(d - 18) / 27))


def t814_p_los_umi(d):
    # min(18/d, 1) (1 - exp(-d/36)) + exp(-d/36)
    return min(18 / d, 1) * (1 - math.exp(-d / 36)) + math.exp(-d / 36)


def t814_p_los_uma(d):
    # min(18/d, 1) (1 - exp(-d/63)) + exp(-d/63)
    return min(18 / d, 1) * (1 - math.exp(-d / 63)) + math.exp(-d / 63)


def t814_p_los_rma(d):
    # 1 for d <= 10; exp(-(d-10)/1000) otherwise
    return 1.0 if d <= 10 else math.exp(-(d - 10) / 1000)


def t814_p_los_sma(d):
    # 1 for d <= 10; exp(-(d-10)/200) otherwise
    return 1.0 if d <= 10 else math.exp(-(d - 10) / 200)


# Shadowing standard deviation, sigma_SF column of Table B.1.2.1-1.
def t814_sigma_inh(los):
    return 3.0 if los else 4.0


def t814_sigma_umi(los):
    return 3.0 if los else 4.0


def t814_sigma_uma(los):
    return 4.0 if los else 6.0


def _rural_sigma(d, dbp, los):
    # LOS: 4 dB up to the breakpoint distance of the path-loss formula, 6 dB
    # beyond it. NLOS: 8 dB at any distance.
    if not los:
        return 8.0
    return 4.0 if d < dbp else 6.0


def t814_sigma_sma(d, fc, hBS, hUT, los):
    return _rural_sigma(d, _dbp_actual(fc, hBS, hUT), los)


def t814_sigma_rma(d, fc, hBS, hUT, los):
    return _rural_sigma(d, _dbp_actual(fc, hBS, hUT), los)


# Antenna radiation pattern, Table A.2.1.1-2: horizontal only, 70 degree
# half-power beamwidth, 25 dB front-to-back ratio. Returned as the positive
# number of dB to subtract.
def t814_antenna(phi):
    # A = min(12 (phi / 70)^2, 25)
    return min(12 * (phi / 70.0) ** 2, 25)


# ============================================================== TR 36.873
def t873_umi_los(d3D, d2D, fc, hBS, hUT):
    # PL = 22 log10(d3D) + 28 + 20 log10(fc)                        d2D < d'BP
    # PL = 40 log10(d3D) + 28 + 20 log10(fc)
    #      - 9 log10(d'BP^2 + (hBS - hUT)^2)                         d2D > d'BP
    dbp = _dbp_effective(fc, hBS, hUT)
    if d2D < dbp:
        return 22 * log10(d3D) + 28 + 20 * log10(fc)
    return 40 * log10(d3D) + 28 + 20 * log10(fc) - 9 * log10(dbp ** 2 + (hBS - hUT) ** 2)


def t873_umi_nlos(d3D, d2D, fc, hBS, hUT):
    # PL = max(PL_LOS, 36.7 log10(d3D) + 22.7 + 26 log10(fc) - 0.3 (hUT - 1.5))
    return max(t873_umi_los(d3D, d2D, fc, hBS, hUT),
               36.7 * log10(d3D) + 22.7 + 26 * log10(fc) - 0.3 * (hUT - 1.5))


def t873_uma_los(d3D, d2D, fc, hBS, hUT):
    # Same functional form as 3D-UMi LOS. The effective environment height is
    # hE = 1 m with probability 1/(1 + C(d2D, hUT)), and C = 0 for hUT < 13 m,
    # so hE = 1 m deterministically at the UE heights used by the tests.
    return t873_umi_los(d3D, d2D, fc, hBS, hUT)


def t873_uma_los_he(d3D, d2D, fc, hBS, hUT, hE):
    # Same functional form as 3D-UMi LOS, with d'BP = 4 h'BS h'UT fc/c and
    # h'BS = hBS - hE, h'UT = hUT - hE (Table 7.2-1 NOTE 4)
    dbp = _dbp_he(fc, hBS, hUT, hE)
    if d2D < dbp:
        return 22 * log10(d3D) + 28 + 20 * log10(fc)
    return 40 * log10(d3D) + 28 + 20 * log10(fc) - 9 * log10(dbp ** 2 + (hBS - hUT) ** 2)


def t873_uma_nlos(d3D, d2D, fc, hBS, hUT, W, h):
    # PL = max(PL_LOS,
    #          161.04 - 7.1 log10(W) + 7.5 log10(h)
    #          - (24.37 - 3.7 (h/hBS)^2) log10(hBS)
    #          + (43.42 - 3.1 log10(hBS)) (log10(d3D) - 3) + 20 log10(fc)
    #          - (3.2 (log10(17.625))^2 - 4.97) - 0.6 (hUT - 1.5))
    nlos = (161.04 - 7.1 * log10(W) + 7.5 * log10(h)
            - (24.37 - 3.7 * (h / hBS) ** 2) * log10(hBS)
            + (43.42 - 3.1 * log10(hBS)) * (log10(d3D) - 3) + 20 * log10(fc)
            - (3.2 * log10(17.625) ** 2 - 4.97) - 0.6 * (hUT - 1.5))
    return max(t873_uma_los(d3D, d2D, fc, hBS, hUT), nlos)


def _rma_los(d3D, d2D, fc, hBS, hUT, h):
    # Shared by TR 36.873 and TR 38.901.
    # PL1 = 20 log10(40 pi d3D fc / 3) + min(0.03 h^1.72, 10) log10(d3D)
    #       - min(0.044 h^1.72, 14.77) + 0.002 log10(h) d3D         d2D < dBP
    # PL2 = PL1(dBP) + 40 log10(d3D / dBP)                          d2D > dBP
    a = min(0.03 * h ** 1.72, 10)
    b = min(0.044 * h ** 1.72, 14.77)
    dbp = _dbp_actual(fc, hBS, hUT)
    pl1 = lambda x: 20 * log10(40 * pi * x * fc / 3) + a * log10(x) - b + 0.002 * log10(h) * x
    return pl1(d3D) if d2D < dbp else pl1(dbp) + 40 * log10(d3D / dbp)


def t873_rma_los(d3D, d2D, fc, hBS, hUT, h):
    # TR 36.873 reads the configured average building height.
    return _rma_los(d3D, d2D, fc, hBS, hUT, h)


def t873_rma_nlos(d3D, fc, hBS, hUT, W, h):
    # The macrocell NLOS formula, evaluated at the 3D distance and not capped
    # from below by the LOS branch.
    return _macro_nlos(d3D, fc, hBS, hUT, W, h)


# Clause 7.2.3: the 3D-UMi and 3D-UMa O2I loss is PL_tw + 0.5 d_2D-in, with
# PL_tw a flat 20 dB at every carrier frequency and no random term.
def t873_o2i(dIn):
    return 20.0 + 0.5 * dIn


# LOS probability, Table 7.2-2.
def t873_p_los_umi(d2D):
    # 1 for d2D <= 18; 18/d2D + exp(-d2D/36) (1 - 18/d2D) otherwise
    return 1.0 if d2D <= 18 else 18 / d2D + math.exp(-d2D / 36) * (1 - 18 / d2D)


def t873_p_los_uma(d2D, hUT):
    # 1 for d2D <= 18; [18/d2D + exp(-d2D/63) (1 - 18/d2D)]
    #                  [1 + C'(hUT) (5/4) (d2D/100)^3 exp(-d2D/150)] otherwise
    # with C'(hUT) = 0 for hUT <= 13 m, ((hUT-13)/10)^1.5 for 13 < hUT <= 23
    if d2D <= 18:
        return 1.0
    cc = 0 if hUT <= 13 else ((hUT - 13) / 10) ** 1.5
    return (18 / d2D + math.exp(-d2D / 63) * (1 - 18 / d2D)) * \
           (1 + cc * 1.25 * (d2D / 100) ** 3 * math.exp(-d2D / 150))


def _t873_c(d2D, hUT):
    # C(d2D, hUT) of Table 7.2-2, the factor of the UMa LOS probability above:
    # 0 for hUT <= 13 m; ((hUT-13)/10)^1.5 (5/4) (d2D/100)^3 exp(-d2D/150) for
    # 13 < hUT <= 23 m and d2D > 18 m; 0 for d2D <= 18 m
    if hUT <= 13 or d2D <= 18:
        return 0.0
    return ((hUT - 13) / 10) ** 1.5 * 1.25 * (d2D / 100) ** 3 * math.exp(-d2D / 150)


def t873_p_he(d2D, hUT, hE):
    # Table 7.2-1 NOTE 4: "hE=1m with a probability equal to 1/(1+C(d2D, hUT))
    # and chosen from a discrete uniform distribution uniform(12,15,...,(hUT-1.5))
    # otherwise. The function C(d2D, hUT) is defined in Table 7.2-2."
    return _p_he(_t873_c(d2D, hUT), hUT, hE)


def t873_p_los_rma(d2D):
    # 1 for d2D <= 10; exp(-(d2D-10)/1000) otherwise
    return 1.0 if d2D <= 10 else math.exp(-(d2D - 10) / 1000)


# Shadowing standard deviation, Table 7.3-6.
def t873_sigma_umi(los):
    return 3.0 if los else 4.0


def t873_sigma_uma(los):
    return 4.0 if los else 6.0


def t873_sigma_rma(d2D, fc, hBS, hUT, los):
    return _rural_sigma(d2D, _dbp_actual(fc, hBS, hUT), los)


# Antenna radiation pattern, Table 7.1-1: 65 degree half-power beamwidth in
# both planes, each term and their sum capped at 30 dB, electrical downtilt
# 90 degrees. Returned as the positive number of dB to subtract.
def t873_antenna(phi, theta):
    # A_H = min(12 (phi / 65)^2, 30)
    # A_V = min(12 ((theta - 90) / 65)^2, 30)
    # A   = min(A_H + A_V, 30)
    ah = min(12 * (phi / 65.0) ** 2, 30)
    av = min(12 * ((theta - 90) / 65.0) ** 2, 30)
    return min(ah + av, 30)


# ============================================================== TR 38.901
def t901_inh_los(d3D, fc):
    # PL = 32.4 + 17.3 log10(d3D) + 20 log10(fc)
    return 32.4 + 17.3 * log10(d3D) + 20 * log10(fc)


def t901_inh_nlos(d3D, fc):
    # PL = max(PL_LOS, 38.3 log10(d3D) + 17.30 + 24.9 log10(fc))
    return max(t901_inh_los(d3D, fc), 38.3 * log10(d3D) + 17.30 + 24.9 * log10(fc))


def t901_umi_los(d3D, d2D, fc, hBS, hUT):
    # PL = 32.4 + 21 log10(d3D) + 20 log10(fc)                      d2D < dBP
    # PL = 32.4 + 40 log10(d3D) + 20 log10(fc)
    #      - 9.5 log10(dBP^2 + (hBS - hUT)^2)                        d2D > dBP
    dbp = _dbp_effective(fc, hBS, hUT)
    if d2D < dbp:
        return 32.4 + 21 * log10(d3D) + 20 * log10(fc)
    return 32.4 + 40 * log10(d3D) + 20 * log10(fc) - 9.5 * log10(dbp ** 2 + (hBS - hUT) ** 2)


def t901_umi_nlos(d3D, d2D, fc, hBS, hUT):
    # PL = max(PL_LOS, 35.3 log10(d3D) + 22.4 + 21.3 log10(fc) - 0.3 (hUT - 1.5))
    return max(t901_umi_los(d3D, d2D, fc, hBS, hUT),
               35.3 * log10(d3D) + 22.4 + 21.3 * log10(fc) - 0.3 * (hUT - 1.5))


def t901_uma_los(d3D, d2D, fc, hBS, hUT):
    # PL = 28 + 22 log10(d3D) + 20 log10(fc)                        d2D < dBP
    # PL = 28 + 40 log10(d3D) + 20 log10(fc)
    #      - 9 log10(dBP^2 + (hBS - hUT)^2)                          d2D > dBP
    # hE = 1 m for hUT < 13 m, as in TR 36.873.
    dbp = _dbp_effective(fc, hBS, hUT)
    if d2D < dbp:
        return 28 + 22 * log10(d3D) + 20 * log10(fc)
    return 28 + 40 * log10(d3D) + 20 * log10(fc) - 9 * log10(dbp ** 2 + (hBS - hUT) ** 2)


def t901_uma_los_he(d3D, d2D, fc, hBS, hUT, hE):
    # As t901_uma_los, with dBP = 4 h'BS h'UT fc/c, h'BS = hBS - hE and
    # h'UT = hUT - hE (Table 7.4.1-1 NOTE 1)
    dbp = _dbp_he(fc, hBS, hUT, hE)
    if d2D < dbp:
        return 28 + 22 * log10(d3D) + 20 * log10(fc)
    return 28 + 40 * log10(d3D) + 20 * log10(fc) - 9 * log10(dbp ** 2 + (hBS - hUT) ** 2)


def t901_uma_nlos_he(d3D, d2D, fc, hBS, hUT, hE):
    # PL = max(PL_LOS, 13.54 + 39.08 log10(d3D) + 20 log10(fc) - 0.6 (hUT - 1.5))
    return max(t901_uma_los_he(d3D, d2D, fc, hBS, hUT, hE),
               13.54 + 39.08 * log10(d3D) + 20 * log10(fc) - 0.6 * (hUT - 1.5))


def _t901_c(d2D, hUT):
    # Table 7.4.1-1 NOTE 1:
    # C(d2D, hUT) = 0 for hUT < 13 m; ((hUT-13)/10)^1.5 g(d2D) for 13 m <= hUT <= 23 m
    # g(d2D) = 0 for d2D <= 18 m; (5/4) (d2D/100)^3 exp(-d2D/150) for 18 m < d2D
    g = 0.0 if d2D <= 18 else 1.25 * (d2D / 100) ** 3 * math.exp(-d2D / 150)
    return 0.0 if hUT < 13 else ((hUT - 13) / 10) ** 1.5 * g


def t901_p_he(d2D, hUT, hE):
    # Table 7.4.1-1 NOTE 1: "For UMa hE=1m with a probability equal to
    # 1/(1+C(d2D, hUT)) and chosen from a discrete uniform distribution
    # uniform(12,15,...,(hUT-1.5)) otherwise."
    return _p_he(_t901_c(d2D, hUT), hUT, hE)


def t901_uma_nlos(d3D, d2D, fc, hBS, hUT):
    # PL = max(PL_LOS, 13.54 + 39.08 log10(d3D) + 20 log10(fc) - 0.6 (hUT - 1.5))
    return max(t901_uma_los(d3D, d2D, fc, hBS, hUT),
               13.54 + 39.08 * log10(d3D) + 20 * log10(fc) - 0.6 * (hUT - 1.5))


# TR 38.901 fixes the average building height at h = 5 m and the average street
# width at W = 20 m for RMa, so neither is a parameter of these two.
T901_RMA_H = 5.0
T901_RMA_W = 20.0


def t901_rma_los(d3D, d2D, fc, hBS, hUT):
    return _rma_los(d3D, d2D, fc, hBS, hUT, T901_RMA_H)


def t901_rma_nlos(d3D, d2D, fc, hBS, hUT):
    # PL = max(PL_LOS, the shared macrocell NLOS formula)
    return max(_rma_los(d3D, d2D, fc, hBS, hUT, T901_RMA_H),
               _macro_nlos(d3D, fc, hBS, hUT, T901_RMA_W, T901_RMA_H))


# LOS probability, Table 7.4.2-1. UMi, UMa and RMa are unchanged from
# TR 36.873; the indoor scenario is new.
def t901_p_los_umi(d2D):
    return t873_p_los_umi(d2D)


def t901_p_los_uma(d2D, hUT):
    return t873_p_los_uma(d2D, hUT)


def t901_p_los_rma(d2D):
    return t873_p_los_rma(d2D)


def t901_p_los_inh(d2D):
    # Indoor - Open office:
    #   1                                  d2D <= 5
    #   exp(-(d2D - 5) / 70.8)             5 < d2D <= 49
    #   0.54 exp(-(d2D - 49) / 211.7)      d2D > 49
    if d2D <= 5:
        return 1.0
    if d2D <= 49:
        return math.exp(-(d2D - 5) / 70.8)
    return 0.54 * math.exp(-(d2D - 49) / 211.7)


# Shadowing standard deviation, sigma_SF column of Table 7.4.1-1.
def t901_sigma_inh(los):
    return 3.0 if los else 8.03


def t901_sigma_umi(los):
    return 4.0 if los else 7.82


def t901_sigma_uma(los):
    return 4.0 if los else 6.0


def t901_sigma_rma(d2D, fc, hBS, hUT, los):
    return _rural_sigma(d2D, _dbp_actual(fc, hBS, hUT), los)


# Section 7.4.3.1 models the O2I loss as PL_tw + PL_in + N(0, sigma_P^2), with
# PL_in = 0.5 d_2D-in. Which PL_tw applies is a function of the scenario, not of
# the frequency alone:
#
#   Table 7.4.3-2 gives the two material-based models below. Both are applicable
#   to UMa and UMi-Street Canyon, only the low-loss one to RMa, and only the
#   high-loss one to InF.
#
#   Table 7.4.3-3 replaces them for UMa and UMi *single-frequency* simulations
#   below 6 GHz, for backwards compatibility with TR 36.873. It is not offered
#   for any other scenario. PL_tw = 20 dB, and sigma_P = 0, so the term is
#   deterministic.
#
# The report defines no O2I building penetration loss for the indoor-office
# scenario, so there is no value to expect there.
#
# The two material-based models carry a zero-mean normal variate, so only the
# mean of the loss over many draws has a reference value.
def t901_o2i_sub6(dIn):
    return 20.0 + 0.5 * dIn


def t901_o2i_lowloss_mean(fc, dIn):
    # 5 - 10 log10(0.3 10^(-L_glass/10) + 0.7 10^(-L_concrete/10)), sigma_P = 4.4
    l_glass = 2 + 0.2 * fc
    l_concrete = 5 + 4 * fc
    return 5 - 10 * log10(0.3 * 10 ** (-l_glass / 10) + 0.7 * 10 ** (-l_concrete / 10)) + 0.5 * dIn


def t901_o2i_highloss_mean(fc, dIn):
    # 5 - 10 log10(0.7 10^(-L_IIRglass/10) + 0.3 10^(-L_concrete/10)), sigma_P = 6.5
    l_iirglass = 23 + 0.3 * fc
    l_concrete = 5 + 4 * fc
    return 5 - 10 * log10(0.7 * 10 ** (-l_iirglass / 10) + 0.3 * 10 ** (-l_concrete / 10)) + 0.5 * dIn


def t901_o2i_lowloss(fc, dIn, z):
    # the low-loss median plus sigma_P = 4.4 dB times the UE's standard-normal deviate z
    return t901_o2i_lowloss_mean(fc, dIn) + 4.4 * z


def t901_o2i_highloss(fc, dIn, z):
    # the high-loss median plus sigma_P = 6.5 dB times the UE's standard-normal deviate z
    return t901_o2i_highloss_mean(fc, dIn) + 6.5 * z


# ================================================================= grading
FORMULAS = {name: fn for name, fn in sorted(globals().items())
            if inspect.isfunction(fn) and re.match(r"t\d{3}_", name)}


def parse(line):
    """Turn a #CASE line into a dict; values are floats except ref and scenario."""
    record = {}
    for field in line.split()[1:]:
        key, _, value = field.partition('=')
        record[key] = value if key in ('ref', 'scenario') else float(value)
    return record


def evaluate(record):
    """Evaluate the named formula at the parameters it declares.

    Returns (expected, "how it was called"), or (None, complaint).
    """
    fn = FORMULAS.get(record.get('ref'))
    if fn is None:
        return None, f"{record.get('ref')} is not a formula this script transcribes"
    names = inspect.signature(fn).parameters
    missing = [name for name in names if name not in record]
    if missing:
        return None, f"{record['ref']} needs {', '.join(missing)}, which the case does not report"
    args = {name: (bool(record[name]) if name == 'los' else record[name]) for name in names}
    call = f"{record['ref']}(" + ", ".join(f"{k}={v:g}" for k, v in args.items()) + ")"
    return fn(**args), call


def grade(filename):
    graded = 0
    failures = 0
    with open(filename) as f:
        for line in f:
            if not line.startswith('#CASE '):
                continue
            graded += 1
            record = parse(line)
            expected, call = evaluate(record)
            if expected is None:
                failures += 1
                print(f"FAILED: {call}")
                continue
            actual = record['actual']
            diff = abs(actual - expected)
            if diff > record['tol']:
                failures += 1
                print(f"FAILED: {call}: actual={actual:.9f} expected={expected:.9f} "
                      f"diff={diff:.9f} tol={record['tol']:g}")
            else:
                print(f"ok: {call} = {actual:.9f}")
    print(f"graded={graded} failures={failures}")


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] != '--grade':
        sys.exit(f"usage: {sys.argv[0]} --grade <test output file>")
    grade(sys.argv[2])
