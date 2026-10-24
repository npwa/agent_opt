"""Room-booking helpers.

Times are integer minutes since midnight. An interval is a tuple (start, end) with start < end;
the end minute is exclusive, so (9 * 60, 10 * 60) is the hour from 09:00 up to, but not including, 10:00.
"""


def overlaps(a, b):
    """True if intervals a and b share at least one minute.

    Touching intervals do not overlap: (540, 600) and (600, 660) share no minute.
    """
    return a[0] < b[1] and b[0] < a[1]


def merge_intervals(intervals):
    """Return a NEW list of disjoint intervals, sorted by start, covering exactly the same minutes.

    Intervals that overlap or touch are merged into one. The argument must not be modified.
    """
    sorted_intervals = sorted(intervals)  # don't mutate the argument
    merged = []
    for start, end in sorted_intervals:
        if merged and start <= merged[-1][1]:  # merge touching intervals too
            merged[-1] = (merged[-1][0], max(merged[-1][1], end))
        else:
            merged.append((start, end))
    return merged


def total_busy_minutes(intervals):
    """Number of distinct minutes covered by the intervals; a minute covered twice counts once."""
    return sum(end - start for start, end in intervals)


def find_free_slots(busy, day_start, day_end, min_len):
    """Free gaps inside the day window [day_start, day_end] that are at least min_len minutes long.

    `busy` may be unsorted and may contain overlapping intervals; parts of a busy interval that lie
    outside the day window are ignored. A gap of exactly min_len minutes is long enough. Returns a
    list of (start, end) tuples in chronological order.
    """
    # Clip busy intervals to [day_start, day_end] and skip those entirely outside
    clipped = []
    for start, end in busy:
        lo = max(start, day_start)
        hi = min(end, day_end)
        if lo < hi:
            clipped.append((lo, hi))

    # Merge overlapping clipped intervals
    merged = sorted(clipped)
    result = []
    for start, end in merged:
        if result and start <= result[-1][1]:
            result[-1] = (result[-1][0], max(result[-1][1], end))
        else:
            result.append((start, end))

    free = []
    cursor = day_start
    for start, end in result:
        gap_end = min(start, day_end)
        if gap_end - cursor >= min_len:
            free.append((cursor, gap_end))
        cursor = max(cursor, end)

    # Check remaining gap from last processed position to day_end
    if day_end > cursor and day_end - cursor >= min_len:
        free.append((cursor, day_end))

    return free
