"""Price calculation."""


def calc_price(unit_price, qty, discount=0.0):
    """Total price for qty items at unit_price with a fractional discount (0.1 = 10%)."""
    if qty < 0:
        raise ValueError("qty must be non-negative")
    if not 0.0 <= discount < 1.0:
        raise ValueError("discount must be in [0, 1)")
    return round(unit_price * qty * (1.0 - discount), 2)
