"""
Statistics for the evaluation (plan 5 and N3). Pure Python (no numpy/scipy at run time), every
function is verified against scipy / published values in tests/test_stats.py.

* wilson_interval, newcombe_diff_interval  - success rates and their differences
* bootstrap_ci, bootstrap_ci_two_sample    - percentile bootstrap (10 000 resamples by default)
* mann_whitney_u                           - U test with scipy-compatible exact/asymptotic methods
* holm_adjust                              - Holm-Bonferroni step-down adjustment
* compare_samples / compare_rates          - the "reported delta": n, medians, 95 % CI, test, effect size
"""

import math
import random
from collections.abc import Callable, Sequence
from dataclasses import dataclass

Z95 = 1.959963984540054
DIRECTIONAL_N = 5  # N3: results with fewer than 5 observations per group are only "directional"


# ---- descriptive --------------------------------------------------------------------------------

def percentile(values: Sequence[float], q: float) -> float:
    """Linear-interpolation percentile (q in 0..100), identical to numpy's default."""
    s = sorted(values)
    pos = (len(s) - 1) * q / 100.0
    lo = int(pos)
    hi = min(lo + 1, len(s) - 1)
    return s[lo] + (s[hi] - s[lo]) * (pos - lo)


def median(values: Sequence[float]) -> float:
    return percentile(values, 50)


def mean(values: Sequence[float]) -> float:
    return sum(values) / len(values)


def geometric_mean(values: Sequence[float]) -> float:
    return math.exp(sum(math.log(v) for v in values) / len(values))


def is_directional(n: int) -> bool:
    return n < DIRECTIONAL_N


# ---- proportions ----------------------------------------------------------------------------------

def wilson_interval(successes: int, n: int, z: float = Z95) -> tuple[float, float]:
    """Wilson score interval for a binomial proportion (95 % by default)."""
    if n <= 0:
        return (0.0, 1.0)
    p = successes / n
    denom = 1 + z * z / n
    centre = (p + z * z / (2 * n)) / denom
    half = z * math.sqrt(p * (1 - p) / n + z * z / (4 * n * n)) / denom
    return (max(0.0, centre - half), min(1.0, centre + half))


def newcombe_diff_interval(k1: int, n1: int, k2: int, n2: int,
                           z: float = Z95) -> tuple[float, float]:
    """Newcombe's hybrid score interval (method 10) for p1 - p2, built from Wilson limits."""
    p1, p2 = k1 / n1, k2 / n2
    l1, u1 = wilson_interval(k1, n1, z)
    l2, u2 = wilson_interval(k2, n2, z)
    d = p1 - p2
    return (d - math.sqrt((p1 - l1) ** 2 + (u2 - p2) ** 2),
            d + math.sqrt((u1 - p1) ** 2 + (p2 - l2) ** 2))


# ---- bootstrap -----------------------------------------------------------------------------------------

def bootstrap_ci(values: Sequence[float], stat: Callable[[Sequence[float]], float] | None = None,
                 resamples: int = 10_000, alpha: float = 0.05,
                 seed: int = 0) -> tuple[float, float]:
    """Percentile bootstrap CI of ``stat`` (default: mean) - deterministic for a given seed."""
    if not values:
        raise ValueError("bootstrap_ci needs at least one value")
    stat = stat or mean
    rnd = random.Random(seed)
    n = len(values)
    stats = sorted(stat([values[rnd.randrange(n)] for _ in range(n)]) for _ in range(resamples))
    return (percentile(stats, 100 * alpha / 2), percentile(stats, 100 * (1 - alpha / 2)))


def bootstrap_ci_two_sample(x: Sequence[float], y: Sequence[float],
                            stat: Callable[[Sequence[float], Sequence[float]], float],
                            resamples: int = 10_000, alpha: float = 0.05,
                            seed: int = 0) -> tuple[float, float]:
    """Percentile bootstrap CI of stat(x*, y*), resampling each group independently."""
    if not x or not y:
        raise ValueError("bootstrap_ci_two_sample needs two non-empty samples")
    rnd = random.Random(seed)
    nx, ny = len(x), len(y)
    out = sorted(stat([x[rnd.randrange(nx)] for _ in range(nx)],
                      [y[rnd.randrange(ny)] for _ in range(ny)]) for _ in range(resamples))
    return (percentile(out, 100 * alpha / 2), percentile(out, 100 * (1 - alpha / 2)))


# ---- Mann-Whitney U ------------------------------------------------------------------------------------

@dataclass(frozen=True)
class MannWhitney:
    u1: float            # U statistic of the first sample (as scipy reports it)
    p: float
    n1: int
    n2: int
    method: str          # "exact" or "asymptotic"
    prob_superiority: float  # P(x > y) + 0.5 P(x = y)
    rank_biserial: float     # 2 * prob_superiority - 1 (= Cliff's delta)


def _midranks(values: Sequence[float]) -> tuple[list[float], list[int]]:
    order = sorted(range(len(values)), key=lambda i: values[i])
    ranks = [0.0] * len(values)
    ties: list[int] = []
    i = 0
    while i < len(order):
        j = i
        while j + 1 < len(order) and values[order[j + 1]] == values[order[i]]:
            j += 1
        for k in range(i, j + 1):
            ranks[order[k]] = (i + j) / 2 + 1
        ties.append(j - i + 1)
        i = j + 1
    return ranks, ties


def _normal_sf(z: float) -> float:
    return 0.5 * math.erfc(z / math.sqrt(2))


