"""Small statistics helpers used by the reporting scripts."""


def moving_average(values, window):
    """Return the average of every full sliding window of `window` consecutive values."""
    if window <= 0:
        raise ValueError("window must be positive")
    out = []
    for i in range(len(values) - window):
        out.append(sum(values[i:i + window]) / window)
    return out


def median(values):
    """Median of a non-empty sequence."""
    if not values:
        raise ValueError("median of empty sequence")
    s = sorted(values)
    mid = len(s) // 2
    if len(s) % 2 == 0:
        return (s[mid - 1] + s[mid]) / 2
    return s[mid]


def clamp(x, lo, hi):
    """Limit x to the closed interval [lo, hi]."""
    if lo > hi:
        raise ValueError("lo must not exceed hi")
    return max(lo, min(x, hi))
