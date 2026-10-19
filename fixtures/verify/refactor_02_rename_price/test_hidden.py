"""Hidden verifier: new name works everywhere, behaviour unchanged, old name gone from API."""
import unittest

import pricing
from cart import Cart
from report import render


class Hidden(unittest.TestCase):
    def test_new_name(self):
        self.assertEqual(pricing.compute_total_price(2.5, 4), 10.0)
        self.assertEqual(pricing.compute_total_price(10.0, 3, 0.1), 27.0)
        self.assertEqual(pricing.compute_total_price(10.0, 3, discount=0.5), 15.0)

    def test_old_name_removed(self):
        self.assertFalse(hasattr(pricing, "calc_price"))

    def test_validation_preserved(self):
        with self.assertRaises(ValueError):
            pricing.compute_total_price(1.0, -1)
        with self.assertRaises(ValueError):
            pricing.compute_total_price(1.0, 1, 1.0)

    def test_cart_and_report(self):
        c = Cart()
        c.add("pen", 1.5, 10)
        c.add("book", 20.0, 2, 0.25)
        self.assertEqual(c.total(), 45.0)
        out = render(c)
        self.assertIn("TOTAL", out)
        self.assertIn("45.00", out)


if __name__ == "__main__":
    unittest.main()
