import itertools
import random

import numpy as np
import pytest
from scipy import stats as sps

from analysis import stats

TOL = 1e-9


def rnd_samples(seed, n1, n2, ties=False):
    r = random.Random(seed)
    if ties:
        return [r.randint(0, 6) for _ in range(n1)], [r.randint(2, 9) for _ in range(n2)]
    return [r.gauss(0, 1) for _ in range(n1)], [r.gauss(0.4, 1.3) for _ in range(n2)]


# ---- descriptive ---------------------------------------------------------------------------------

def test_percentile_median_mean_match_numpy():
    data = [3.1, 0.2, 9.9, 4.4, 4.4, 1.0, 7.7]
    for q in (0, 1, 25, 50, 90, 99, 100):
        assert stats.percentile(data, q) == pytest.approx(float(np.percentile(data, q)), abs=TOL)
    assert stats.median(data) == pytest.approx(float(np.median(data)))
    assert stats.mean(data) == pytest.approx(float(np.mean(data)))
    assert stats.geometric_mean([1, 4]) == pytest.approx(2.0)
    assert stats.is_directional(4) and not stats.is_directional(5)


# ---- Wilson / Newcombe ----------------------------------------------------------------------------

@pytest.mark.parametrize(("k", "n"), [(0, 10), (10, 10), (3, 10), (57, 100), (1, 2), (299, 300), (1, 1)])
def test_wilson_matches_scipy(k, n):
    ref = sps.binomtest(k, n).proportion_ci(confidence_level=0.95, method="wilson")
    lo, hi = stats.wilson_interval(k, n)
    assert lo == pytest.approx(ref.low, abs=TOL) and hi == pytest.approx(ref.high, abs=TOL)
    assert stats.wilson_interval(0, 0) == (0.0, 1.0)


def test_newcombe_published_example():
    # Newcombe (1998), Statistics in Medicine 17:873: 56/70 vs 48/80, method 10 -> (0.0524, 0.3339)
    lo, hi = stats.newcombe_diff_interval(56, 70, 48, 80)
    assert lo == pytest.approx(0.0524, abs=5e-4) and hi == pytest.approx(0.3339, abs=5e-4)
    # symmetric under swapping the groups
    lo2, hi2 = stats.newcombe_diff_interval(48, 80, 56, 70)
    assert (lo2, hi2) == pytest.approx((-hi, -lo), abs=TOL)


# ---- bootstrap -------------------------------------------------------------------------------------------

def test_bootstrap_is_deterministic_and_brackets_statistic():
    data = rnd_samples(1, 40, 1)[0]
    a = stats.bootstrap_ci(data, resamples=2000, seed=7)
    assert a == stats.bootstrap_ci(data, resamples=2000, seed=7)
    assert a != stats.bootstrap_ci(data, resamples=2000, seed=8)
    assert a[0] < stats.mean(data) < a[1]
    lo, hi = stats.bootstrap_ci(data, stats.median, resamples=2000)
    assert lo <= stats.median(data) <= hi
    with pytest.raises(ValueError):
        stats.bootstrap_ci([])


def test_bootstrap_coverage_is_nominal():
    """Plan test: 1 000 simulated experiments, the 95 % interval covers the truth 93-97 % of the time."""
    rnd = random.Random(2026)
    sims, hits = 1000, 0
    for i in range(sims):
        sample = [rnd.gauss(5.0, 2.0) for _ in range(60)]
        lo, hi = stats.bootstrap_ci(sample, resamples=400, seed=i)
        hits += lo <= 5.0 <= hi
    assert 0.93 <= hits / sims <= 0.97, hits / sims


def test_two_sample_bootstrap_ratio_and_difference_recover_truth():
    r = random.Random(5)
    base = [r.lognormvariate(4.0, 0.2) for _ in range(30)]
    var = [r.lognormvariate(4.0, 0.2) * 0.7 for _ in range(30)]  # 30 % faster
    lo, hi = stats.bootstrap_ci_two_sample(var, base, lambda a, b: stats.median(a) / stats.median(b),
                                           resamples=2000)
    assert lo < 0.7 < hi and hi < 1.0
    lo, hi = stats.bootstrap_ci_two_sample(base, base, lambda a, b: stats.median(a) - stats.median(b),
                                           resamples=2000)
    assert lo < 0 < hi  # identical groups: the difference CI straddles zero
    with pytest.raises(ValueError):
        stats.bootstrap_ci_two_sample([], [1.0], lambda a, b: 0.0)


# ---- Mann-Whitney U ----------------------------------------------------------------------------------------

@pytest.mark.parametrize(("n1", "n2", "ties"), [(10, 10, False), (10, 10, True), (25, 40, False),
                                                (30, 30, True), (5, 60, False), (50, 50, True)])
@pytest.mark.parametrize("alt", ["two-sided", "greater", "less"])
def test_mann_whitney_asymptotic_matches_scipy(n1, n2, ties, alt):
    x, y = rnd_samples(n1 * 31 + n2, n1, n2, ties)
    ref = sps.mannwhitneyu(x, y, alternative=alt, method="asymptotic")
    res = stats.mann_whitney_u(x, y, alternative=alt, method="asymptotic")
    assert res.u1 == pytest.approx(ref.statistic, abs=TOL)
    assert res.p == pytest.approx(ref.pvalue, abs=TOL)


