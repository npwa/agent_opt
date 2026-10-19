"""Plain-text cart report."""
from cart import Cart
from pricing import calc_price


def render(cart: Cart) -> str:
    rows = []
    for name, unit_price, qty, discount in cart.lines:
        rows.append(f"{name:<10} {qty:>3} x {unit_price:>7.2f}  = {calc_price(unit_price, qty, discount):>8.2f}")
    rows.append(f"{'TOTAL':<10} {cart.total():>29.2f}")
    return "\n".join(rows)
