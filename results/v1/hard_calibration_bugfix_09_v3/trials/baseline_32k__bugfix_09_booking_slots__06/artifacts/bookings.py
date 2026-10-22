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
    sorted_intervals = sorted(intervals)
    merged = []
    for start, end in sorted_intervals:
        if merged and start <= merged[-1][1]:
            merged[-1] = (merged[-1][0], max(merged[-1][1], end))
        else:
            merged.append((start, end))
    return merged


def total_busy_minutes(intervals):
    """Number of distinct minutes covered by the intervals; a minute covered twice counts once."""
    merged = merge_intervals(intervals)
    return sum(end - start for start, end in merged)


def find_free_slots(busy, day_start, day_end, min_len):
    """Free gaps inside the day window [day_start, day_end] that are at least min_len minutes long.

    `busy` may be unsorted and may contain overlapping intervals; parts of a busy interval that lie
    outside the day window are ignored. A gap of exactly min_len minutes is long enough. Returns a
    list of (start, end) tuples in chronological order.
    """
    free = []
    cursor = day_start
    for start, end in sorted(busy):
        start = max(start, day_start)
        end = min(end, day_end)
        if end <= start:
            continue
        if start - cursor >= min_len:
            free.append((cursor, start))
        cursor = max(cursor, end)
    if cursor < day_end and day_end - cursor >= min_len:
        free.append((cursor, day_end))
    return free
