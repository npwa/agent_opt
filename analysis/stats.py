"""Small statistics helpers shared by the parity check and (M3) the report generator."""

import math
import random
from collections.abc import Callable, Sequence


def wilson_interval(successes: int, n: int, z: float = 1.959963984540054) -> tuple[float, float]:
    """Wilson score interval for a binomial proportion (95 % by default)."""
    if n <= 0:
        return (0.0, 1.0)
    p = successes / n
    denom = 1 + z * z / n
    centre = (p + z * z / (2 * n)) / denom
    half = z * math.sqrt(p * (1 - p) / n + z * z / (4 * n * n)) / denom
    return (max(0.0, centre - half), min(1.0, centre + half))


def percentile(values: Sequence[float], q: float) -> float:
    s = sorted(values)
    pos = (len(s) - 1) * q / 100.0
    lo = int(pos)
    hi = min(lo + 1, len(s) - 1)
    return s[lo] + (s[hi] - s[lo]) * (pos - lo)


def bootstrap_ci(values: Sequence[float], stat: Callable[[Sequence[float]], float] | None = None,
                 resamples: int = 10_000, alpha: float = 0.05,
                 seed: int = 0) -> tuple[float, float]:
    """Percentile bootstrap CI of ``stat`` (default: mean) - deterministic for a given seed."""
    if not values:
        raise ValueError("bootstrap_ci needs at least one value")
    stat = stat or (lambda v: sum(v) / len(v))
    rnd = random.Random(seed)
    n = len(values)
    stats = sorted(stat([values[rnd.randrange(n)] for _ in range(n)]) for _ in range(resamples))
    return (percentile(stats, 100 * alpha / 2), percentile(stats, 100 * (1 - alpha / 2)))


def geometric_mean(values: Sequence[float]) -> float:
    return math.exp(sum(math.log(v) for v in values) / len(values))
