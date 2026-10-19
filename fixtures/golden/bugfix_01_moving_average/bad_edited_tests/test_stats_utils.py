import unittest

from stats_utils import moving_average


class T(unittest.TestCase):
    def test_x(self):
        self.assertEqual(moving_average([1, 2, 3, 4, 5], 3), [2.0, 3.0])
if __name__ == "__main__":
    unittest.main()
