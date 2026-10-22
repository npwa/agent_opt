"""Hidden verifier: every documented behaviour of bookings.py (stdlib unittest)."""
import copy
import unittest

from bookings import find_free_slots, merge_intervals, overlaps, total_busy_minutes


class Overlaps(unittest.TestCase):
    def test_touching_is_not_overlap_both_ways(self):
        self.assertFalse(overlaps((540, 600), (600, 660)))
        self.assertFalse(overlaps((600, 660), (540, 600)))

    def test_overlap_contained_and_disjoint(self):
        self.assertTrue(overlaps((540, 600), (599, 660)))
        self.assertTrue(overlaps((540, 720), (600, 630)))
        self.assertTrue(overlaps((600, 630), (540, 720)))
        self.assertFalse(overlaps((540, 600), (601, 660)))


class Merge(unittest.TestCase):
    def test_touching_and_overlapping_chain(self):
        self.assertEqual(merge_intervals([(600, 660), (540, 600), (650, 700)]), [(540, 700)])

    def test_unsorted_disjoint_stay_separate_and_sorted(self):
        self.assertEqual(merge_intervals([(700, 720), (540, 600)]), [(540, 600), (700, 720)])

    def test_nested_and_duplicates(self):
        self.assertEqual(merge_intervals([(540, 720), (600, 630), (540, 720)]), [(540, 720)])

    def test_empty_and_single(self):
        self.assertEqual(merge_intervals([]), [])
        self.assertEqual(merge_intervals([(1, 2)]), [(1, 2)])

    def test_input_not_modified(self):
        data = [(700, 720), (540, 600), (590, 610)]
        before = copy.deepcopy(data)
        out = merge_intervals(data)
        self.assertEqual(data, before)
        self.assertIsNot(out, data)
        self.assertEqual(out, [(540, 610), (700, 720)])

    def test_returns_new_list_even_when_nothing_to_merge(self):
        data = [(1, 2), (5, 6)]
        self.assertIsNot(merge_intervals(data), data)


class BusyMinutes(unittest.TestCase):
    def test_overlap_counts_once(self):
        self.assertEqual(total_busy_minutes([(540, 600), (570, 630)]), 90)

    def test_nested_touching_and_empty(self):
        self.assertEqual(total_busy_minutes([(540, 720), (600, 630)]), 180)
        self.assertEqual(total_busy_minutes([(540, 600), (600, 660)]), 120)
        self.assertEqual(total_busy_minutes([]), 0)

    def test_input_not_modified(self):
        data = [(600, 630), (540, 600)]
        total_busy_minutes(data)
        self.assertEqual(data, [(600, 630), (540, 600)])


class FreeSlots(unittest.TestCase):
    def test_no_busy_whole_day(self):
        self.assertEqual(find_free_slots([], 540, 1020, 30), [(540, 1020)])

    def test_trailing_and_leading_gap(self):
        self.assertEqual(find_free_slots([(600, 660)], 540, 1020, 30), [(540, 600), (660, 1020)])

    def test_gap_of_exactly_min_len_is_included(self):
        self.assertEqual(find_free_slots([(570, 600)], 540, 600, 30), [(540, 570)])
        self.assertEqual(find_free_slots([(540, 570), (600, 660)], 540, 660, 30), [(570, 600)])

    def test_shorter_gap_is_excluded(self):
        self.assertEqual(find_free_slots([(540, 570), (590, 660)], 540, 660, 30), [])

    def test_unsorted_and_overlapping_busy(self):
        busy = [(700, 760), (600, 660), (640, 720)]
        self.assertEqual(find_free_slots(busy, 540, 800, 30), [(540, 600), (760, 800)])

    def test_busy_outside_the_window_is_ignored(self):
        self.assertEqual(find_free_slots([(400, 560), (1000, 1100)], 540, 1020, 30), [(560, 1000)])

    def test_busy_covers_whole_day(self):
        self.assertEqual(find_free_slots([(500, 1100)], 540, 1020, 30), [])

    def test_min_len_longer_than_day(self):
        self.assertEqual(find_free_slots([], 540, 570, 60), [])

    def test_input_not_modified(self):
        busy = [(700, 760), (600, 660)]
        find_free_slots(busy, 540, 800, 30)
        self.assertEqual(busy, [(700, 760), (600, 660)])


if __name__ == "__main__":
    unittest.main()