def _exact_u_counts(n1: int, n2: int) -> list[int]:
    """Number of arrangements giving each U value 0..n1*n2 under H0 (no ties)."""
    # f(n1, n2) via the recurrence f_{n,m}(u) = f_{n-1,m}(u - m) + f_{n,m-1}(u)
    table: dict[tuple[int, int], list[int]] = {}

    def f(n: int, m: int) -> list[int]:
        if (n, m) in table:
            return table[(n, m)]
        if n == 0 or m == 0:
            res = [1]
        else:
            a, b = f(n - 1, m), f(n, m - 1)
            res = [0] * (n * m + 1)
            for u, c in enumerate(a):
                res[u + m] += c
            for u, c in enumerate(b):
                res[u] += c
        table[(n, m)] = res
        return res

    return f(n1, n2)


def mann_whitney_u(x: Sequence[float], y: Sequence[float], alternative: str = "two-sided",
                   method: str = "auto") -> MannWhitney:
    """Mann-Whitney U test, results identical to ``scipy.stats.mannwhitneyu`` (tested to 1e-9).

    alternative: 'two-sided', 'greater' (x stochastically larger) or 'less'.
    method: 'asymptotic' (tie-corrected normal approximation with continuity correction),
    'exact' (no ties required) or 'auto' (exact when min(n) <= 8 and there are no ties).
    """
    n1, n2 = len(x), len(y)
    if n1 == 0 or n2 == 0:
        raise ValueError("mann_whitney_u needs two non-empty samples")
    if alternative not in ("two-sided", "greater", "less"):
        raise ValueError(f"unknown alternative {alternative!r}")
    ranks, ties = _midranks(list(x) + list(y))
    u1 = sum(ranks[:n1]) - n1 * (n1 + 1) / 2
    u2 = n1 * n2 - u1
    has_ties = any(t > 1 for t in ties)
    if method == "auto":
        method = "exact" if (min(n1, n2) <= 8 and not has_ties) else "asymptotic"
    if method == "exact" and has_ties:
        raise ValueError("exact method requires no ties")

    if alternative == "greater":
        u, flip = u1, False
    elif alternative == "less":
        u, flip = u2, False
    else:
        u, flip = max(u1, u2), True

    if method == "exact":
        counts = _exact_u_counts(n1, n2)
        total = sum(counts)
        p = sum(counts[int(round(u)):]) / total  # P(U >= u)
    else:
        n = n1 + n2
        tie_term = sum(t**3 - t for t in ties)
        s = math.sqrt(n1 * n2 / 12 * ((n + 1) - tie_term / (n * (n - 1))))
        z = (u - n1 * n2 / 2 - 0.5) / s if s > 0 else 0.0
        p = _normal_sf(z) if s > 0 else 1.0
    if flip:
        p *= 2
    prob = u1 / (n1 * n2)
    return MannWhitney(u1, min(p, 1.0), n1, n2, method, prob, 2 * prob - 1)


# ---- multiple comparisons ----------------------------------------------------------------------------

def holm_adjust(pvalues: Sequence[float]) -> list[float]:
    """Holm-Bonferroni adjusted p-values (step-down, monotone), same order as the input."""
    m = len(pvalues)
    order = sorted(range(m), key=lambda i: pvalues[i])
    adjusted = [0.0] * m
    running = 0.0
    for rank, i in enumerate(order):
        running = max(running, min(1.0, (m - rank) * pvalues[i]))
        adjusted[i] = running
    return adjusted


# ---- the reported delta ------------------------------------------------------------------------------------

def compare_samples(baseline: Sequence[float], variant: Sequence[float], resamples: int = 10_000,
                    seed: int = 0, higher_is_better: bool = False) -> dict:
    """Variant vs baseline for a continuous metric (e.g. wall time).

    Reports what N3 requires for every delta: n, medians, difference and ratio of medians with
    95 % percentile-bootstrap CIs, the test used with its p-value, and an effect size.
    ``improved`` is True only when the CI of the improvement excludes 0 (acceptance rule 7.4).
    """
    d_lo, d_hi = bootstrap_ci_two_sample(
        variant, baseline, lambda a, b: median(a) - median(b), resamples, seed=seed)
    r_lo, r_hi = bootstrap_ci_two_sample(
        variant, baseline, lambda a, b: median(a) / median(b), resamples, seed=seed + 1)
    mw = mann_whitney_u(variant, baseline)
    delta = median(variant) - median(baseline)
    improved = (d_lo > 0) if higher_is_better else (d_hi < 0)
    return {
        "n_baseline": len(baseline), "n_variant": len(variant),
        "median_baseline": median(baseline), "median_variant": median(variant),
        "delta_median": delta, "delta_ci": [d_lo, d_hi],
        "ratio_median": median(variant) / median(baseline), "ratio_ci": [r_lo, r_hi],
        "test": f"Mann-Whitney U ({mw.method})", "p": mw.p,
        "effect_size": {"rank_biserial": mw.rank_biserial, "prob_superiority": mw.prob_superiority},
        "improved": improved,
        "directional": is_directional(min(len(baseline), len(variant))),
        "ci_method": f"percentile bootstrap, {resamples} resamples",
    }


def compare_rates(k_baseline: int, n_baseline: int, k_variant: int, n_variant: int) -> dict:
    """Success rates with Wilson intervals and the Newcombe interval of the difference."""
    lo, hi = newcombe_diff_interval(k_variant, n_variant, k_baseline, n_baseline)
    return {
        "n_baseline": n_baseline, "n_variant": n_variant,
        "rate_baseline": k_baseline / n_baseline, "rate_variant": k_variant / n_variant,
        "wilson_baseline": list(wilson_interval(k_baseline, n_baseline)),
        "wilson_variant": list(wilson_interval(k_variant, n_variant)),
        "diff": k_variant / n_variant - k_baseline / n_baseline, "diff_ci": [lo, hi],
        "ci_method": "Wilson intervals; Newcombe hybrid score interval for the difference",
        "directional": is_directional(min(n_baseline, n_variant)),
    }