@pytest.mark.parametrize(("n1", "n2"), [(3, 4), (5, 5), (8, 8), (6, 20), (2, 7)])
@pytest.mark.parametrize("alt", ["two-sided", "greater", "less"])
def test_mann_whitney_exact_matches_scipy(n1, n2, alt):
    x, y = rnd_samples(n1 * 7 + n2, n1, n2)
    ref = sps.mannwhitneyu(x, y, alternative=alt, method="exact")
    res = stats.mann_whitney_u(x, y, alternative=alt, method="exact")
    assert res.u1 == pytest.approx(ref.statistic, abs=TOL) and res.p == pytest.approx(ref.pvalue, abs=TOL)
    assert res.method == "exact"


def test_mann_whitney_auto_follows_scipy_rule():
    x, y = rnd_samples(1, 6, 30)
    assert stats.mann_whitney_u(x, y).method == "exact"
    assert stats.mann_whitney_u(*rnd_samples(1, 12, 12)).method == "asymptotic"
    tx, ty = rnd_samples(2, 6, 6, ties=True)
    assert stats.mann_whitney_u(tx, ty).method == "asymptotic"
    for a, b in ((x, y), (tx, ty), rnd_samples(3, 20, 15)):
        assert stats.mann_whitney_u(a, b).p == pytest.approx(sps.mannwhitneyu(a, b).pvalue, abs=TOL)
    with pytest.raises(ValueError):
        stats.mann_whitney_u(tx, ty, method="exact")


def test_mann_whitney_effect_sizes_and_edge_cases():
    res = stats.mann_whitney_u([1, 2, 3], [4, 5, 6])
    assert res.prob_superiority == 0.0 and res.rank_biserial == -1.0
    assert stats.mann_whitney_u([4, 5, 6], [1, 2, 3]).rank_biserial == 1.0
    same = stats.mann_whitney_u([1.0] * 12, [1.0] * 12)  # zero variance: no evidence
    assert same.p == 1.0 and same.prob_superiority == 0.5
    x, y = rnd_samples(9, 20, 20)
    r = stats.mann_whitney_u(x, y)
    assert r.rank_biserial == pytest.approx(2 * r.u1 / 400 - 1)
    with pytest.raises(ValueError):
        stats.mann_whitney_u([], [1])
    with pytest.raises(ValueError):
        stats.mann_whitney_u([1], [2], alternative="bigger")


def test_mann_whitney_exact_counts_sum_to_binomial():
    for n1, n2 in itertools.product(range(1, 6), range(1, 6)):
        from math import comb
        assert sum(stats._exact_u_counts(n1, n2)) == comb(n1 + n2, n1)


def test_mann_whitney_type_i_error_is_controlled():
    r = random.Random(11)
    rejections = 0
    sims = 400
    for _ in range(sims):
        x = [r.gauss(0, 1) for _ in range(15)]
        y = [r.gauss(0, 1) for _ in range(15)]
        rejections += stats.mann_whitney_u(x, y).p < 0.05
    assert rejections / sims < 0.09  # nominal 5 %, tolerant of simulation noise


# ---- Holm ----------------------------------------------------------------------------------------------------------

def test_holm_textbook_example():
    # R: p.adjust(c(0.01, 0.04, 0.03, 0.005), method = "holm") -> 0.03 0.06 0.06 0.02
    assert stats.holm_adjust([0.01, 0.04, 0.03, 0.005]) == pytest.approx([0.03, 0.06, 0.06, 0.02], abs=TOL)


def test_holm_properties():
    r = random.Random(4)
    for _ in range(50):
        p = [r.random() ** 2 for _ in range(r.randint(1, 12))]
        adj = stats.holm_adjust(p)
        m = len(p)
        assert all(a >= q - TOL and a <= 1.0 for a, q in zip(adj, p, strict=True))
        assert all(a <= min(1.0, m * q) + TOL for a, q in zip(adj, p, strict=True))  # never above Bonferroni
        order = sorted(range(m), key=lambda i: p[i])
        assert all(adj[order[i]] <= adj[order[i + 1]] + TOL for i in range(m - 1))  # monotone in p
    assert stats.holm_adjust([]) == [] and stats.holm_adjust([0.2]) == [0.2]


# ---- the reported delta ------------------------------------------------------------------------------------------------

def test_compare_samples_reports_everything_n3_requires():
    r = random.Random(8)
    base = [r.gauss(100, 8) for _ in range(12)]
    fast = [r.gauss(70, 8) for _ in range(12)]
    rep = stats.compare_samples(base, fast, resamples=2000)
    assert rep["n_baseline"] == rep["n_variant"] == 12
    assert rep["improved"] is True and rep["delta_ci"][1] < 0 and rep["ratio_ci"][1] < 1
    assert rep["p"] < 0.01 and rep["effect_size"]["rank_biserial"] < -0.7
    assert rep["test"].startswith("Mann-Whitney") and "bootstrap" in rep["ci_method"]
    assert rep["directional"] is False
    same = stats.compare_samples(base, base, resamples=1000)
    assert same["improved"] is False and same["p"] > 0.9
    assert stats.compare_samples(base[:4], fast[:4], resamples=200)["directional"] is True
    slow = stats.compare_samples(fast, base, resamples=1000, higher_is_better=True)
    assert slow["improved"] is True  # for higher-is-better metrics the CI must exclude 0 from above


def test_compare_rates_reports_wilson_and_difference_ci():
    rep = stats.compare_rates(k_baseline=48, n_baseline=80, k_variant=56, n_variant=70)
    assert rep["diff"] == pytest.approx(0.2) and rep["diff_ci"][0] == pytest.approx(0.0524, abs=5e-4)
    assert rep["wilson_baseline"] == pytest.approx(list(stats.wilson_interval(48, 80)))
    assert rep["directional"] is False and stats.compare_rates(1, 3, 2, 3)["directional"] is True
