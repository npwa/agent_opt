import unittest

from cart import Cart
from pricing import compute_total_price
from report import render


class CartTests(unittest.TestCase):
    def test_calc_price(self):
        self.assertEqual(compute_total_price(2.5, 4), 10.0)
        self.assertEqual(compute_total_price(10.0, 3, 0.1), 27.0)

    def test_negative_qty(self):
        with self.assertRaises(ValueError):
            compute_total_price(1.0, -1)

    def test_cart_total(self):
        c = Cart()
        c.add("pen", 1.5, 10)
        c.add("book", 20.0, 2, 0.25)
        self.assertEqual(c.total(), 45.0)

    def test_report(self):
        c = Cart()
        c.add("pen", 1.5, 10)
        self.assertIn("TOTAL", render(c))


if __name__ == "__main__":
    unittest.main()
