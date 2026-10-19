import unittest

from stats_utils import clamp, median, moving_average


class StatsTests(unittest.TestCase):
    def test_moving_average_basic(self):
        self.assertEqual(moving_average([1, 2, 3, 4, 5], 3), [2.0, 3.0, 4.0])

    def test_moving_average_window_equals_length(self):
        self.assertEqual(moving_average([2, 4, 6], 3), [4.0])

    def test_median(self):
        self.assertEqual(median([3, 1, 2]), 2)
        self.assertEqual(median([4, 1, 3, 2]), 2.5)

    def test_clamp(self):
        self.assertEqual(clamp(5, 0, 3), 3)
        self.assertEqual(clamp(-1, 0, 3), 0)

    def test_bad_window(self):
        with self.assertRaises(ValueError):
            moving_average([1, 2], 0)


if __name__ == "__main__":
    unittest.main()
