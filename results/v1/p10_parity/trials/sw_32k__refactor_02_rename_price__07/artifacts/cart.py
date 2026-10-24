"""Shopping cart built on pricing.calc_price."""
from pricing import compute_total_price as calc_price


class Cart:
    def __init__(self):
        self.lines = []

    def add(self, name, unit_price, qty, discount=0.0):
        self.lines.append((name, unit_price, qty, discount))

    def total(self):
        return round(sum(calc_price(p, q, d) for _, p, q, d in self.lines), 2)
