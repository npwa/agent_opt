"""Visible tests for bookings.py (run with: python3 -m unittest)."""
import unittest

from bookings import find_free_slots, merge_intervals, overlaps, total_busy_minutes


class OverlapTests(unittest.TestCase):
    def test_overlapping(self):
        self.assertTrue(overlaps((540, 600), (570, 630)))

    def test_disjoint(self):
        self.assertFalse(overlaps((540, 600), (660, 720)))

    def test_touching_is_not_overlap(self):
        self.assertFalse(overlaps((540, 600), (600, 660)))


class MergeTests(unittest.TestCase):
    def test_overlapping_merge(self):
        self.assertEqual(merge_intervals([(540, 600), (570, 660)]), [(540, 660)])

    def test_touching_merge(self):
        self.assertEqual(merge_intervals([(540, 600), (600, 660)]), [(540, 660)])


class SlotTests(unittest.TestCase):
    def test_busy_minutes_disjoint(self):
        self.assertEqual(total_busy_minutes([(540, 600), (660, 690)]), 90)

    def test_free_slots_basic(self):
        self.assertEqual(find_free_slots([(600, 660)], 540, 720, 30), [(540, 600), (660, 720)])


if __name__ == "__main__":
    unittest.main()
