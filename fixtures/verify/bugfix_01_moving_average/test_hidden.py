"""Hidden verifier: visible tests plus extra edge cases (stdlib unittest)."""
import unittest

from stats_utils import clamp, median, moving_average


class Hidden(unittest.TestCase):
    def test_basic(self):
        self.assertEqual(moving_average([1, 2, 3, 4, 5], 3), [2.0, 3.0, 4.0])

    def test_window_equals_length(self):
        self.assertEqual(moving_average([2, 4, 6], 3), [4.0])

    def test_window_one(self):
        self.assertEqual(moving_average([5, 6, 7], 1), [5.0, 6.0, 7.0])

    def test_window_longer_than_data(self):
        self.assertEqual(moving_average([1, 2], 3), [])

    def test_empty(self):
        self.assertEqual(moving_average([], 2), [])

    def test_count_of_windows(self):
        self.assertEqual(len(moving_average(list(range(10)), 4)), 7)

    def test_untouched_helpers(self):
        self.assertEqual(median([4, 1, 3, 2]), 2.5)
        self.assertEqual(clamp(9, 0, 3), 3)
        with self.assertRaises(ValueError):
            moving_average([1], -1)


if __name__ == "__main__":
    unittest.main()
